"""Record a fake-device session, replay it, decode identically (contract 08)."""

from pathlib import Path

from depz_sensor_sdk import Sr04
from depz_sensor_sdk.transport.record_replay import RecordingLink, ReplayLink

from fake_device import FakeSr04


def _drive(dev: Sr04) -> dict:
    out = {
        "software": dev.get_software_name(),
        "period": dev.get_sample_period_us(),
    }
    m = dev.measure_once()
    out["echo"] = m.echo_time_us
    return out


def test_record_then_replay(tmp_path: Path):
    rec_path = tmp_path / "session.depzrec"

    fake = FakeSr04()
    dev = Sr04(RecordingLink(fake.link, rec_path, header_extra={"port": "loopback"}), timeout=1.0)
    try:
        live = _drive(dev)
    finally:
        dev.close()
        fake.close()

    lines = rec_path.read_text().splitlines()
    assert lines and '"schema": "depz.rec/1"' in lines[0].replace(": ", ": ") or "depz.rec/1" in lines[0]

    # Loose replay: same decoded results from the recorded rx stream.
    replay_dev = Sr04(ReplayLink(rec_path), timeout=1.0)
    try:
        replayed = _drive(replay_dev)
    finally:
        replay_dev.close()
    assert replayed == live

    # Strict replay: our TX byte stream must be identical run-to-run.
    strict_dev = Sr04(ReplayLink(rec_path, strict_tx=True), timeout=1.0)
    try:
        assert _drive(strict_dev) == live
    finally:
        strict_dev.close()
