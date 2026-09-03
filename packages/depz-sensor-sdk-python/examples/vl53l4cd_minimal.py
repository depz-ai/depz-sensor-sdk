"""VL53L4CD ToF — stream single-zone distance. Run: python vl53l4cd_minimal.py [port]"""

import sys

from depz_sensor_sdk import Vl53l4Cd, open_device

dev = open_device(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0")
assert isinstance(dev, Vl53l4Cd), f"expected VL53L4CD, got {type(dev).__name__}"

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
