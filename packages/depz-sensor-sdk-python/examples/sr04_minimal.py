"""SR04 ultrasonic — stream live distance. Run: python sr04_minimal.py [port]"""

import sys

from depz_sensor_sdk import DepzError, Sr04, open_device

try:
    dev = open_device(sys.argv[1] if len(sys.argv) > 1 else None)  # no port: the first DEPZ board
except DepzError as e:
    sys.exit(f"open: {e}")
if not isinstance(dev, Sr04):
    dev.close()
    sys.exit(f"open: found a {type(dev).__name__}, not an SR04: pass the port of an SR04")

dev.set_sample_period_us(50_000)  # 20 Hz
dev.start()
try:
    for m in dev.measurements():
        print(f"{m.distance_mm:7.1f} mm" if m.valid else "   no echo")
except KeyboardInterrupt:
    pass
finally:
    dev.stop()
    dev.close()
