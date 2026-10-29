"""`depz-sensor` command-line tool (cli.py) on in-process fakes.

The CLI only ever names a port; `DeviceBase` and `_open_for_streaming` turn
that name into a `SerialLink`. The `rig` fixture swaps `SerialLink` for a
factory that hands out the loopback side of a fresh fake firmware per open,
so every open path runs the real wire protocol: the probe connection, then
the resolved class's own connection (contract 02 §4.4), or the single
recorded connection of `record` (contract 08). Ctrl+C is modelled by a
`print` replacement that raises KeyboardInterrupt at a chosen line.
"""

from __future__ import annotations

import json
import runpy
import sys
import threading
import time
from pathlib import Path
from types import SimpleNamespace

import pytest

from depz_sensor_sdk import __version__, bootloader, cli, device, discovery
from depz_sensor_sdk.dataset import DatasetReader, DatasetWriter
from depz_sensor_sdk.errors import DeviceLostError
from depz_sensor_sdk.sr04 import Sr04
from depz_sensor_sdk.transport import serial_link
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.usb_ids import DEPZ_USB_VID, PID_SR04, PID_VL53L4CD, PID_VL53L8CH, PID_VL53L8CX
from depz_sensor_sdk.vl53l4 import Vl53l4cd
from depz_sensor_sdk.vl53l4.uld import parse_result_block

from fake_device import FakeSr04
from fake_vl53l4 import FakeVl53l4cd
from fake_vl53l8 import FakeVl53l8


class FakeGeneric(FakeSr04):
    """An application the SDK has no class for: resolves to DeviceBase."""

    SOFTWARE_NAME = "APP_widget_v0.3"
    DEVICE_NAME = "DEPZ Widget"
    SERIAL = "SN0099"


class Rig:
    """Fake serial ports: `add(port, factory, pid, usb_serial)` registers a
    firmware per port; each open of that port gets a fresh instance."""

    def __init__(self):
        self.factories: dict[str, tuple] = {}
        self.opened: dict[str, list] = {}
        self.lines: list[str] = []  # stdout
        self.err: list[str] = []  # file=sys.stderr
        self.interrupt_on = None  # predicate(line) -> raise KeyboardInterrupt after it, once

    def add(self, port: str, factory, *, pid: int = PID_SR04, usb_serial: str | None = None):
        self.factories[port] = (factory, pid, usb_serial)
        self.opened.setdefault(port, [])

    def open_link(self, port: str, *_args, **_kwargs):
        if port not in self.factories:
            raise DeviceLostError(f"cannot open {port}: no such port")
        fake = self.factories[port][0]()
        self.opened[port].append(fake)
        return fake.link

    def comports(self):
        return [
            SimpleNamespace(device=p, vid=DEPZ_USB_VID, pid=pid, serial_number=sn)
            for p, (_f, pid, sn) in self.factories.items()
        ]

    def print(self, *args, **kwargs):
        line = " ".join(str(a) for a in args)
        (self.err if kwargs.get("file") is sys.stderr else self.lines).append(line)
        if self.interrupt_on is not None and self.interrupt_on(line):
            self.interrupt_on = None  # one Ctrl+C
            raise KeyboardInterrupt

    def interrupt_at(self, substring: str, count: int = 1):
        """Raise KeyboardInterrupt after the `count`-th line containing `substring`."""
        seen = [0]

        def pred(line: str) -> bool:
            if substring in line:
                seen[0] += 1
            return seen[0] >= count

        self.interrupt_on = pred

    def close_all(self):
        for fakes in self.opened.values():
            for f in fakes:
                f.close()


@pytest.fixture()
def rig(monkeypatch):
    r = Rig()
    monkeypatch.setattr(device, "SerialLink", r.open_link)
    monkeypatch.setattr(serial_link, "SerialLink", r.open_link)
    monkeypatch.setattr(discovery.list_ports, "comports", r.comports)
    monkeypatch.setattr(cli, "print", r.print, raising=False)
    yield r
    r.close_all()


def _wait(pred, timeout: float = 2.0) -> None:
    deadline = time.monotonic() + timeout
    while not pred():
        assert time.monotonic() < deadline, "condition not met in time"
        time.sleep(0.005)


def _last(rig: Rig, port: str):
    return rig.opened[port][-1]


# ── argument parsing / dispatch ──────────────────────────────────────────────


