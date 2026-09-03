"""Multiple sensors driven at once on real hardware.

The bench has four physically distinct units on four ports, so these are true
concurrency tests, not simulations. The questions they answer:

* Do four devices stream simultaneously without stealing each other's data?
* Does each sample carry the identity of the device it came from?
* Does tearing one device down disturb the others?
* Is a second opener on one port rejected rather than allowed to corrupt it?

Parallelism across *different* ports is the point here. Two tests must never
share one port — every device is taken under an exclusive lease.
"""

from __future__ import annotations

import contextlib
import threading
import time

import pytest

from _drivers import SensorDriver
from _support import (
    dump_diagnostics,
    port_lease,
    sample_resources,
    wait_for_threads_to_die,
    write_report,
)

pytestmark = [pytest.mark.hardware, pytest.mark.timeout(600)]

READ_BUDGET_S = 20.0


@contextlib.contextmanager
def _all_devices(inventory, families=None):
    """Open every attached device (or the named families) under leases."""
    wanted = [d for d in inventory if families is None or d.family in families]
    if len(wanted) < 2:
        pytest.skip(f"need >=2 devices, bench has {len(wanted)}")
    stack = contextlib.ExitStack()
    drivers = []
    try:
        for info in wanted:
            stack.enter_context(port_lease(info.resolve_port(), timeout=60))
            drv = SensorDriver.open(info)
            drivers.append(drv)
            drv.prepare()
        yield drivers
    finally:
        for drv in drivers:
            with contextlib.suppress(Exception):
                drv.close()
        stack.close()


def test_all_attached_sensors_stream_simultaneously(hw_inventory):
    """Every unit on the bench streams at once — the headline concurrency test."""
    with _all_devices(hw_inventory) as drivers:
        for drv in drivers:
            drv.start()
        try:
            samples = {}
            for drv in drivers:
                got = [drv.read_one(timeout=READ_BUDGET_S) for _ in range(3)]
                assert all(drv.sample_is_valid(s) for s in got), (
                    f"{drv.info.stable_id}: invalid sample while others were streaming"
                )
                samples[drv.info.stable_id] = got
            assert len(samples) == len(drivers)
        finally:
            for drv in drivers:
                with contextlib.suppress(Exception):
                    drv.stop()


def test_parallel_start_and_read(hw_inventory):
    """Start and read every device from its own thread, all at once.

    Sequential access would hide contention; this is the shape a real
    multi-sensor application actually uses.
    """
    with _all_devices(hw_inventory) as drivers:
        errors: dict[str, str] = {}
        counts: dict[str, int] = {}
        barrier = threading.Barrier(len(drivers))

        def _worker(drv: SensorDriver) -> None:
            try:
                barrier.wait(timeout=30)  # maximise overlap
                drv.start()
                n = 0
                for _ in range(5):
                    sample = drv.read_one(timeout=READ_BUDGET_S)
                    assert drv.sample_is_valid(sample)
                    n += 1
                counts[drv.info.stable_id] = n
                drv.stop()
            except BaseException as exc:  # noqa: BLE001 — recorded, then asserted
                errors[drv.info.stable_id] = f"{type(exc).__name__}: {exc}"

        threads = [
            threading.Thread(target=_worker, args=(d,), name=f"multi-{d.family}", daemon=True)
            for d in drivers
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=90)

        alive = [t.name for t in threads if t.is_alive()]
        if alive:
            dump_diagnostics("multi-sensor-parallel-hang", {"stuck_threads": alive})
        assert not alive, f"threads still running after 90s: {alive}"
        assert not errors, f"parallel streaming failed: {errors}"
        assert all(n == 5 for n in counts.values()), f"sample counts: {counts}"


def test_no_cross_talk_between_sensors(hw_inventory):
    """Each device's data must reach only that device's consumer.

    This is the failure F-01 produced at the port level; here it is asserted at
    the *object* level across four live streams. Every sample is tagged with the
    driver that received it, and the sample's type must match that driver's
    family — an SR04 measurement surfacing on the BNO086's stream would mean the
    reader threads are crossed.
    """
    expected_types = {
        "sr04": "Sr04Measurement",
        "vl53l8cx": "Vl53l8Frame",
        "vl53l8ch": "Vl53l8Frame",
        "bno086": "RotationVector",
    }
    with _all_devices(hw_inventory) as drivers:
        for drv in drivers:
            drv.start()
        try:
            for drv in drivers:
                for _ in range(5):
                    sample = drv.read_one(timeout=READ_BUDGET_S)
                    got = type(sample).__name__
                    want = expected_types[drv.family]
                    assert got == want, (
                        f"{drv.info.stable_id} received a {got}, expected {want} — "
                        "streams are crossed"
                    )
        finally:
            for drv in drivers:
                with contextlib.suppress(Exception):
                    drv.stop()


