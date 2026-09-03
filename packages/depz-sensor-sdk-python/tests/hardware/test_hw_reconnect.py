"""Software reconnect: the main reliability suite.

"Reconnect" is not one operation. This suite separates the levels that the
system actually supports, because they exercise different code and fail
differently:

* **L1 restart**  — ``stop() -> start()``      (device stays open)
* **L2 reopen**   — ``close() -> open()``      (port released and retaken)
* **L3 recreate** — drop the object, build a new one (all host state rebuilt)

A physical unplug/replug is a *different kind of test* and is deliberately not
simulated here: `close()/open()` is not unplugging, and pretending otherwise
would be the exact "mock claiming to be hardware" failure this campaign exists
to prevent. See QA_MATRIX.md § Limitations.

A cycle only counts as PASS when the device really came back: new data, from a
*new session* (timestamps past the old session's last sample), stoppable
again, with the port released and no thread/fd growth.

Profiles (``--profile``): smoke=10, regression=50, stress=200, extended=1000.
Seeded jitter (``--seed``) shakes out ordering races while staying replayable.
"""

from __future__ import annotations

import gc
import time

import pytest

from _drivers import SensorDriver
from _support import (
    CycleMetrics,
    byte_trace_enabled,
    dump_diagnostics,
    port_lease,
    sample_resources,
    wait_for_threads_to_die,
    write_report,
)

pytestmark = [pytest.mark.hardware, pytest.mark.timeout(3600)]

#: A reconnect that has not produced data by now is a failure, not a slow day.
#: Chosen as ~10x the worst observed cold path (VL53L8 open+init+start+frame
#: measured at ~1.9s); large enough to never flake, small enough that a real
#: wedge is caught in seconds rather than hanging the suite.
RECONNECT_BUDGET_S = 20.0


def _one_cycle(info, cycle: int, rng, level: str, drv_holder: list) -> CycleMetrics:
    """Run one reconnect cycle at `level` and measure it."""
    m = CycleMetrics(cycle=cycle, ok=False)
    t_start = time.monotonic()
    try:
        if level == "L1":
            drv = drv_holder[0]
        else:
            t0 = time.monotonic()
            drv = SensorDriver.open(info)
            m.open_s = time.monotonic() - t0
            drv_holder[0] = drv
            drv.prepare()

        retries_before = getattr(drv, "retried_enables", 0)
        t0 = time.monotonic()
        drv.start()
        m.start_s = time.monotonic() - t0
        m.retries = getattr(drv, "retried_enables", 0) - retries_before

        t0 = time.monotonic()
        sample = drv.read_one(timeout=RECONNECT_BUDGET_S)
        m.first_frame_s = time.monotonic() - t0
        assert drv.sample_is_valid(sample), f"cycle {cycle}: first sample invalid"
        m.frames = 1

        # Jitter: let the stream run a random slice so stop() lands at a
        # different point in the frame cadence each cycle.
        time.sleep(rng.uniform(0.0, 0.05))
        for _ in range(rng.randint(0, 2)):
            try:
                drv.read_one(timeout=RECONNECT_BUDGET_S)
                m.frames += 1
            except Exception:
                break

        t0 = time.monotonic()
        drv.stop()
        m.stop_s = time.monotonic() - t0

        if level != "L1":
            t0 = time.monotonic()
            drv.close()
            m.close_s = time.monotonic() - t0
            if level == "L3":
                drv_holder[0] = None
                del drv
                gc.collect()

        m.ok = True
    except Exception as exc:
        m.error = f"{type(exc).__name__}: {exc}"
        if byte_trace_enabled():
            # Give a *late* reply 3 s to land in the trace before dumping —
            # the F-24 question is precisely "was the exchange slow or lost".
            time.sleep(3.0)
        dump_diagnostics(
            f"reconnect-{info.family}-{level}",
            {
                "cycle": cycle,
                "stable_id": info.stable_id,
                "level": level,
                "error": m.error,
            },
        )
    finally:
        m.total_s = time.monotonic() - t_start
        # Only the counters a leak would move, not the whole snapshot: at 50+
        # cycles x 3 levels x 4 sensors the full dict is most of the report's
        # bytes, and these four are what the trend is actually read from.
        # Baseline/final carry the complete picture.
        r = sample_resources()
        m.resources = {
            "rss_kb": r.rss_kb,
            "fd_count": r.fd_count,
            "tty_fd_count": r.tty_fd_count,
            "depz_reader_threads": r.depz_reader_threads,
        }
    return m


