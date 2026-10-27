"""BNO086 failure paths over the fake hub, complementing test_bno086.py and
test_bno086_commands.py, which only ever see the hub agree: a command
answered with a non-zero status (contract 05 §6: R0 = status), a hub that
stays silent (contract 07 §3 `timeout`, naming the report waited for), a reply
shorter than its fields (`invalid_reply`, that request only), responses for
another command on the shared 0xF1 report, an oscillator type the enum does
not know, and the everyday enable sugar the other files leave out."""

from __future__ import annotations

import pytest

from depz_sensor_sdk import Bno086, SensorId
from depz_sensor_sdk.bno086 import Sh2Error
from depz_sensor_sdk.bno086.sh2 import (ControlReport, OscillatorType, Sh2Command,
                                        build_set_feature)
from depz_sensor_sdk.bno086.shtp import ShtpChannel
from depz_sensor_sdk.errors import DepzTimeoutError, InvalidReplyError

from fake_bno086 import FakeBno086


@pytest.fixture()
def rig():
    fake = FakeBno086()
    dev = Bno086(fake.link, timeout=1.0)
    yield fake, dev
    dev.close()
    fake.close()


def _status_r0(fake, monkeypatch, status):
    """Every Command Response the fake sends carries `status` in R0."""
    real = FakeBno086._command_response

    def answer(cmd_seq, command, r, resp_seq=0):
        real(fake, cmd_seq, command, [status] + list(r[1:]), resp_seq)

    monkeypatch.setattr(fake, "_command_response", answer)


def _no_waiters(dev, rid):
    return dev._control_waiters.get(rid, []) == []


# ── a non-zero R0 status is an Sh2Error ──────────────────────────────────────

@pytest.mark.parametrize("call, what", [
    (lambda d: d.set_calibration(), "ME calibration configure failed: status 4"),
    (lambda d: d.get_calibration(), "ME calibration get failed: status 4"),
    (lambda d: d.save_dcd(), "DCD save failed: status 4"),
    (lambda d: d.clear_counts(SensorId.ACCELEROMETER), "clear counts failed: status 4"),
])
def test_a_command_answered_with_a_non_zero_status_raises_sh2error(rig, monkeypatch, call, what):
    fake, dev = rig
    _status_r0(fake, monkeypatch, 4)
    with pytest.raises(Sh2Error, match=what):
        call(dev)
    assert _no_waiters(dev, ControlReport.COMMAND_RESPONSE)


def test_an_oscillator_type_the_enum_does_not_know_comes_back_raw(rig, monkeypatch):
    fake, dev = rig
    assert dev.get_oscillator_type() is OscillatorType.EXT_CRYSTAL
    _status_r0(fake, monkeypatch, 7)          # R0 is the type itself, not a status
    osc = dev.get_oscillator_type()
    assert osc == 7 and not isinstance(osc, OscillatorType)


def test_metadata_for_a_sensor_without_a_record_is_refused_before_the_wire(rig):
    fake, dev = rig
    before = fake.send_shtp_attempts
    with pytest.raises(Sh2Error, match="no metadata FRS record known for sensor 0x0A"):
        dev.get_metadata(0x0A)  # pressure: not on this hub, no record
    assert fake.send_shtp_attempts == before


# ── a silent hub is a timeout naming the report waited for ───────────────────

def test_get_feature_times_out_when_the_hub_never_answers(rig, monkeypatch):
    fake, dev = rig
    monkeypatch.setattr(fake, "_send_feature_response", lambda sensor: None)
    with pytest.raises(DepzTimeoutError) as e:
        dev.get_feature(SensorId.ACCELEROMETER, timeout=0.1)
    assert e.value.cmd == ControlReport.GET_FEATURE_RESPONSE
    assert _no_waiters(dev, ControlReport.GET_FEATURE_RESPONSE)


def test_get_feature_ignores_a_response_for_another_sensor(rig, monkeypatch):
    # SH-2 gives Get Feature Response no token but the sensor id (contract 05
    # §6): an answer about the gyroscope does not satisfy a question about the
    # accelerometer.
    fake, dev = rig
    real = FakeBno086._send_feature_response
    monkeypatch.setattr(fake, "_send_feature_response",
                        lambda sensor: real(fake, SensorId.GYROSCOPE))
    with pytest.raises(DepzTimeoutError):
        dev.get_feature(SensorId.ACCELEROMETER, timeout=0.15)


def test_product_ids_times_out_when_no_component_answers(rig, monkeypatch):
    fake, dev = rig
    real = FakeBno086._on_shtp
    monkeypatch.setattr(fake, "_on_shtp",
                        lambda frame: None if frame[4:5] == b"\xf9" else real(fake, frame))
    with pytest.raises(DepzTimeoutError) as e:
        dev.product_ids(timeout=0.1)
    assert e.value.cmd == ControlReport.PRODUCT_ID_RESPONSE
    assert _no_waiters(dev, ControlReport.PRODUCT_ID_RESPONSE)


