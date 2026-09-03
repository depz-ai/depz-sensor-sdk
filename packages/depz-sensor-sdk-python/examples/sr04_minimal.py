"""SR04 ultrasonic — stream live distance. Run: python sr04_minimal.py [port]"""

import sys

from depz_sensor_sdk import Sr04, open_device

dev = open_device(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0")
assert isinstance(dev, Sr04), f"expected SR04, got {type(dev).__name__}"

dev.set_sample_period_us(50_000)  # 20 Hz
dev.start()
try:
    for m in dev.stream():
        print(f"{m.distance_mm:7.1f} mm" if m.valid else "   no echo")
except KeyboardInterrupt:
    pass
finally:
    dev.stop()
    dev.close()
