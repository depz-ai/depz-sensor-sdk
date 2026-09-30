"""
uld/l3.py — VL53L3CX ULP 1.0.0 in Python: plain single-target ranging.

The VL53L3CX is a histogram part, but ST also ships an "ultra low power" driver
for it that runs the die in the same simple mode the VL53L1/L4 ULDs use. This
module is that driver. It gives distance, signal, ambient and sigma — no
multi-target list and no histograms; for those the product has a second driver,
uld/vl53lx/.

Why it sits on VL53L1Die: under the ULP configuration the L3CX and the L4CD are
the same machine. Same register addresses, same 17-byte result block at 0x0089,
same status table, and — the part that actually matters for the timing
arithmetic — the same VCSEL periods in the configuration blob (0x60 = 0x07,
0x63 = 0x05), so the MACROP_A/B timeouts mean the same milliseconds on both.
What the ULP genuinely has of its own is its configuration blob, two extra
writes at the end of init and a different stop code. That is what this module
holds.

ST's own ULP API is narrower than what is inherited here: it exposes the
MACROP_A/B pair as a raw "macro timing" number instead of milliseconds,
its ROI is square-only, and its interrupt configuration is one threshold plus a
flag rather than the four-window form. Those are restrictions of that API, not
of the die — the registers underneath are the family's, so the family's shape
is kept and the tool stays uniform.

Reference C driver: ../../temp/STSW-IMG033_L3/VL53L3CX_UltraLowPower_Driver/
"""

from depz_sensor_sdk.vl53lx.uld.vl53l1_die import (CONFIG_ADDR, CONFIG_END,
                            PHASECAL_CONFIG__TIMEOUT_MACROP, VL53L1Die)

ULP_VERSION = (1, 0, 0)

# VL53L3CX_ULP_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
# Byte 0 (register 0x2D) is overridden with CONFIG_FMP_BYTE by sensor_init(),
# as on the rest of the family.
DEFAULT_CONFIGURATION = bytes([
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,   # 0x2D..0x34
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   # 0x35..0x3C
    0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,   # 0x3D..0x44
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,   # 0x45..0x4C
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   # 0x4D..0x54
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   # 0x55..0x5C
    0x00, 0x00, 0x01, 0x07, 0x00, 0x02, 0x05, 0x00,   # 0x5D..0x64
    0xb4, 0x00, 0xbb, 0x08, 0x38, 0x00, 0x00, 0x00,   # 0x65..0x6C
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   # 0x6D..0x74
    0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,   # 0x75..0x7C
    0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00,   # 0x7D..0x84
    0x01, 0x00, 0x00,                                 # 0x85..0x87
])
assert len(DEFAULT_CONFIGURATION) == CONFIG_END - CONFIG_ADDR + 1


class VL53L3(VL53L1Die):
    """Port of VL53L3CX_ULP_api.c (ULP 1.0.0). See the module docstring for
    what it takes from the die and what is its own."""

    CONFIGURATION = DEFAULT_CONFIGURATION
    # The ULP writes 0x00 where the VL53L4CD ULD writes 0x80.
    STOP_MODE     = 0x00
    # No offset, crosstalk or calibration: the ULP driver has none, and on a
    # histogram part those are not the same registers the VL53L4CD uses.
    SUPPORTS      = frozenset({'timing', 'thresholds', 'signal_thresh',
                               'sigma_thresh', 'roi'})

    def init_extra(self):
        """The writes that close VL53L3CX_ULP_SensorInit(): the same
        ALGO__RANGE_IGNORE_THRESHOLD_MCPS the L4CD ULD sets, plus two of the
        ULP's own.

        The ULP also leaves the inter-measurement period at 1000 ms; the die's
        sensor_init() closes with set_range_timing(50, 0), which brings the
        part to the 50 ms continuous the rest of the family starts at.
        """
        self.p.wr_word(0x0024, 0x0500)
        self.p.wr_byte(0x0081, 0x8A)                # 0b10001010, ULP only
        self.p.wr_byte(PHASECAL_CONFIG__TIMEOUT_MACROP, 0x03)