def test_independent_counters_and_timestamps(hw_inventory):
    """Each device keeps its own clock and stream state.

    Device timestamps come from independent MCU clocks, so they are NOT
    expected to agree; what must hold is that each device's own series is
    monotonic and does not inherit its neighbour's.
    """
    with _all_devices(hw_inventory) as drivers:
        for drv in drivers:
            drv.start()
        try:
            series = {}
            for drv in drivers:
                ts = [
                    drv.sample_timestamp_us(drv.read_one(timeout=READ_BUDGET_S))
                    for _ in range(5)
                ]
                assert ts == sorted(ts), f"{drv.info.stable_id}: timestamps went backwards"
                series[drv.info.stable_id] = ts
            # Two devices producing an identical timestamp series would mean one
            # stream is being fanned out to both consumers.
            seen = [tuple(v) for v in series.values()]
            assert len(set(seen)) == len(seen), f"two devices reported identical series: {series}"
        finally:
            for drv in drivers:
                with contextlib.suppress(Exception):
                    drv.stop()


def test_disconnect_one_while_others_stream(hw_inventory):
    """Closing one device must not disturb the rest."""
    with _all_devices(hw_inventory) as drivers:
        if len(drivers) < 2:
            pytest.skip("need >=2 devices")
        for drv in drivers:
            drv.start()
        victim, survivors = drivers[0], drivers[1:]
        try:
            for drv in drivers:
                drv.read_one(timeout=READ_BUDGET_S)

            victim.stop()
            victim.close()

            for drv in survivors:
                sample = drv.read_one(timeout=READ_BUDGET_S)
                assert drv.sample_is_valid(sample), (
                    f"{drv.info.stable_id} broke when {victim.info.stable_id} disconnected"
                )
        finally:
            for drv in survivors:
                with contextlib.suppress(Exception):
                    drv.stop()


def test_reconnect_one_while_others_stream(hw_inventory):
    """A full close/open of one unit, with its neighbours mid-stream."""
    with _all_devices(hw_inventory) as drivers:
        if len(drivers) < 2:
            pytest.skip("need >=2 devices")
        for drv in drivers:
            drv.start()
        target_info = drivers[0].info
        survivors = drivers[1:]
        try:
            for drv in drivers:
                drv.read_one(timeout=READ_BUDGET_S)

            drivers[0].stop()
            drivers[0].close()

            # The neighbours keep producing across the whole reconnect.
            for drv in survivors:
                assert drv.sample_is_valid(drv.read_one(timeout=READ_BUDGET_S))

            reborn = SensorDriver.open(target_info)
            try:
                reborn.prepare()
                reborn.start()
                assert reborn.sample_is_valid(reborn.read_one(timeout=READ_BUDGET_S))
                for drv in survivors:
                    assert drv.sample_is_valid(drv.read_one(timeout=READ_BUDGET_S)), (
                        f"{drv.info.stable_id} broke while a neighbour reconnected"
                    )
                reborn.stop()
            finally:
                reborn.close()
        finally:
            for drv in survivors:
                with contextlib.suppress(Exception):
                    drv.stop()


def test_simultaneous_stop(hw_inventory):
    """Stop everything at once from separate threads."""
    with _all_devices(hw_inventory) as drivers:
        for drv in drivers:
            drv.start()
        for drv in drivers:
            drv.read_one(timeout=READ_BUDGET_S)

        errors: dict[str, str] = {}
        barrier = threading.Barrier(len(drivers))

        def _stopper(drv: SensorDriver) -> None:
            try:
                barrier.wait(timeout=30)
                drv.stop()
            except BaseException as exc:  # noqa: BLE001
                errors[drv.info.stable_id] = f"{type(exc).__name__}: {exc}"

        threads = [threading.Thread(target=_stopper, args=(d,), daemon=True) for d in drivers]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=30)
        assert not [t for t in threads if t.is_alive()], "a simultaneous stop() hung"
        assert not errors, f"simultaneous stop failed: {errors}"


