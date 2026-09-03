"""BNO086-specific hardware contract (contract 05).

Protocol correctness only. Physical orientation accuracy needs a reference rig
and is explicitly out of scope — see QA_MATRIX.md § Limitations. What is
asserted here is that the quaternion is *structurally* sane (finite, unit
norm), which catches decode/scaling faults without pretending to validate the
IMU itself.
"""

from __future__ import annotations

import math
import time
import warnings

import pytest

from _support import port_lease

from depz_sensor_sdk import Bno086
from depz_sensor_sdk.bno086.reports import RotationVector, SensorId

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_bno086, pytest.mark.timeout(120)]


@pytest.fixture
def imu(bno_info):
    with port_lease(bno_info.resolve_port()):
        dev = Bno086(bno_info.resolve_port())
        try:
            yield dev
        finally:
            dev.close()


def _collect(dev, sensor, n, timeout=5.0):
    it = dev.reports(sensors=sensor)
    out = []
    deadline = time.monotonic() + timeout
    while len(out) < n and time.monotonic() < deadline:
        try:
            out.append(it._queue.get(timeout=max(0.05, deadline - time.monotonic())))
        except Exception:
            break
    it.close()
    return out


def test_product_id(imu):
    pid = imu.product_id()
    assert pid.sw_version_major >= 0
    assert pid.sw_part_number > 0, "no SH-2 part number — the hub did not answer"


def test_enable_disable_rotation_vector(imu):
    resp = imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    assert resp is not None and resp.interval_us > 0
    reports = _collect(imu, SensorId.ROTATION_VECTOR, 5)
    assert len(reports) == 5, f"only {len(reports)} reports in 5s at 50Hz"
    imu.disable(SensorId.ROTATION_VECTOR)
    assert imu.get_feature(SensorId.ROTATION_VECTOR).interval_us == 0


def test_reenable_after_disable_reports_the_real_rate(imu):
    """Regression for F-03: enable() must not read a stale Get Feature Response.

    The hub emits an unsolicited Get Feature Response for every state-changing
    Set Feature, and SH-2 gives that response no correlation token — so the
    GFR(interval=0) from a preceding disable() could satisfy enable()'s verify
    read and be reported as "granted 0 Hz" for a sensor that was in fact
    streaming. It alternated with perfect 50% regularity.
    """
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    _collect(imu, SensorId.ROTATION_VECTOR, 1)

    granted = []
    spurious = []
    for _ in range(8):
        imu.disable(SensorId.ROTATION_VECTOR)
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            resp = imu.enable(SensorId.ROTATION_VECTOR, hz=50)
        granted.append(resp.interval_us)
        spurious.extend(str(w.message) for w in caught if "granted 0.0 Hz" in str(w.message))

    assert not spurious, f"enable() reported a stale 'granted 0 Hz': {spurious[:2]}"
    assert all(iv > 0 for iv in granted), f"stale interval readings: {granted}"

    # ...and the device really is streaming at the rate it just reported.
    assert len(_collect(imu, SensorId.ROTATION_VECTOR, 5)) == 5


def test_quaternion_is_structurally_valid(imu):
    """Finite, unit-norm quaternion. NOT an accuracy claim."""
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    reports = _collect(imu, SensorId.ROTATION_VECTOR, 30)
    assert len(reports) >= 20, f"only {len(reports)} reports"
    for r in reports:
        assert isinstance(r, RotationVector)
        for name, v in (("i", r.i), ("j", r.j), ("k", r.k), ("real", r.real)):
            assert math.isfinite(v), f"{name} is not finite: {v}"
            assert -1.0001 <= v <= 1.0001, f"{name} out of Q14 unit range: {v}"
        norm = math.sqrt(r.i**2 + r.j**2 + r.k**2 + r.real**2)
        # Q14 quantisation alone is ~1e-4; 2% catches a genuinely broken
        # quaternion without failing on a stationary sensor's noise.
        assert abs(norm - 1.0) < 0.02, f"quaternion norm {norm:.4f} is not unit"
    imu.disable(SensorId.ROTATION_VECTOR)