def test_version_prints_sdk_version(capsys):
    assert cli.main(["--version"]) == 0
    assert capsys.readouterr().out.strip() == __version__


def test_no_command_prints_help(capsys):
    assert cli.main([]) == 0
    assert capsys.readouterr().out.startswith("usage: depz-sensor")


@pytest.mark.filterwarnings("ignore:.*found in sys.modules.*:RuntimeWarning")  # runpy on an imported module
def test_python_m_entry_point_exits_with_main_result(monkeypatch, capsys):
    monkeypatch.setattr(sys, "argv", ["depz-sensor", "--version"])
    with pytest.raises(SystemExit) as exc:
        runpy.run_module("depz_sensor_sdk.cli", run_name="__main__", alter_sys=True)
    assert exc.value.code == 0
    assert capsys.readouterr().out.strip() == __version__


@pytest.mark.parametrize(
    "argv, func, expected",
    [
        (["list"], "cmd_list", {"timeout": 0.2}),
        (["list", "--timeout", "1.5"], "cmd_list", {"timeout": 1.5}),
        (["info", "/dev/x"], "cmd_info", {"port": "/dev/x", "timeout": 0.2}),
        (["monitor", "/dev/x"], "cmd_monitor", {"port": "/dev/x", "record": None}),
        (["monitor", "/dev/x", "--record", "a.depzrec"], "cmd_monitor", {"record": "a.depzrec"}),
        (["record", "/dev/x", "--out", "a.depzrec"], "cmd_record", {"out": "a.depzrec"}),
        (["record-data", "/dev/a", "/dev/b", "--out", "s.depzdata"], "cmd_record_data",
         {"ports": ["/dev/a", "/dev/b"], "out": "s.depzdata", "seconds": 10.0,
          "tof_hz": 15, "layers": False}),
        (["record-data", "/dev/a", "--out", "s", "--seconds", "2", "--tof-hz", "30", "--layers"],
         "cmd_record_data", {"seconds": 2.0, "tof_hz": 30, "layers": True}),
        (["play-data", "s.depzdata"], "cmd_play_data", {"path": "s.depzdata", "speed": 1.0}),
        (["play-data", "s", "--speed", "0"], "cmd_play_data", {"speed": 0.0}),
        (["flash", "/dev/x", "fw.fwdepz"], "cmd_flash", {"port": "/dev/x", "fwdepz": "fw.fwdepz"}),
    ],
)
def test_subcommands_dispatch_with_defaults(monkeypatch, argv, func, expected):
    got = {}

    def spy(args):
        got.update(vars(args))
        return 7

    monkeypatch.setattr(cli, func, spy)
    assert cli.main(argv) == 7
    for key, value in expected.items():
        assert got[key] == value, key


def test_record_requires_out(capsys):
    with pytest.raises(SystemExit) as exc:
        cli.main(["record", "/dev/x"])
    assert exc.value.code == 2
    assert "--out" in capsys.readouterr().err


@pytest.mark.skipif(sys.platform == "win32", reason="port aliases are POSIX")
def test_record_resolves_the_usb_id_of_an_aliased_port(rig, monkeypatch, tmp_path):
    """`--record` opens one connection and probes on it (`open_on_link`), so
    the USB id must come from the port the alias names (02 §4.3, ERRATA A5)."""
    node = tmp_path / "ttyACM0"
    node.write_bytes(b"")
    alias = tmp_path / "usb-DEPZ_ToF-if00"
    alias.symlink_to(node)
    rig.add(str(alias), FakeVl53l8, pid=PID_VL53L8CH)
    monkeypatch.setattr(
        discovery.list_ports, "comports",
        lambda: [SimpleNamespace(device=str(node), vid=DEPZ_USB_VID, pid=PID_VL53L8CH, serial_number="S")],
    )
    seen = {}

    def spy_open_on_link(link, vid=None, pid=None, *, timeout):
        seen.update(vid=vid, pid=pid)
        link.close()
        return SimpleNamespace(close=lambda: None)

    monkeypatch.setattr(discovery, "open_on_link", spy_open_on_link)
    cli._open_for_streaming(str(alias), str(tmp_path / "a.depzrec"))
    assert seen == {"vid": DEPZ_USB_VID, "pid": PID_VL53L8CH}


# ── list / info ──────────────────────────────────────────────────────────────


