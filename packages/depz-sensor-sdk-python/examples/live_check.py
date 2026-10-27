"""Live check on real boards: every DEPZ board in /dev/serial/by-id (or the ports given) is
opened, set streaming the usual way and read for N seconds; per board the rate and one
plausibility value (the centre distance of a ToF sensor, |gravity| of a BNO055). The same
check runs in the Rust, Java and C# SDKs, so the numbers can be compared board by board.

    uv run python examples/live_check.py [seconds] [port ...]
"""

from __future__ import annotations

import os
import statistics
import sys
import time
from typing import Any, Callable, Iterator

from depz_sensor_sdk import open_device
from depz_sensor_sdk.bno055 import Bno055
from depz_sensor_sdk.bno086 import Bno086
from depz_sensor_sdk.bno086.reports import SensorId
from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.sr04 import Sr04
from depz_sensor_sdk.tmf8829 import Tmf8829
from depz_sensor_sdk.vl53l4 import Vl53l4cd
from depz_sensor_sdk.vl53l8 import RESOLUTION_8X8, Vl53l8
from depz_sensor_sdk.vl53lx import Vl53lx


def centre(d: Any) -> float | None:
    """The four centre zones of an 8×8 frame."""
    d = list(d)
    return sum(d[i] for i in (27, 28, 35, 36)) / 4 if len(d) >= 64 else None


def setup(dev: DeviceBase) -> tuple[Iterator[Any], Callable[[Any], float | None], Callable[[], None]]:
    """Start the board's usual stream: (items, plausibility value of an item, stop)."""
    if isinstance(dev, Sr04):
        it = dev.stream()
        dev.start()
        return it, lambda m: m.distance_mm, dev.stop
    if isinstance(dev, Vl53l4cd):
        dev.init()
        it = dev.measurements()
        dev.start_ranging()
        return it, lambda m: m.distance_mm if m.range_status == 0 else None, dev.stop_ranging
    if isinstance(dev, Vl53l8):
        dev.init()
        dev.set_resolution(RESOLUTION_8X8)
        dev.set_ranging_frequency_hz(15)
        it = dev.frames()
        dev.start_ranging()
        return it, lambda f: centre(f.distance_mm.ravel()), dev.stop_ranging
    if isinstance(dev, Vl53lx):
        dev.init()
        it = dev.measurements()
        dev.start_ranging()
        return it, lambda m: m.distance_mm if m.status == 0 else None, dev.stop_ranging

    if isinstance(dev, Tmf8829):
        def tmf(m: Any) -> float | None:
            z = [float(v) for v in (m.distance_mm.ravel() if m.distance_mm is not None else []) if v > 0]
            return statistics.median(z) if z else None

        dev.init()
        it = dev.measurements()
        dev.start()
        return it, tmf, dev.stop
    if isinstance(dev, Bno055):
        dev.configure()
        it = dev.samples()
        dev.start_stream(10)
        return it, lambda s: sum(v * v for v in s.gravity) ** 0.5 if s.gravity else None, lambda: None
    if isinstance(dev, Bno086):
        it = dev.reports()
        dev.enable(SensorId.ROTATION_VECTOR, 100)
        dev.enable(SensorId.ACCELEROMETER, 100)
        return it, lambda r: None, lambda: None
    raise TypeError(f"no live check for {type(dev).__name__}")


def unit(dev: DeviceBase) -> str:
    return "|g| m/s²" if isinstance(dev, Bno055) else "reports" if isinstance(dev, Bno086) else "mm"


def main() -> int:
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
    ports = sys.argv[2:] or sorted(os.path.join("/dev/serial/by-id", p) for p in os.listdir("/dev/serial/by-id"))
    failed = 0
    for port in ports:
        name = os.path.basename(port)
        t = time.monotonic()
        try:
            dev = open_device(port)
            try:
                sw = dev.get_software_name()
                it, value, stop = setup(dev)
                n, vals = 0, []
                end = time.monotonic() + secs
                while time.monotonic() < end:
                    item = next(it)
                    n += 1
                    v = value(item)
                    if v is not None:
                        vals.append(v)
                stop()
            finally:
                dev.close()
            med = f"{statistics.median(vals):.1f} {unit(dev)}" if vals else "—"
            print(f"ok    {name:<70} {sw:<22} {n / secs:6.1f}/s  median {med}  ({time.monotonic() - t:.1f} s)")
        except Exception as e:  # noqa: BLE001 - one board's failure is a line of the report
            failed += 1
            print(f"FAIL  {name:<70} {type(e).__name__}: {e}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
