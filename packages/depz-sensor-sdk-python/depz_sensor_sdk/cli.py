"""`depz-sensor` command-line tool.

Subcommands: list, info, monitor, record (flash lands in M2).
"""

from __future__ import annotations

import argparse
import sys
import time


def cmd_list(args: argparse.Namespace) -> int:
    from .discovery import list_depz_devices

    devices = list_depz_devices(timeout=args.timeout)
    if not devices:
        print("no DEPZ devices found")
        return 1
    for d in devices:
        sensor = d.sensor_type.value if d.sensor_type else "-"
        vidpid = (
            f"{d.usb_vid:04x}:{d.usb_pid:04x}" if d.usb_vid is not None and d.usb_pid is not None else "-"
        )
        print(
            f"{d.port}\t{d.mode}\t{sensor}\t{d.software_name}\t"
            f"name={d.device_name!r}\tserial={d.serial_number!r}\tusb={vidpid}"
        )
    return 0


def cmd_info(args: argparse.Namespace) -> int:
    from .discovery import probe_port

    info = probe_port(args.port, timeout=args.timeout)
    if info is None:
        print(f"no DEPZ device answered on {args.port}", file=sys.stderr)
        return 1
    for field in ("port", "mode", "sensor_type", "software_name", "fw_version",
                  "device_name", "serial_number"):
        print(f"{field}: {getattr(info, field)}")
    return 0


def _open_for_streaming(port: str, record_path: str | None):
    from .device import DeviceBase
    from .discovery import open_device
    from .transport.record_replay import RecordingLink
    from .transport.serial_link import SerialLink

    if record_path is None:
        return open_device(port)
    link = RecordingLink(SerialLink(port), record_path, header_extra={"port": port})
    dev = DeviceBase(link, timeout=0.2)
    from .discovery import _identify, _promote

    return _promote(dev, _identify(dev))


def cmd_monitor(args: argparse.Namespace) -> int:
    from .sr04 import Sr04
    from .vl53l4 import Vl53l4Cd

    dev = _open_for_streaming(args.port, args.record)
    try:
        print(f"connected: {dev.get_software_name()} (Ctrl+C to stop)")
        dev.on_event(lambda ev: print(f"[event] {ev}"))
        if isinstance(dev, Sr04):
            dev.start()
            for m in dev.stream():
                d = m.distance_mm
                dist = f"{d:8.1f} mm" if d is not None else "   no echo"
                print(f"{m.timestamp_us:>12} us  {m.echo_time_us:>6} us  {dist}  [{m.source}]")
        elif isinstance(dev, Vl53l4Cd):
            dev.init()
            dev.start_ranging()
            for r in dev.measurements():
                print(
                    f"{r.timestamp_us:>12} us  {r.distance_mm:>5} mm  "
                    f"sigma {r.sigma_mm:>3}  signal {r.signal_rate_kcps:>6} kcps  "
                    f"status {r.range_status} ({r.status_text})"
                )
        else:
            print("generic device: printing events; temperature every 2 s")
            while True:
                print(f"MCU temperature: {dev.read_mcu_temperature():.1f} C")
                time.sleep(2)
    except KeyboardInterrupt:
        return 0
    finally:
        try:
            if isinstance(dev, Sr04):
                dev.stop()
            elif isinstance(dev, Vl53l4Cd) and dev.ranging:
                dev.stop_ranging()
        except Exception:
            pass
        dev.close()
    return 0


def cmd_record(args: argparse.Namespace) -> int:
    args.record = args.out
    return cmd_monitor(args)


def cmd_record_data(args: argparse.Namespace) -> int:
    """Multi-device decoded recording onto one host timeline (contract 09)."""
    import time as _time

    from .dataset import SessionRecorder
    from .discovery import open_device
    from .sr04 import Sr04
    from .vl53l4 import Vl53l4Cd
    from .vl53l8 import RESOLUTION_8X8, Vl53l8

    devices = [open_device(p) for p in args.ports]
    rec = SessionRecorder(args.out, vl53l8_layers=args.layers)
    try:
        for dev in devices:
            did = rec.add(dev)
            print(f"{did}: {dev.get_software_name()} @ {dev.port}")
        rec.start()
        for dev in devices:
            if isinstance(dev, Sr04):
                dev.start()
            elif isinstance(dev, Vl53l4Cd):
                dev.init()
                dev.start_ranging()
            elif isinstance(dev, Vl53l8):
                print(f"{dev.port}: init (may take ~30 s over network links)...")
                dev.init(progress=lambda s: print(f"  {s}"))
                dev.set_resolution(RESOLUTION_8X8)
                dev.set_ranging_frequency_hz(args.tof_hz)
                dev.start_ranging()
        print(f"recording to {args.out} for {args.seconds:.0f}s (Ctrl+C to stop early)")
        t_end = _time.time() + args.seconds
        while _time.time() < t_end:
            _time.sleep(0.5)
    except KeyboardInterrupt:
        pass
    finally:
        for dev in devices:
            try:
                if isinstance(dev, Sr04):
                    dev.stop()
                elif isinstance(dev, (Vl53l4Cd, Vl53l8)) and dev.ranging:
                    dev.stop_ranging()
            except Exception:
                pass
        rec.stop()
        for dev in devices:
            dev.close()
    print(f"wrote {args.out}")
    return 0