def test_list_one_line_per_device_in_usb_serial_order(rig):
    rig.add("/dev/fake1", FakeSr04, usb_serial="B2")
    rig.add("/dev/fake0", FakeGeneric, pid=PID_VL53L4CD, usb_serial="A1")
    assert cli.main(["list"]) == 0
    assert len(rig.lines) == 2
    assert rig.lines[0].split("\t")[0] == "/dev/fake0"  # smallest USB serial first
    port, mode, sensor, software, name, serial, usb = rig.lines[1].split("\t")
    assert (port, mode, sensor, software) == ("/dev/fake1", "app", "sr04", FakeSr04.SOFTWARE_NAME)
    assert name == f"name={FakeSr04.DEVICE_NAME!r}"
    assert serial == f"serial={FakeSr04.SERIAL!r}"
    assert usb == f"usb={DEPZ_USB_VID:04x}:{PID_SR04:04x}"
    # an application the SDK has no class for still lists (SensorType.UNKNOWN)
    assert rig.lines[0].split("\t")[2] == "unknown"
    # list probes and closes: no connection left behind
    for fakes in rig.opened.values():
        assert all(f.link.closed for f in fakes)


def test_list_without_devices_fails(rig):
    assert cli.main(["list"]) == 1
    assert rig.lines == ["no DEPZ devices found"]


def test_info_prints_probe_fields(rig):
    rig.add("/dev/fake0", FakeSr04)
    assert cli.main(["info", "/dev/fake0"]) == 0
    assert rig.lines == [
        "port: /dev/fake0",
        "mode: app",
        "sensor_type: SensorType.SR04",
        f"software_name: {FakeSr04.SOFTWARE_NAME}",
        "fw_version: 0.95",
        f"device_name: {FakeSr04.DEVICE_NAME}",
        f"serial_number: {FakeSr04.SERIAL}",
    ]


def test_info_silent_port_fails(rig):
    class Silent:
        def __init__(self):
            self.link, self._other = LoopbackLink.pair()

        def close(self):
            self.link.close()

    rig.add("/dev/mute", Silent)
    assert cli.main(["info", "/dev/mute", "--timeout", "0.05"]) == 1
    assert rig.err == ["no DEPZ device answered on /dev/mute"]
    assert rig.lines == []


def test_info_missing_port_fails(rig):
    assert cli.main(["info", "/dev/nope"]) == 1
    assert rig.err == ["no DEPZ device answered on /dev/nope"]


# ── monitor ──────────────────────────────────────────────────────────────────


def _feed_sr04(rig: Rig, port: str, echoes: list[int]) -> threading.Thread:
    """Once the streaming connection's loop is started, emit `echoes`."""

    def run():
        _wait(lambda: len(rig.opened[port]) == 2 and rig.opened[port][1].loop_running)
        for echo in echoes:
            rig.opened[port][1].send_measurement(0x37, echo)
            time.sleep(0.01)

    t = threading.Thread(target=run, daemon=True)
    t.start()
    return t


def test_monitor_sr04_until_ctrl_c(rig):
    rig.add("/dev/fake0", FakeSr04)
    rig.interrupt_at(" us  ", count=3)  # measurement lines
    feeder = _feed_sr04(rig, "/dev/fake0", [5831, 0xFFFF, 2916])
    assert cli.main(["monitor", "/dev/fake0"]) == 0
    feeder.join(2.0)

    assert rig.lines[0] == f"connected: {FakeSr04.SOFTWARE_NAME} (Ctrl+C to stop)"
    rows = [l for l in rig.lines if l.endswith("[loop]")]
    assert len(rows) == 3
    assert rows[0].split()[2:4] == ["5831", "us"]
    assert rows[0].endswith("mm  [loop]")  # a distance
    assert "no echo" in rows[1]  # 0xFFFF = no echo (contract 03)
    assert float(rows[2].split()[4]) == pytest.approx(500.1, abs=0.1)  # 2916 µs ≈ 0.5 m

    probe, stream = rig.opened["/dev/fake0"]  # probe connection, then the Sr04's own
    assert probe.link.closed and stream.link.closed
    assert stream.loop_running is False  # stopped on the way out


