"""VL53L8CX ToF — stream 8x8 depth frames. Run: python vl53l8cx_minimal.py [port]"""

import sys

from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l8 import RESOLUTION_8X8, Vl53l8Cx

dev = open_device(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0")
assert isinstance(dev, Vl53l8Cx), f"expected VL53L8CX, got {type(dev).__name__}"

dev.init(progress=print)  # ~25 s: downloads the sensor firmware (variant fixed by class)
dev.set_resolution(RESOLUTION_8X8)
dev.set_ranging_frequency_hz(15)  # must be >= 2 Hz
dev.start_ranging()
try:
    for frame in dev.frames():
        g = frame.grid()  # 8x8 numpy array, mm
        print(f"center {g[3:5, 3:5].mean():5.0f} mm  min {g.min()}  max {g.max()}  {frame.silicon_temp_degc} C")
except KeyboardInterrupt:
    pass
finally:
    dev.stop_ranging()
    dev.close()