def test_slow_consumer_on_one_sensor_does_not_starve_others(hw_inventory):
    """One consumer that stops reading must not stall its neighbours.

    Stream queues are bounded drop-oldest (contract 07 §3), so the slow one
    should shed samples while everyone else keeps flowing.
    """
    with _all_devices(hw_inventory) as drivers:
        if len(drivers) < 2:
            pytest.skip("need >=2 devices")
        for drv in drivers:
            drv.start()
        slow, fast = drivers[0], drivers[1:]
        try:
            for drv in drivers:
                drv.read_one(timeout=READ_BUDGET_S)

            # `slow` deliberately reads nothing for a while.
            deadline = time.monotonic() + 2.0
            reads = {d.info.stable_id: 0 for d in fast}
            while time.monotonic() < deadline:
                for drv in fast:
                    with contextlib.suppress(Exception):
                        drv.read_one(timeout=1.0)
                        reads[drv.info.stable_id] += 1

            assert all(n > 0 for n in reads.values()), (
                f"a stalled consumer starved its neighbours: {reads}"
            )
            # And the neglected one recovers rather than being wedged.
            assert slow.sample_is_valid(slow.read_one(timeout=READ_BUDGET_S))
        finally:
            for drv in drivers:
                with contextlib.suppress(Exception):
                    drv.stop()


def test_second_opener_is_rejected(any_info):
    """Regression for F-01, at the multi-consumer level.

    While one owner streams, a second open of the same port must be refused —
    not allowed to silently split the byte stream.
    """
    from depz_sensor_sdk import DeviceLostError

    info = any_info
    with port_lease(info.resolve_port()):
        owner = SensorDriver.open(info)
        try:
            owner.prepare()
            owner.start()
            owner.read_one(timeout=READ_BUDGET_S)

            with pytest.raises(DeviceLostError):
                SensorDriver.open(info)

            # The rejected attempt must not have disturbed the owner.
            assert owner.sample_is_valid(owner.read_one(timeout=READ_BUDGET_S)), (
                "the incumbent's stream broke when a second opener was rejected"
            )
            owner.stop()
        finally:
            owner.close()


def test_sequential_open_of_every_device_leaves_no_residue(hw_inventory):
    """Open/close each unit in turn; resources must return to baseline."""
    baseline = sample_resources()
    for info in hw_inventory:
        with port_lease(info.resolve_port()):
            drv = SensorDriver.open(info)
            try:
                drv.prepare()
                drv.start()
                assert drv.sample_is_valid(drv.read_one(timeout=READ_BUDGET_S))
                drv.stop()
            finally:
                drv.close()
        assert not wait_for_threads_to_die(timeout=5.0), f"{info.stable_id} leaked a reader thread"

    final = sample_resources()
    assert final.tty_fd_count <= baseline.tty_fd_count, (
        f"serial fds leaked across the bench sweep: "
        f"{baseline.tty_fd_count} -> {final.tty_fd_count}"
    )


def test_multi_sensor_soak(hw_inventory, profile):
    """Hold every device streaming for the profile's duration.

    Records per-device throughput and the resource trend so a drift shows up as
    data rather than as a vague "it felt slow".
    """
    duration = float(profile["duration_s"])
    with _all_devices(hw_inventory) as drivers:
        for drv in drivers:
            drv.start()
        counts = {d.info.stable_id: 0 for d in drivers}
        errors: dict[str, str] = {}
        start = time.monotonic()
        baseline = None
        try:
            while time.monotonic() - start < duration:
                for drv in drivers:
                    try:
                        drv.read_one(timeout=READ_BUDGET_S)
                        counts[drv.info.stable_id] += 1
                    except Exception as exc:
                        errors[drv.info.stable_id] = f"{type(exc).__name__}: {exc}"
                        break
                if errors:
                    break
                if baseline is None and time.monotonic() - start > min(5.0, duration / 4):
                    baseline = sample_resources()
        finally:
            for drv in drivers:
                with contextlib.suppress(Exception):
                    drv.stop()

        elapsed = time.monotonic() - start
        final = sample_resources()
        report = {
            "duration_s": round(elapsed, 2),
            "profile": profile["name"],
            "devices": [d.info.stable_id for d in drivers],
            "samples": counts,
            "rates_hz": {k: round(v / elapsed, 2) for k, v in counts.items()},
            "errors": errors,
            "baseline": baseline.as_dict() if baseline else None,
            "final": final.as_dict(),
        }
        path = write_report(f"multi_sensor_soak_{profile['name']}.json", report)
        print(f"\n[multi-sensor soak {elapsed:.0f}s] rates={report['rates_hz']} -> {path.name}")

        assert not errors, f"a device failed during the soak: {errors}"
        assert all(n > 0 for n in counts.values()), f"a device produced nothing: {counts}"
        if baseline:
            assert final.tty_fd_count <= baseline.tty_fd_count, "serial fd leak during soak"
            assert final.depz_reader_threads <= baseline.depz_reader_threads, (
                "reader thread leak during soak"
            )