def cmd_play_data(args: argparse.Namespace) -> int:
    from .dataset import DatasetReader

    reader = DatasetReader(args.path)
    print("devices:", {k: v.get("sensor_type") for k, v in reader.devices.items()})
    n = 0

    def show(rec):
        nonlocal n
        n += 1
        summary = rec.value
        if rec.kind == "vl53l8":
            d = rec.value["distance_mm"]
            summary = f"res={rec.value['resolution']} d[min..max]={min(d)}..{max(d)}mm"
        elif rec.kind == "sr04":
            summary = f"echo={rec.value['echo_us']}us ({rec.value['source']})"
        print(f"t={rec.t_host_us:>14} {rec.device_id} {rec.kind:8} {summary}")

    try:
        reader.play(show, speed=args.speed)
    except KeyboardInterrupt:
        pass
    print(f"{n} records")
    return 0


def cmd_flash(args: argparse.Namespace) -> int:
    from .bootloader import update_firmware

    def progress(frac: float, msg: str) -> None:
        print(f"[{frac * 100:5.1f}%] {msg}")

    update_firmware(args.port, args.fwdepz, progress=progress)
    print("firmware updated; device rebooted into the application")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="depz-sensor", description="DEPZ USB sensor line tool")
    parser.add_argument("--version", action="store_true", help="print SDK version")
    sub = parser.add_subparsers(dest="command")

    p_list = sub.add_parser("list", help="probe serial ports for DEPZ devices")
    p_list.add_argument("--timeout", type=float, default=0.2)
    p_list.set_defaults(func=cmd_list)

    p_info = sub.add_parser("info", help="identify one port")
    p_info.add_argument("port")
    p_info.add_argument("--timeout", type=float, default=0.2)
    p_info.set_defaults(func=cmd_info)

    p_mon = sub.add_parser("monitor", help="stream measurements/events to stdout")
    p_mon.add_argument("port")
    p_mon.add_argument("--record", help="also capture to a .depzrec file")
    p_mon.set_defaults(func=cmd_monitor)

    p_rec = sub.add_parser("record", help="monitor + capture to .depzrec")
    p_rec.add_argument("port")
    p_rec.add_argument("--out", required=True, help="output .depzrec path")
    p_rec.set_defaults(func=cmd_record)

    p_rd = sub.add_parser(
        "record-data", help="multi-device time-synced dataset capture (.depzdata)"
    )
    p_rd.add_argument("ports", nargs="+", help="serial ports of the devices to record")
    p_rd.add_argument("--out", required=True, help="output .depzdata[.gz] path")
    p_rd.add_argument("--seconds", type=float, default=10.0)
    p_rd.add_argument("--tof-hz", type=int, default=15, help="VL53L8 ranging frequency")
    p_rd.add_argument("--layers", action="store_true",
                      help="include signal/ambient/sigma/reflectance layers")
    p_rd.set_defaults(func=cmd_record_data)

    p_pd = sub.add_parser("play-data", help="play a .depzdata dataset to stdout")
    p_pd.add_argument("path")
    p_pd.add_argument("--speed", type=float, default=1.0, help="0 = as fast as possible")
    p_pd.set_defaults(func=cmd_play_data)

    p_fl = sub.add_parser("flash", help="update application firmware from a .fwdepz file")
    p_fl.add_argument("port")
    p_fl.add_argument("fwdepz")
    p_fl.set_defaults(func=cmd_flash)

    args = parser.parse_args(argv)
    if args.version:
        from . import __version__

        print(__version__)
        return 0
    if not getattr(args, "command", None):
        parser.print_help()
        return 0
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
