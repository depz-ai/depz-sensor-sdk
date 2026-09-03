"""BNO086 IMU — stream orientation quaternion. Run: python bno086_minimal.py [port]"""

import sys

from depz_sensor_sdk import open_device
from depz_sensor_sdk.bno086 import Bno086, SensorId

dev = open_device(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0")
assert isinstance(dev, Bno086), f"expected BNO086, got {type(dev).__name__}"

# Note: hardware_reset() is intentionally NOT called — it is unnecessary for
# streaming and firmware APP_BNO086_v0.95 doesn't emit the SH-2 executable
# reset-complete (ERRATA E9), so skipping it keeps this example robust.
dev.enable_rotation_vector(hz=100)
try:
    for rep in dev.reports(sensors=[SensorId.ROTATION_VECTOR]):
        print(f"i={rep.i:+.3f} j={rep.j:+.3f} k={rep.k:+.3f} w={rep.real:+.3f}  acc={rep.accuracy}")
except KeyboardInterrupt:
    pass
finally:
    dev.close()