def test_monitor_ends_when_the_device_vanishes(rig):
    rig.add("/dev/fake0", FakeSr04)

    def unplug():
        _wait(lambda: len(rig.opened["/dev/fake0"]) == 2 and _last(rig, "/dev/fake0").loop_running)
        _last(rig, "/dev/fake0").send_measurement(0x37)
        _wait(lambda: any(l.endswith("[loop]") for l in rig.lines))
        _last(rig, "/dev/fake0").close()

    threading.Thread(target=unplug, daemon=True).start()
    assert cli.main(["monitor", "/dev/fake0"]) == 0
    assert sum(l.endswith("[loop]") for l in rig.lines) == 1
    assert any(l.startswith("[event] DisconnectedEvent") for l in rig.lines)


def test_monitor_vl53l4cd_until_ctrl_c(rig):
    rig.add("/dev/tof", FakeVl53l4cd, pid=PID_VL53L4CD)
    rig.interrupt_at("sigma", count=2)
    blocks: list[bytes] = []

    def feed():
        _wait(lambda: len(rig.opened["/dev/tof"]) == 2 and _last(rig, "/dev/tof").streaming)
        blocks.append(_last(rig, "/dev/tof").send_result(1234, sigma_mm=3, signal_kcps=1600))
        time.sleep(0.01)
        blocks.append(_last(rig, "/dev/tof").send_result(77, status=4, sigma_mm=10))

    feeder = threading.Thread(target=feed, daemon=True)
    feeder.start()
    assert cli.main(["monitor", "/dev/tof"]) == 0
    feeder.join(2.0)

    assert rig.lines[0] == f"connected: {FakeVl53l4cd.SOFTWARE_NAME} (Ctrl+C to stop)"
    rows = [l for l in rig.lines if "sigma" in l]
    assert len(rows) == 2
    for row, raw in zip(rows, blocks):
        r = parse_result_block(raw)
        assert f" {r.distance_mm:>5} mm" in row
        assert f"sigma {r.sigma_mm:>3}" in row
        assert f"signal {r.signal_rate_kcps:>6} kcps" in row
        assert f"status {r.range_status} (" in row
    fake = _last(rig, "/dev/tof")
    assert fake.streaming is False  # STOP_STREAM sent
    assert fake.platform.written_to(0x0087)[-1] == b"\x80"  # SYSTEM_START: stop ranging
    assert fake.link.closed


def test_monitor_vl53l4cd_ends_when_the_device_vanishes(rig):
    rig.add("/dev/tof", FakeVl53l4cd, pid=PID_VL53L4CD)

    def unplug():
        _wait(lambda: len(rig.opened["/dev/tof"]) == 2 and _last(rig, "/dev/tof").streaming)
        _last(rig, "/dev/tof").send_result(500)
        _wait(lambda: any("sigma" in l for l in rig.lines))
        _last(rig, "/dev/tof").close()

    threading.Thread(target=unplug, daemon=True).start()
    assert cli.main(["monitor", "/dev/tof"]) == 0
    assert sum("sigma" in l for l in rig.lines) == 1
    assert any(l.startswith("[event] DisconnectedEvent") for l in rig.lines)


def test_monitor_generic_device_prints_temperature(rig, monkeypatch):
    rig.add("/dev/gen", FakeGeneric)
    pauses: list[float] = []

    def sleep(seconds: float) -> None:  # Ctrl+C lands in the second pause
        pauses.append(seconds)
        if len(pauses) == 2:
            raise KeyboardInterrupt
        _last(rig, "/dev/gen").temperature_raw = -45  # -4.5 °C next time

    monkeypatch.setattr(cli, "time", SimpleNamespace(sleep=sleep))
    assert cli.main(["monitor", "/dev/gen"]) == 0
    assert rig.lines == [
        f"connected: {FakeGeneric.SOFTWARE_NAME} (Ctrl+C to stop)",
        "generic device: printing events; temperature every 2 s",
        "MCU temperature: 27.3 C",
        "MCU temperature: -4.5 C",
    ]
    assert pauses == [2, 2]
    assert _last(rig, "/dev/gen").link.closed


# ── record (.depzrec, contract 08) ───────────────────────────────────────────


