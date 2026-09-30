"""
uld/vl53lx — ST's VL53LX BareDriver on the host, for VL53L3CX and VL53L4CX.

These two dies run a histogram engine: instead of a single distance the device
returns 24 photon-count bins per frame, and the driver turns those into one or
more targets. That post-processing is what the ULP driver in `uld/l3.py` does
not have, and it is why this package exists.

Structure, as it is being built:

    regs.py     register blocks and their codec, generated from the C driver
    image.py    DeviceImage — the blocks that make up the device state, and
                moving them over the bridge
    nvm.py      NvmReader — the factory calibration that is not in any register
    tuning.py   ST's tuning parameters, generated from the C driver
    core.py     BareDriver — preset modes, timing maths, the ranging cycle and
                the histogram frame
    hist.py     post-processing — 24 bins into targets, on the host alone
    driver.py   VL53LX — the SensorDriver vl53_tool.py talks to

Reference C driver: ../../../temp/STSW-IMG033_L3/…/VL53L3CX_BareDriver/
"""

from depz_sensor_sdk.vl53lx.uld.bare.image import DeviceImage        # noqa: F401
from depz_sensor_sdk.vl53lx.uld.bare.nvm import NvmReader            # noqa: F401
from depz_sensor_sdk.vl53lx.uld.bare.regs import BLOCKS, RegBlock    # noqa: F401
from depz_sensor_sdk.vl53lx.uld.bare.tuning import TUNING, DEFAULTS  # noqa: F401
from depz_sensor_sdk.vl53lx.uld.bare.core import BareDriver, HistogramBinData    # noqa: F401
from depz_sensor_sdk.vl53lx.uld.bare.hist import process_data, to_measurement   # noqa: F401
from depz_sensor_sdk.vl53lx.uld.bare.driver import VL53LX                       # noqa: F401