def test_get_errors_times_out_without_the_terminating_record(rig, monkeypatch):
    fake, dev = rig
    monkeypatch.setattr(fake, "_command_response", lambda *a, **k: None)
    with pytest.raises(DepzTimeoutError) as e:
        dev.get_errors(timeout=0.1)
    assert e.value.cmd == ControlReport.COMMAND_RESPONSE
    assert _no_waiters(dev, ControlReport.COMMAND_RESPONSE)


def test_frs_read_and_write_time_out_on_a_silent_hub(rig, monkeypatch):
    fake, dev = rig
    monkeypatch.setattr(fake, "_frs_read_response", lambda *a, **k: None)
    monkeypatch.setattr(fake, "_frs_write_response", lambda *a, **k: None)
    with pytest.raises(DepzTimeoutError) as e:
        dev.frs_read(0x2D3E, timeout=0.1)
    assert e.value.cmd == ControlReport.FRS_READ_RESPONSE
    with pytest.raises(DepzTimeoutError) as e:
        dev.frs_write(0x2D3E, [1, 2, 3, 4], timeout=0.1)
    assert e.value.cmd == ControlReport.FRS_WRITE_RESPONSE
    assert _no_waiters(dev, ControlReport.FRS_READ_RESPONSE)
    assert _no_waiters(dev, ControlReport.FRS_WRITE_RESPONSE)


def test_clear_dcd_and_reset_requires_the_reset_complete(rig, monkeypatch):
    # Unlike SENSOR_RESET (§8, best effort), command 0x0B has no other answer:
    # without the executable reset-complete it is a timeout.
    fake, dev = rig
    real = FakeBno086.push_shtp_cargo

    def no_reset_complete(channel, payload, **kw):
        if channel != ShtpChannel.EXECUTABLE:
            real(fake, channel, payload, **kw)

    monkeypatch.setattr(fake, "push_shtp_cargo", no_reset_complete)
    with pytest.raises(DepzTimeoutError) as e:
        dev.clear_dcd_and_reset(timeout=0.2)
    assert e.value.cmd == Sh2Command.CLEAR_DCD_AND_RESET


# ── replies that do not decode, or are not ours ──────────────────────────────

def test_a_short_product_id_response_fails_that_request_only(rig, monkeypatch):
    fake, dev = rig
    real = FakeBno086._on_shtp

    def short_answer(frame):
        if frame[4:5] == b"\xf9":   # SHTP header 4 B, then the report id
            fake.push_shtp_cargo(ShtpChannel.CONTROL, bytes([0xF8, 1, 3, 8]))  # 4 of 16 B
        else:
            real(fake, frame)

    monkeypatch.setattr(fake, "_on_shtp", short_answer)
    with pytest.raises(InvalidReplyError) as e:
        dev.product_ids(timeout=0.5)
    assert e.value.cmd == ControlReport.PRODUCT_ID_RESPONSE
    monkeypatch.undo()
    assert dev.product_id().sw_part_number == 10003608  # the next one is answered


def test_a_short_command_response_fails_get_errors(rig, monkeypatch):
    fake, dev = rig
    monkeypatch.setattr(
        fake, "_command_response",
        lambda cmd_seq, command, r, resp_seq=0: fake.push_shtp_cargo(
            ShtpChannel.CONTROL, bytes((0xF1, 0, command, cmd_seq, resp_seq))))
    with pytest.raises(InvalidReplyError) as e:
        dev.get_errors()
    assert e.value.cmd == ControlReport.COMMAND_RESPONSE


def test_command_responses_for_another_request_are_skipped(rig, monkeypatch):
    # Errors and Counts share report 0xF1; only (command, commandSeq) says
    # whose a response is (contract 05 §6).
    fake, dev = rig
    real = FakeBno086._command_response

    def with_strangers(cmd_seq, command, r, resp_seq=0):
        real(fake, (cmd_seq + 1) & 0xFF, command, [0, 0, 255], resp_seq)   # stale seq
        real(fake, cmd_seq, Sh2Command.COUNTER, [0, 0, 255], resp_seq)     # other command
        real(fake, cmd_seq, command, r, resp_seq)

    monkeypatch.setattr(fake, "_command_response", with_strangers)
    records = dev.get_errors()
    assert [(r.severity, r.seq) for r in records] == [(1, 0), (2, 1)]


# ── sugar ────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("method, sensor, interval_us", [
    ("enable_game_rotation_vector", SensorId.GAME_ROTATION_VECTOR, 10_000),
    ("enable_magnetometer", SensorId.MAGNETOMETER, 20_000),
    ("enable_linear_acceleration", SensorId.LINEAR_ACCELERATION, 10_000),
    ("enable_gravity", SensorId.GRAVITY, 10_000),
    ("enable_gyro_integrated_rv", SensorId.GYRO_INTEGRATED_RV, 2_500),
])
def test_enable_sugar_uses_the_sensor_and_its_default_rate(rig, method, sensor, interval_us):
    fake, dev = rig
    resp = getattr(dev, method)()
    assert fake.last_set_feature == build_set_feature(sensor, interval_us)
    assert resp is not None and resp.sensor_id == sensor and resp.interval_us == interval_us

