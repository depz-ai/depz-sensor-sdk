"""VL53L8CH ToF — 8x8 depth frames PLUS Compact-Network-Histogram (CNH) output.

Vl53l8Ch inherits the whole Vl53l8Cx surface; the only addition is configure_cnh().
Run: python vl53l8ch_minimal.py [port]
"""

import sys

from depz_sensor_sdk import CnhConfig, open_device
from depz_sensor_sdk.vl53l8 import RESOLUTION_8X8, Vl53l8Ch, cnh

dev = open_device(sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0")
assert isinstance(dev, Vl53l8Ch), f"expected VL53L8CH, got {type(dev).__name__}"

dev.init(progress=print)  # ~25 s: downloads the CH sensor firmware (variant fixed by class)
dev.set_resolution(RESOLUTION_8X8)
dev.set_ranging_frequency_hz(15)  # must be >= 2 Hz

# CNH: a per-aggregate distance histogram on top of the normal frame (CH only).
cfg = CnhConfig()
cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)  # 16 aggregates
assert cfg.required_memory() <= 6160, "CNH config exceeds the device buffer"
dev.configure_cnh(cfg)  # arm the histogram block before ranging (Vl53l8Ch only)

dev.start_ranging()
try:
    for frame in dev.frames():
        g = frame.grid()  # 8x8 numpy array, mm
        line = f"center {g[3:5, 3:5].mean():5.0f} mm  min {g.min()}  max {g.max()}  {frame.silicon_temp_degc} C"
        if frame.cnh_raw is not None:
            decoded = cnh.decode(cfg, frame.cnh_raw)  # {'ref_residual', 'aggregates'}
            line += f"  CNH aggregates: {len(decoded['aggregates'])}"
        print(line)
except KeyboardInterrupt:
    pass
finally:
    dev.stop_ranging()
    dev.close()
