"""
uld/l4.py — VL53L4CD ULD 2.2.3 in Python, for VL53L4CD and VL53L4CX alike.

One class, `VL53L4`, serves both products: over I2C they are the same machine —
same model id, same register map, same result block. What the L4CX has of its
own is the histogram mode, which is not this ULD at all but ST's BareDriver, in
uld/vl53lx/. Which product a board carries comes from uld/registry.py, not from
this class.

What this module holds is what belongs to this ULD: its configuration blob,
the write that closes its init and its two calibrations. The register map, the
result block and the ranging body it shares with the other drivers of the die
live in uld/vl53l1_die.py.

A faithful port of VL53L4CD_api.c + VL53L4CD_calibration.c — register
sequences and integer widths included. Do not "simplify" them.

Reference C driver: ../../temp/STSW-IMG026/VL53L4CD_ULD_Driver/
"""

from depz_sensor_sdk.vl53lx._link import Vl53Error
from depz_sensor_sdk.vl53lx.uld.vl53l1_die import (CONFIG_ADDR, CONFIG_END, INNER_OFFSET_MM,
                            OUTER_OFFSET_MM, RANGE_OFFSET_MM,
                            XTALK_PLANE_OFFSET_KCPS, VL53L1Die)

# ── ULD driver (VL53L4CD_api.c / VL53L4CD_calibration.c, 2.2.3) ──────────────
ULD_VERSION = (2, 2, 3, 0)

# VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87. Byte 0
# (register 0x2D) is overridden with CONFIG_FMP_BYTE by sensor_init(), as on
# the rest of the die.
DEFAULT_CONFIGURATION = bytes([
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,   # 0x2D..0x34
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   # 0x35..0x3C
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,   # 0x3D..0x44
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,   # 0x45..0x4C
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   # 0x4D..0x54
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   # 0x55..0x5C
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00,   # 0x5D..0x64
    0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,   # 0x65..0x6C
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   # 0x6D..0x74
    0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,   # 0x75..0x7C
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00,   # 0x7D..0x84
    0x01, 0x00, 0x00,                                 # 0x85..0x87
])
assert len(DEFAULT_CONFIGURATION) == CONFIG_END - CONFIG_ADDR + 1


class VL53L4(VL53L1Die):
    """Port of VL53L4CD_api.c + VL53L4CD_calibration.c (ULD 2.2.3)."""

    CONFIGURATION = DEFAULT_CONFIGURATION
    SUPPORTS      = frozenset({'timing', 'offset', 'xtalk', 'thresholds',
                               'signal_thresh', 'sigma_thresh', 'temp_update',
                               'calib_offset', 'calib_xtalk'})

    def reach_mm(self):
        """1.2 m — the VL53L4CD's rating, because this is its blob: VCSEL
        periods 0x07/0x05 and a 0x38 phase window. A VL53L4CX running it is
        held to the same distance although the product is rated for 6 m;
        `--hist` is what gives that die its own range."""
        return 1200

    def init_extra(self):
        """The one write VL53L4CD_SensorInit() adds at the end of init:
        ALGO__RANGE_IGNORE_THRESHOLD_MCPS."""
        self.p.wr_word(0x0024, 0x0500)

    # ── calibration (VL53L4CD_calibration.c) ──
    def calibrate_offset(self, target_dist_mm: int, nb_samples: int = 20) -> int:
        if not 5 <= nb_samples <= 255 or not 10 <= target_dist_mm <= 1000:
            raise Vl53Error('nb_samples must be 5..255, target 10..1000 mm')

        self.p.wr_word(RANGE_OFFSET_MM, 0)
        self.p.wr_word(INNER_OFFSET_MM, 0)
        self.p.wr_word(OUTER_OFFSET_MM, 0)

        self._collect(10, lambda i, r: None)            # device heat loop

        distances = []
        self._collect(nb_samples, lambda i, r: distances.append(r.distance_mm))

        offset_mm = target_dist_mm - sum(distances) // nb_samples
        self.p.wr_word(RANGE_OFFSET_MM, (offset_mm * 4) & 0xFFFF)
        return offset_mm

    def calibrate_xtalk(self, target_dist_mm: int, nb_samples: int = 20) -> int:
        if not 5 <= nb_samples <= 255 or not 10 <= target_dist_mm <= 5000:
            raise Vl53Error('nb_samples must be 5..255, target 10..5000 mm')

        self.p.wr_word(XTALK_PLANE_OFFSET_KCPS, 0)      # disable compensation

        self._collect(10, lambda i, r: None)            # device heat loop

        samples = []

        def keep(i, r):
            # Discard invalid measurements and the first frame.
            if r.range_status == 0 and i > 0:
                samples.append(r)

        self._collect(nb_samples, keep)

        if not samples:
            raise Vl53Error('xtalk calibration failed: no valid samples')

        n = float(len(samples))
        avg_distance = sum(s.distance_mm for s in samples) / n
        avg_spad_nb = sum(s.number_of_spad for s in samples) / n
        avg_signal = sum(s.signal_rate_kcps for s in samples) / n

        tmp_xtalk = (1.0 - avg_distance / float(target_dist_mm)) * (avg_signal / avg_spad_nb)
        if tmp_xtalk > 127:     # 127 kcps is the max xtalk value (65536/512)
            raise Vl53Error(f'xtalk calibration failed: {tmp_xtalk:.1f} kcps > 127')

        self.p.wr_word(XTALK_PLANE_OFFSET_KCPS, int(tmp_xtalk * 512.0) & 0xFFFF)
        return round(tmp_xtalk)