def test_record_keeps_probe_and_stream_on_one_connection(rig, tmp_path: Path):
    """`record` opens the port once: the recording holds the probe too
    (contract 02 §4.4), so a replay resolves the class and streams."""
    out = tmp_path / "cap.depzrec"
    rig.add("/dev/fake0", FakeSr04)
    rig.interrupt_at("[loop]", count=2)

    def feed():
        _wait(lambda: rig.opened["/dev/fake0"] and _last(rig, "/dev/fake0").loop_running)
        _last(rig, "/dev/fake0").send_measurement(0x37, 5831)
        time.sleep(0.01)
        _last(rig, "/dev/fake0").send_measurement(0x37, 1458)

    feeder = threading.Thread(target=feed, daemon=True)
    feeder.start()
    assert cli.main(["record", "/dev/fake0", "--out", str(out)]) == 0
    feeder.join(2.0)
    assert len(rig.opened["/dev/fake0"]) == 1  # one connection, not probe + reopen

    header = json.loads(out.read_text(encoding="utf-8").splitlines()[0])
    assert header["port"] == "/dev/fake0"

    dev = discovery.open_on_link(ReplayLink(out, strict_tx=True), DEPZ_USB_VID, PID_SR04)
    try:
        assert isinstance(dev, Sr04)
        assert dev.get_software_name() == FakeSr04.SOFTWARE_NAME
        it = dev.stream()
        dev.start()
        assert [next(it).echo_time_us for _ in range(2)] == [5831, 1458]
        dev.stop()
    finally:
        dev.close()


def test_monitor_record_flag_is_record(rig, tmp_path: Path):
    out = tmp_path / "cap.depzrec"
    rig.add("/dev/fake0", FakeSr04)
    rig.interrupt_at("connected:")
    assert cli.main(["monitor", "/dev/fake0", "--record", str(out)]) == 0
    assert json.loads(out.read_text(encoding="utf-8").splitlines()[0])["port"] == "/dev/fake0"
    assert len(rig.opened["/dev/fake0"]) == 1


# ── record-data / play-data (.depzdata, contract 09) ─────────────────────────


def test_record_data_two_sensors_onto_one_timeline(rig, tmp_path: Path):
    out = tmp_path / "s.depzdata"
    rig.add("/dev/a", FakeSr04, usb_serial="A")
    rig.add("/dev/tof", FakeVl53l4cd, pid=PID_VL53L4CD, usb_serial="B")

    def feed():
        _wait(lambda: len(rig.opened["/dev/a"]) == 2 and _last(rig, "/dev/a").loop_running)
        _wait(lambda: len(rig.opened["/dev/tof"]) == 2 and _last(rig, "/dev/tof").streaming)
        for i in range(3):
            _last(rig, "/dev/a").send_measurement(0x37, 5000 + i)
            _last(rig, "/dev/tof").send_result(300 + i, stream_count=i)
            time.sleep(0.01)

    feeder = threading.Thread(target=feed, daemon=True)
    feeder.start()
    assert cli.main(["record-data", "/dev/a", "/dev/tof", "--out", str(out), "--seconds", "0.3"]) == 0
    feeder.join(2.0)

    assert rig.lines[0] == f"d0: {FakeSr04.SOFTWARE_NAME} @ /dev/a"
    assert rig.lines[1] == f"d1: {FakeVl53l4cd.SOFTWARE_NAME} @ /dev/tof"
    assert rig.lines[2] == f"recording to {out} for 0s (Ctrl+C to stop early)"
    assert rig.lines[-1] == f"wrote {out}"
    assert _last(rig, "/dev/a").loop_running is False
    assert _last(rig, "/dev/tof").streaming is False
    assert _last(rig, "/dev/a").link.closed and _last(rig, "/dev/tof").link.closed

    reader = DatasetReader(out)
    assert {k: v["sensor_type"] for k, v in reader.devices.items()} == {"d0": "sr04", "d1": "vl53l4"}
    by_kind: dict[str, list] = {}
    for rec in reader:
        by_kind.setdefault(rec.kind, []).append(rec)
    assert [r.value["echo_us"] for r in by_kind["sr04"]] == [5000, 5001, 5002]
    assert [r.value["distance_mm"] for r in by_kind["vl53l4"]] == [300, 301, 302]
    assert all(r.device_id == "d0" for r in by_kind["sr04"])
    assert all(r.device_id == "d1" for r in by_kind["vl53l4"])
    ts = [r.t_host_us for r in reader]
    assert ts == sorted(ts)


def test_record_data_ctrl_c_still_writes_the_file(rig, tmp_path: Path):
    out = tmp_path / "s.depzdata.gz"
    rig.add("/dev/a", FakeSr04)
    rig.interrupt_at("recording to")
    assert cli.main(["record-data", "/dev/a", "--out", str(out), "--seconds", "60"]) == 0
    assert rig.lines[-1] == f"wrote {out}"
    assert _last(rig, "/dev/a").loop_running is False
    assert list(DatasetReader(out).devices) == ["d0"]


