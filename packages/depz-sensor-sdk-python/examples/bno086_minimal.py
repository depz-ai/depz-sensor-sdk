"""BNO086 IMU — stream orientation quaternion. Run: python bno086_minimal.py [port]"""

import sys

from depz_sensor_sdk import DepzError, open_device
from depz_sensor_sdk.bno086 import Bno086, RotationVector, SensorId

try:
    dev = open_device(sys.argv[1] if len(sys.argv) > 1 else None)  # no port: the first DEPZ board
except DepzError as e:
    sys.exit(f"open: {e}")
if not isinstance(dev, Bno086):
    dev.close()
    sys.exit(f"open: found a {type(dev).__name__}, not a BNO086: pass the port of a BNO086")

# Note: hardware_reset() is intentionally NOT called — it is unnecessary for
# streaming and firmware APP_BNO086_v0.95 doesn't emit the SH-2 executable
# reset-complete (ERRATA E9), so skipping it keeps this example robust.
dev.enable(SensorId.ROTATION_VECTOR, hz=100)
try:
    for rep in dev.reports(where=lambda r: r.sensor_id == SensorId.ROTATION_VECTOR):
        if isinstance(rep, RotationVector):
            print(f"i={rep.i:+.3f} j={rep.j:+.3f} k={rep.k:+.3f} w={rep.real:+.3f}  acc={rep.accuracy}")
except KeyboardInterrupt:
    pass
finally:
    dev.close()