def test_timestamps_are_monotonic(imu):
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    reports = _collect(imu, SensorId.ROTATION_VECTOR, 30)
    assert len(reports) >= 20
    ts = [r.timestamp_us for r in reports]
    assert ts == sorted(ts), "report timestamps went backwards"
    assert len(set(ts)) > 1, "every report carries the same timestamp"
    imu.disable(SensorId.ROTATION_VECTOR)


def test_report_sequence_is_continuous(imu):
    """The 8-bit rolling counter must not skip — a gap means dropped samples."""
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    reports = _collect(imu, SensorId.ROTATION_VECTOR, 40)
    assert len(reports) >= 20
    gaps = [
        (a.seq, b.seq)
        for a, b in zip(reports, reports[1:])
        if b.seq != (a.seq + 1) & 0xFF
    ]
    assert not gaps, f"sequence gaps in a quiet stream: {gaps[:5]}"
    imu.disable(SensorId.ROTATION_VECTOR)


@pytest.mark.parametrize(
    "sensor",
    [
        SensorId.ROTATION_VECTOR,
        SensorId.GAME_ROTATION_VECTOR,
        SensorId.ACCELEROMETER,
        SensorId.GYROSCOPE,
        SensorId.MAGNETOMETER,
        SensorId.LINEAR_ACCELERATION,
        SensorId.GRAVITY,
    ],
)
def test_each_report_type_streams(imu, sensor):
    """Every report the SDK exposes sugar for must actually stream."""
    resp = imu.enable(sensor, hz=25)
    assert resp is not None and resp.interval_us > 0, f"0x{sensor:02X} not granted a rate"
    reports = _collect(imu, sensor, 3, timeout=4.0)
    assert len(reports) >= 3, f"0x{sensor:02X}: only {len(reports)} reports in 4s"
    assert all(r.sensor_id == sensor for r in reports), "wrong sensor_id in stream"
    imu.disable(sensor)


def test_switching_report_types_does_not_mix_streams(imu):
    """After switching, the old report type must stop and not leak through."""
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    assert _collect(imu, SensorId.ROTATION_VECTOR, 3)
    imu.disable(SensorId.ROTATION_VECTOR)

    imu.enable(SensorId.ACCELEROMETER, hz=50)
    it = imu.reports()  # unfiltered: catches anything still arriving
    time.sleep(0.1)
    seen = []
    deadline = time.monotonic() + 2.0
    while time.monotonic() < deadline and len(seen) < 30:
        try:
            seen.append(it._queue.get(timeout=0.3))
        except Exception:
            break
    it.close()
    imu.disable(SensorId.ACCELEROMETER)

    assert seen, "no reports after switching to the accelerometer"
    kinds = {r.sensor_id for r in seen}
    assert SensorId.ROTATION_VECTOR not in kinds, (
        f"rotation vector still streaming after disable(): saw {kinds}"
    )


def test_high_rate_streaming(imu):
    """400 Hz gyro-integrated RV: the SDK must keep up without gaps."""
    resp = imu.enable(SensorId.GYRO_INTEGRATED_RV, hz=400)
    assert resp is not None
    t0 = time.monotonic()
    reports = _collect(imu, SensorId.GYRO_INTEGRATED_RV, 200, timeout=5.0)
    elapsed = time.monotonic() - t0
    imu.disable(SensorId.GYRO_INTEGRATED_RV)
    assert len(reports) >= 150, f"only {len(reports)} high-rate reports in {elapsed:.1f}s"
    rate = len(reports) / elapsed
    assert rate > 100, f"effective rate {rate:.0f}/s is far below the 400Hz request"


def test_hardware_reset_recovers(imu):
    """reset() must leave the device usable and re-armable."""
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    assert _collect(imu, SensorId.ROTATION_VECTOR, 3)

    imu.hardware_reset(timeout=3.0)

    # Reset clears all feature state — reports must be re-enabled explicitly.
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    reports = _collect(imu, SensorId.ROTATION_VECTOR, 5, timeout=6.0)
    assert len(reports) == 5, f"only {len(reports)} reports after reset+re-enable"
    assert imu.product_id().sw_part_number > 0, "hub unresponsive after reset"