def test_record_data_survives_a_vanished_device(rig, tmp_path: Path):
    """An unplugged sensor must not lose the session: the file is still
    written and the other devices are stopped and closed."""
    out = tmp_path / "s.depzdata"
    rig.add("/dev/a", FakeSr04, usb_serial="A")
    rig.add("/dev/gen", FakeGeneric, usb_serial="G")

    def unplug():
        _wait(lambda: len(rig.opened["/dev/a"]) == 2 and _last(rig, "/dev/a").loop_running)
        _last(rig, "/dev/a").send_measurement(0x37, 4321)
        time.sleep(0.05)
        _last(rig, "/dev/a").close()

    threading.Thread(target=unplug, daemon=True).start()
    assert cli.main(["record-data", "/dev/a", "/dev/gen", "--out", str(out), "--seconds", "0.3"]) == 0
    assert rig.lines[-1] == f"wrote {out}"
    assert _last(rig, "/dev/gen").link.closed

    reader = DatasetReader(out)
    assert {k: v["sensor_type"] for k, v in reader.devices.items()} == {"d0": "sr04", "d1": "unknown"}
    assert [r.value["echo_us"] for r in reader if r.kind == "sr04"] == [4321]


def _dataset(path: Path) -> Path:
    w = DatasetWriter(path, {
        "d0": {"sensor_type": "sr04", "time_sync": {"offset_us": 0, "rtt_us": 1}},
        "d1": {"sensor_type": "vl53l8", "time_sync": {"offset_us": 0, "rtt_us": 1}},
        "d2": {"sensor_type": "bno086", "time_sync": {"offset_us": 0, "rtt_us": 1}},
    })
    w.write("d0", 1_000, "sr04", {"echo_us": 5831, "source": "loop"})
    w.write("d1", 2_000, "vl53l8", {"resolution": 16, "distance_mm": [400, 120, 333, 250] * 4,
                                     "target_status": [5] * 16})
    w.write("d2", 3_000, "temperature", {"mcu_c": 31.5})
    w.close()
    return path


def test_play_data_summarises_each_record(rig, tmp_path: Path):
    path = _dataset(tmp_path / "s.depzdata")
    assert cli.main(["play-data", str(path), "--speed", "0"]) == 0
    assert rig.lines[0] == "devices: {'d0': 'sr04', 'd1': 'vl53l8', 'd2': 'bno086'}"
    assert rig.lines[1] == f"t={1000:>14} d0 {'sr04':8} echo=5831us (loop)"
    assert rig.lines[2] == f"t={2000:>14} d1 {'vl53l8':8} res=16 d[min..max]=120..400mm"
    assert rig.lines[3] == f"t={3000:>14} d2 {'temperature':8} {{'mcu_c': 31.5}}"
    assert rig.lines[4] == "3 records"


def test_play_data_ctrl_c_reports_records_so_far(rig, tmp_path: Path):
    path = _dataset(tmp_path / "s.depzdata")
    rig.interrupt_at(" d1 ")
    assert cli.main(["play-data", str(path), "--speed", "0"]) == 0
    assert rig.lines[-1] == "2 records"


# ── flash ────────────────────────────────────────────────────────────────────


def test_flash_reports_progress(rig, monkeypatch):
    calls = []

    def fake_update(port, fwdepz_path, *, progress=None):
        calls.append((port, fwdepz_path))
        progress(0.0, "entering bootloader")
        progress(0.5, "writing")
        progress(1.0, "done")
        return "APP_usonic_SR04_v1.06"

    monkeypatch.setattr(bootloader, "update_firmware", fake_update)
    assert cli.main(["flash", "/dev/fake0", "app.fwdepz"]) == 0
    assert calls == [("/dev/fake0", "app.fwdepz")]
    assert rig.lines == [
        "[  0.0%] entering bootloader",
        "[ 50.0%] writing",
        "[100.0%] done",
        "firmware updated; the device answers as APP_usonic_SR04_v1.06",
    ]


# ── record-data with a VL53L8 (contract 04) on the bus ───────────────────────