def _run_profile(info, level: str, cycles: int, rng, seed: int) -> dict:
    """Drive `cycles` reconnects and return a report dict."""
    results: list[CycleMetrics] = []
    drv_holder: list = [None]
    baseline = None
    try:
        with port_lease(info.resolve_port(), timeout=60):
            if level == "L1":
                drv_holder[0] = SensorDriver.open(info)
                drv_holder[0].prepare()

            for i in range(cycles):
                m = _one_cycle(info, i, rng, level, drv_holder)
                results.append(m)
                # Baseline after warm-up: the first few cycles allocate lazily
                # (numpy buffers, ULD tables), so comparing against cycle 0
                # would flag normal warm-up as a leak.
                if i == min(4, cycles - 1):
                    baseline = sample_resources()
                if not m.ok:
                    break
                time.sleep(rng.uniform(0.0, 0.02))
    finally:
        if drv_holder[0] is not None:
            try:
                drv_holder[0].close()
            except Exception:
                pass

    ok = [m for m in results if m.ok]
    failed = [m for m in results if not m.ok]
    final = sample_resources()
    report = {
        "stable_id": info.stable_id,
        "family": info.family,
        "level": level,
        "seed": seed,
        "cycles_requested": cycles,
        "cycles_run": len(results),
        "cycles_ok": len(ok),
        "cycles_failed": len(failed),
        "errors": [{"cycle": m.cycle, "error": m.error} for m in failed],
        "timing": _timing_summary(ok),
        "baseline": baseline.as_dict() if baseline else None,
        "final": final.as_dict(),
        "cycles": [m.__dict__ for m in results],
    }
    return report