def test_invalid_rate_is_rejected(imu):
    with pytest.raises(ValueError):
        imu.enable(SensorId.ROTATION_VECTOR, hz=0)
    with pytest.raises(ValueError):
        imu.enable(SensorId.ROTATION_VECTOR, hz=-5)
    with pytest.raises(ValueError):
        imu.enable(SensorId.ROTATION_VECTOR)  # neither hz nor interval_us
    with pytest.raises(ValueError):
        imu.enable(SensorId.ROTATION_VECTOR, hz=50, interval_us=20000)  # both


def test_calibration_status_is_readable(imu):
    cal = imu.get_calibration()
    assert isinstance(cal.accel, bool)
    assert isinstance(cal.gyro, bool)
    assert isinstance(cal.mag, bool)


def test_accuracy_field_is_in_range(imu):
    """Rotation vector carries an accuracy estimate in radians (Q12)."""
    imu.enable(SensorId.ROTATION_VECTOR, hz=50)
    reports = _collect(imu, SensorId.ROTATION_VECTOR, 10)
    imu.disable(SensorId.ROTATION_VECTOR)
    assert reports
    for r in reports:
        assert r.accuracy_rad is not None, "0x05 must report accuracy"
        assert math.isfinite(r.accuracy_rad)
        assert 0 <= r.accuracy_rad <= 2 * math.pi, f"accuracy {r.accuracy_rad} out of range"
        assert 0 <= r.accuracy <= 3, f"accuracy status bits {r.accuracy} out of 0..3"


def test_first_command_after_reopen_latency_is_bounded(bno_info):
    """Pin ERRATA E12: the first command after a CDC reopen costs ~119 ms.

    Not a pass/fail on the device being fast — it is a *characterisation* lock.
    E12 is firmware behaviour we cannot fix from here, but it drives real
    host-side decisions (the viewer's 1000 ms timeout; probe_port's 200 ms
    default silently dropping the device). If a firmware update fixes it, or if
    it degrades past the point where the SDK's default timeout works at all,
    this test says so instead of letting the assumption rot.
    """
    import statistics

    from depz_sensor_sdk import DeviceBase

    port = bno_info.resolve_port()
    latencies = []
    with port_lease(port):
        for _ in range(8):
            dev = DeviceBase(port, timeout=3.0)
            t0 = time.monotonic()
            try:
                dev.get_device_name()
                latencies.append((time.monotonic() - t0) * 1000)
            finally:
                dev.close()

    median = statistics.median(latencies)
    print(f"\n[E12] BNO086 first-command latency: median {median:.1f} ms, max {max(latencies):.1f} ms")

    # Upper bound: past this, probe_port's 200 ms default starts dropping the
    # device silently and the SDK's story stops holding together.
    assert median < 190, (
        f"BNO086 first-command latency regressed to {median:.1f} ms — "
        "probe_port()'s 200ms default will start silently dropping the device (ERRATA E12)"
    )
    if median < 20:
        pytest.fail(
            f"BNO086 first-command latency is now {median:.1f} ms — ERRATA E12 documents "
            "~119 ms. If firmware fixed it, mark E12 resolved and drop this guard."
        )


def test_probe_finds_the_bno086_at_the_sdk_default_timeout(bno_info):
    """E12 guard: discovery must not silently lose the BNO086.

    probe_port() returns None on timeout rather than raising, so an overrun
    makes the device *disappear* from list_depz_devices() instead of erroring.
    With ~119 ms of the 200 ms default already spent, the margin is thin enough
    to be worth an explicit test.
    """
    from depz_sensor_sdk.discovery import probe_port
    from depz_sensor_sdk.protocol.identity import SensorType

    port = bno_info.resolve_port()
    for attempt in range(5):
        with port_lease(port):
            info = probe_port(port, timeout=0.2)  # the SDK default, on purpose
        assert info is not None, (
            f"attempt {attempt}: BNO086 vanished from probe_port() at the 200ms "
            "SDK default — see ERRATA E12"
        )
        assert info.sensor_type == SensorType.BNO086
        assert info.fw_version, "probe returned a device with no firmware version"