@pytest.mark.parametrize("pid, cls_name", [(PID_VL53L8CX, "cx"), (PID_VL53L8CH, "ch")])
def test_record_data_vl53l8_inits_configures_8x8_and_records_frames(rig, tmp_path: Path, pid, cls_name):
    """The VL53L8 branch: init with progress lines, 8×8 at --tof-hz, frames
    recorded as `vl53l8` records (contract 09), stream stopped on exit."""
    out = tmp_path / "s.depzdata"
    rig.add("/dev/a", FakeSr04, usb_serial="A")
    rig.add("/dev/tof8", FakeVl53l8, pid=pid, usb_serial="B")
    expected: list[dict] = []

    def feed():
        _wait(lambda: len(rig.opened["/dev/tof8"]) == 2 and _last(rig, "/dev/tof8").streaming, timeout=5.0)
        _wait(lambda: _last(rig, "/dev/a").loop_running)
        for i in range(3):
            _last(rig, "/dev/a").send_measurement(0x37, 5000 + i)
            _last(rig, "/dev/tof8").sensor.distance_offset_mm = 10 * i
            expected.append(_last(rig, "/dev/tof8").send_frame())
            time.sleep(0.01)

    feeder = threading.Thread(target=feed, daemon=True)
    feeder.start()
    assert cli.main(["record-data", "/dev/a", "/dev/tof8", "--out", str(out),
                     "--seconds", "1.0", "--tof-hz", "10", "--layers"]) == 0  # 8×8 caps at 15 Hz (contract 11 §3)
    feeder.join(5.0)
    assert len(expected) == 3

    tof = _last(rig, "/dev/tof8")
    assert rig.lines[0] == f"d0: {FakeSr04.SOFTWARE_NAME} @ /dev/a"
    assert rig.lines[1] == f"d1: {FakeVl53l8.SOFTWARE_NAME} @ /dev/tof8"
    assert rig.lines[2] == "/dev/tof8: init (may take ~30 s over network links)..."
    assert rig.lines[3] == "  SW reboot..."
    assert "  Sensor init done" in rig.lines
    assert rig.lines[-1] == f"wrote {out}"
    # the class the PID resolved to downloaded its own blob
    assert tof.sensor.variant == cls_name
    assert tof.sensor.resolution == 64 and tof.sensor.frequency_hz == 10
    assert tof.streaming is False and tof.sensor.ranging is False
    assert tof.link.closed and _last(rig, "/dev/a").link.closed

    reader = DatasetReader(out)
    assert {k: v["sensor_type"] for k, v in reader.devices.items()} == {"d0": "sr04", "d1": "vl53l8"}
    frames = [r for r in reader if r.kind == "vl53l8"]
    assert [r.device_id for r in frames] == ["d1"] * 3
    for rec, exp in zip(frames, expected):
        assert rec.value["resolution"] == 64
        assert rec.value["distance_mm"] == exp["distance_mm"]
        assert rec.value["target_status"] == exp["target_status"]
        assert rec.value["nb_target_detected"] == exp["nb_target_detected"]
        assert rec.value["signal_per_spad"] == exp["signal_per_spad"]  # --layers
        assert rec.value["range_sigma_mm"] == exp["range_sigma_mm"]
        assert rec.value["silicon_temp_degc"] == exp["silicon_temp_degc"]
    assert [r.value["distance_mm"][0] for r in frames] == [400, 410, 420]
    assert [r.value["echo_us"] for r in reader if r.kind == "sr04"] == [5000, 5001, 5002]
    assert not [r for r in reader if r.kind == "vl53l8_cnh"]  # no CNH armed by the CLI
    ts = [r.t_host_us for r in reader]
    assert ts == sorted(ts)


def test_record_data_vl53l8_without_layers_keeps_the_core_fields(rig, tmp_path: Path):
    out = tmp_path / "s.depzdata.gz"
    rig.add("/dev/tof8", FakeVl53l8, pid=PID_VL53L8CX)

    def feed():
        _wait(lambda: len(rig.opened["/dev/tof8"]) == 2 and _last(rig, "/dev/tof8").streaming, timeout=5.0)
        _last(rig, "/dev/tof8").send_frame()

    threading.Thread(target=feed, daemon=True).start()
    assert cli.main(["record-data", "/dev/tof8", "--out", str(out), "--seconds", "0.8"]) == 0
    recs = [r for r in DatasetReader(out) if r.kind == "vl53l8"]
    assert len(recs) == 1
    assert set(recs[0].value) == {"resolution", "silicon_temp_degc", "distance_mm",
                                  "target_status", "nb_target_detected"}
    assert len(recs[0].value["distance_mm"]) == 64