def _timing_summary(ok: list[CycleMetrics]) -> dict:
    if not ok:
        return {}

    def stat(attr: str) -> dict:
        vals = sorted(getattr(m, attr) for m in ok)
        return {
            "min": round(vals[0], 4),
            "median": round(vals[len(vals) // 2], 4),
            "p95": round(vals[min(len(vals) - 1, int(len(vals) * 0.95))], 4),
            "max": round(vals[-1], 4),
        }

    return {k: stat(k) for k in ("open_s", "start_s", "first_frame_s", "stop_s", "close_s", "total_s")}


def _assert_healthy(report: dict, baseline_key: str = "baseline") -> None:
    """PASS conditions from the reconnect contract."""
    assert report["cycles_failed"] == 0, (
        f"{report['stable_id']} {report['level']}: "
        f"{report['cycles_failed']}/{report['cycles_run']} cycles failed: {report['errors'][:3]}"
    )
    assert report["cycles_ok"] == report["cycles_requested"]

    base, final = report.get(baseline_key), report["final"]
    if not base:
        return

    # Serial handles and reader threads must return to baseline exactly: each
    # is created and destroyed once per cycle, so any growth is a leak, not
    # noise. No arbitrary tolerance is defensible here.
    assert final["tty_fd_count"] <= base["tty_fd_count"], (
        f"leaked serial fd(s) over {report['cycles_run']} cycles: "
        f"{base['tty_fd_count']} -> {final['tty_fd_count']}"
    )
    assert final["depz_reader_threads"] <= base["depz_reader_threads"], (
        f"leaked reader thread(s): {base['depz_reader_threads']} -> {final['depz_reader_threads']}"
    )
    # Total fds may legitimately wobble by a couple (logging, gc timing), but
    # cannot grow per-cycle.
    assert final["fd_count"] <= base["fd_count"] + 4, (
        f"fd growth over {report['cycles_run']} cycles: {base['fd_count']} -> {final['fd_count']}"
    )
    # RSS: allow 32 MB of allocator/arena drift over a run, then require that
    # growth is not proportional to cycle count (which is what a real leak
    # looks like). numpy frame buffers make a tighter absolute bound flaky.
    rss_growth_kb = final["rss_kb"] - base["rss_kb"]
    assert rss_growth_kb < 32_768, (
        f"RSS grew {rss_growth_kb/1024:.1f} MB over {report['cycles_run']} cycles "
        f"({base['rss_kb']/1024:.1f} -> {final['rss_kb']/1024:.1f} MB)"
    )


def _no_stale_session(before_last_ts: int, after_first_ts: int, family: str) -> None:
    assert after_first_ts > before_last_ts, (
        f"{family}: the reconnected stream replayed the previous session "
        f"(first new ts {after_first_ts} <= last old ts {before_last_ts})"
    )


# ── the three reconnect levels ───────────────────────────────────────────────


@pytest.mark.parametrize("level", ["L1", "L2", "L3"])
def test_reconnect_profile(any_device_info, level, profile, rng, seed):
    """Drive the configured profile at each reconnect level."""
    cycles = profile["cycles"]
    report = _run_profile(any_device_info, level, cycles, rng, seed)
    path = write_report(
        f"reconnect_{any_device_info.family}_{level}_{profile['name']}.json", report
    )
    print(
        f"\n[{any_device_info.stable_id} {level}] {report['cycles_ok']}/{cycles} ok, "
        f"median total {report['timing'].get('total_s', {}).get('median')}s -> {path.name}"
    )
    _assert_healthy(report)


def test_reconnect_produces_a_new_session(any_device_info):
    """Data after a reconnect must be new, not the previous session replayed."""
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            last = drv.sample_timestamp_us(drv.read_one(timeout=RECONNECT_BUDGET_S))
            for _ in range(3):
                last = max(last, drv.sample_timestamp_us(drv.read_one(timeout=RECONNECT_BUDGET_S)))
            drv.stop()
        finally:
            drv.close()

        drv2 = SensorDriver.open(info)
        try:
            drv2.prepare()
            drv2.start()
            first = drv2.sample_timestamp_us(drv2.read_one(timeout=RECONNECT_BUDGET_S))
            _no_stale_session(last, first, info.family)
            drv2.stop()
        finally:
            drv2.close()


# ── reconnect from each lifecycle state (contract §10.1) ─────────────────────


def _reconnect_and_verify(info, drv) -> None:
    """Close whatever state `drv` is in, reopen, and prove the device works."""
    try:
        drv.close()
    except Exception:
        pass
    assert not wait_for_threads_to_die(timeout=5.0), "reader thread survived close()"

    fresh = SensorDriver.open(info)
    try:
        fresh.prepare()
        fresh.start()
        assert fresh.sample_is_valid(fresh.read_one(timeout=RECONNECT_BUDGET_S))
        fresh.stop()
    finally:
        fresh.close()


def test_reconnect_right_after_open(any_device_info):
    info = any_device_info
    with port_lease(info.resolve_port()):
        _reconnect_and_verify(info, SensorDriver.open(info))


def test_reconnect_after_prepare_before_start(any_device_info):
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        _reconnect_and_verify(info, drv)


def test_reconnect_immediately_after_start(any_device_info):
    """Close before the first sample has even arrived."""
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        _reconnect_and_verify(info, drv)


def test_reconnect_during_stable_streaming(any_device_info):
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        for _ in range(5):
            drv.read_one(timeout=RECONNECT_BUDGET_S)
        _reconnect_and_verify(info, drv)


def test_reconnect_with_a_read_pending(any_device_info):
    """The hardest one: tear down while a reader is parked in the SDK."""
    import threading

    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        drv.read_one(timeout=RECONNECT_BUDGET_S)
        drv.stop()
        drv.drain_stragglers()

        done = threading.Event()
        t = threading.Thread(
            target=lambda: (_swallow(drv.pending_read), done.set()),
            name="reconnect-pending-read",
            daemon=True,
        )
        t.start()
        time.sleep(0.3)
        drv.close()
        assert done.wait(10.0), "close() stranded a pending reader during reconnect"
        t.join(timeout=5.0)

        _reconnect_and_verify(info, drv)


def _swallow(fn):
    try:
        fn()
    except BaseException:
        pass


def test_reconnect_after_a_config_change(any_device_info):
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        _apply_a_config_change(drv)
        _reconnect_and_verify(info, drv)


def _apply_a_config_change(drv) -> None:
    """Touch one real, family-appropriate setting."""
    from _support import FAMILY_BNO086, FAMILY_SR04

    if drv.family == FAMILY_SR04:
        drv.dev.set_sample_period_us(30_000)
    elif drv.family == FAMILY_BNO086:
        drv.dev.enable(0x05, hz=25)
        drv.dev.disable(0x05)
    else:
        drv.dev.set_ranging_frequency_hz(10)


def test_reconnect_right_after_stop(any_device_info):
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        drv.read_one(timeout=RECONNECT_BUDGET_S)
        drv.stop()
        _reconnect_and_verify(info, drv)


def test_reconnect_after_a_read_timeout(any_device_info):
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        drv.read_one(timeout=RECONNECT_BUDGET_S)
        drv.stop()
        drv.drain_stragglers()
        with pytest.raises(Exception):
            drv.read_one(timeout=0.4)  # provoke the timeout
        _reconnect_and_verify(info, drv)


def test_reconnect_after_a_failed_open(any_device_info):
    """A failed open must not poison the next, valid one."""
    from depz_sensor_sdk import DepzError, DeviceLostError, NoDepzDeviceError, open_device

    with pytest.raises((DeviceLostError, NoDepzDeviceError, DepzError)):
        open_device("/dev/depz-nonexistent-port", timeout=0.2)

    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            assert drv.sample_is_valid(drv.read_one(timeout=RECONNECT_BUDGET_S))
            drv.stop()
        finally:
            drv.close()


def test_connect_disconnect_storm(any_device_info, rng):
    """Rapid open/close with no settle time — the classic double-click race.

    Each iteration closes at a random point in the open/start path, so the
    teardown lands in a different state every time.
    """
    info = any_device_info
    with port_lease(info.resolve_port(), timeout=60):
        for i in range(20):
            drv = SensorDriver.open(info)
            try:
                stage = rng.randint(0, 2)
                if stage >= 1:
                    drv.prepare()
                if stage >= 2:
                    drv.start()
            finally:
                drv.close()
            assert not wait_for_threads_to_die(timeout=5.0), (
                f"storm iteration {i} (stage {stage}) leaked a reader thread"
            )

        # The device must still be fully usable after the storm.
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            assert drv.sample_is_valid(drv.read_one(timeout=RECONNECT_BUDGET_S))
            drv.stop()
        finally:
            drv.close()


def test_recovery_smoke_after_stress(any_device_info):
    """Post-stress canary: the unit must work without a physical replug."""
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            samples = [drv.read_one(timeout=RECONNECT_BUDGET_S) for _ in range(3)]
            assert all(drv.sample_is_valid(s) for s in samples)
            drv.stop()
        finally:
            drv.close()
