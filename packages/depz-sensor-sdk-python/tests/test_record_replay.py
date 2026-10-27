"""Record a fake-device session, replay it, decode identically (contract 08)."""

import gzip
import json
import time
from pathlib import Path

import pytest

from depz_sensor_sdk import Sr04
from depz_sensor_sdk.errors import LinkClosedError, ReplayMismatchError
from depz_sensor_sdk.transport.link import LoopbackLink
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


def _capture(path: Path, events: list[tuple[int, str, bytes]]) -> Path:
    """A hand-written capture: (t_us, dir, data) lines under a schema header."""
    lines = [json.dumps({"schema": "depz.rec/1"})]
    for t, d, data in events:
        lines.append(json.dumps({"t": t, "dir": d, "data": data.hex()}))
    path.write_text("\n".join(lines) + "\n\n", encoding="utf-8")  # trailing blank line
    return path


def test_recording_link_tracks_the_inner_link(tmp_path: Path):
    a, _b = LoopbackLink.pair()
    rec = RecordingLink(a, tmp_path / "x.depzrec")
    assert not rec.closed
    rec.close()
    assert rec.closed and a.closed
    assert json.loads((tmp_path / "x.depzrec").read_text().splitlines()[0])["schema"] == "depz.rec/1"


def test_replay_gates_rx_on_tx_and_skips_blank_lines(tmp_path: Path):
    path = _capture(tmp_path / "c.depzrec", [
        (10, "tx", b"\x01\x02"),
        (20, "rx", b"\xaa"),
        (30, "tx", b"\x03"),
        (40, "rx", b"\xbb"),
    ])
    link = ReplayLink(path)
    assert link.header["schema"] == "depz.rec/1"
    assert link.read(0.01) == b""  # nothing written yet: the first rx waits
    link.write(b"\x01")
    assert link.read(0.01) == b""  # one of the two preceding tx bytes
    link.write(b"\x02")
    assert link.read(0.01) == b"\xaa"
    assert not link.exhausted
    link.write(b"\x03")
    assert link.read(0.01) == b"\xbb"
    assert link.exhausted and link.read(0.01) == b""
    link.close()
    assert link.closed
    with pytest.raises(LinkClosedError):
        link.read(0.01)
    with pytest.raises(LinkClosedError):
        link.write(b"\x00")


def test_replay_strict_tx_rejects_a_diverging_host(tmp_path: Path):
    path = _capture(tmp_path / "c.depzrec", [(10, "tx", b"\x01\x02"), (20, "rx", b"\xaa")])
    link = ReplayLink(path, strict_tx=True)
    link.write(b"\x01")
    with pytest.raises(ReplayMismatchError, match="offset 1"):  # contract 08, ERRATA A14
        link.write(b"\x09")
    link.write(b"\x02")
    assert link.read(0.01) == b"\xaa"


# `{"schema": "depz.rec/1"}` and one rx event `aa`, gzipped (mtime 0): the
# same 70 bytes in every SDK's test
_GZIPPED_CAPTURE = bytes.fromhex(
    "1f8b0800000000000203ab562a4ece48cd4d54b252504a492da8d22b4a4dd63754aae5aa"
    "562a018a19e80085338b40b245154a204e6209586d6222500d007e3bc00b3d000000"
)


def test_replay_refuses_a_gzipped_capture(tmp_path: Path):
    # ERRATA A6: there is no `.depzrec.gz` — a capture is plain text, a gzip
    # container is not inflated but refused like any damaged file
    plain = tmp_path / "c.depzrec"
    plain.write_bytes(gzip.decompress(_GZIPPED_CAPTURE))
    link = ReplayLink(plain)  # the content itself is a valid capture
    assert link.read(0.01) == b"\xaa"
    link.close()
    for name in ("c.depzrec", "c.depzrec.gz"):
        path = tmp_path / name
        path.write_bytes(_GZIPPED_CAPTURE)
        with pytest.raises(ValueError):
            ReplayLink(path)


def test_replay_realtime_paces_rx_by_recorded_time(tmp_path: Path):
    path = _capture(tmp_path / "c.depzrec", [
        (0, "rx", b"\x00"),          # already due: no wait
        (2_000, "rx", b"\xaa"),      # 2 ms in
        (5_000_000, "rx", b"\xbb"),  # 5 s in: far beyond any read budget here
    ])
    link = ReplayLink(path, realtime=True)
    t0 = time.monotonic()
    assert link.read(1.0) == b"\x00"
    assert link.read(1.0) == b"\xaa"
    assert time.monotonic() - t0 >= 0.0015
    # the second event is not due within the read's budget: b"" and nothing
    # is consumed — a later read with budget would still get it
    t0 = time.monotonic()
    assert link.read(0.01) == b""
    assert time.monotonic() - t0 < 0.5
    assert not link.exhausted
    link.close()
