"""VL53L4CD ToF — stream single-zone distance. Run: python vl53l4cd_minimal.py [port]"""

import sys

from depz_sensor_sdk import DepzError, Vl53l4cd, open_device

try:
    dev = open_device(sys.argv[1] if len(sys.argv) > 1 else None)  # no port: the first DEPZ board
except DepzError as e:
    sys.exit(f"open: {e}")
if not isinstance(dev, Vl53l4cd):
    dev.close()
    sys.exit(f"open: found a {type(dev).__name__}, not a VL53L4CD: pass the port of a VL53L4CD")

dev.init()  # ULD boot + VHV calibration, well under a second (no firmware blob)
dev.start_ranging()  # default timing: 50 ms budget, continuous (~20 Hz)
try:
    for m in dev.measurements():
        print(f"{m.distance_mm:5d} mm  sigma {m.sigma_mm:3d} mm  {m.status_text}")
except KeyboardInterrupt:
    pass
finally:
    dev.stop_ranging()
    dev.close()
