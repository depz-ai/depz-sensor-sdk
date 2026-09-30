"""
uld/registry.py — the product table: one row per part number, holding what is
true of the part itself and which driver serves which way of driving it.

Two axes, and they are independent:

  * **product** — whose parameter set to load, e.g. 'VL53L4CD'. It decides the
    configuration blob, the register-address width and how far the resulting
    configuration can see. Normally it is what is soldered on the board, which
    the bootloader metablock names (`product_from_board_name`), but it is a
    *choice*: naming a neighbour on purpose is how one borrows its driver. ST's
    own advice for the VL53L3CX is exactly that — "the L3 is an L1 with the lens
    removed, use the L1 ULD if you patch the chip-id check" — and here that is
    just `product='VL53L1CX'` on an L3CX board.
  * **driver** — how to talk to the sensor, one of `DRIVER_KINDS`. Named by the
    kind of driver ST ships, not by a part number: the product is already the
    other half of the pair, so repeating it here could only contradict it.

`driver_for(product, kind)` resolves the pair. Every pair that exists has
exactly one implementation, and a pair that does not exist is a dash in the
table below rather than a silent fallback — a VL53L4CX has no light driver of
its own, and the way to run it as one is to say `product='VL53L4CD'`.

MODEL_ID is a cross-check (`model_id_ok`), never a selector: ST gives VL53L1CX
and VL53L1CB the same word, and VL53L4CD and VL53L4CX theirs, so the id can only
say "not something else entirely". A board whose metablock was never stamped
gets no automatic answer at all; the caller must say which product it is,
because guessing would be worse than asking.
"""

import re

from depz_sensor_sdk.vl53lx.uld.l0x import VL53L0X
from depz_sensor_sdk.vl53lx.uld.l1 import VL53L1
from depz_sensor_sdk.vl53lx.uld.l3 import VL53L3
from depz_sensor_sdk.vl53lx.uld.l4 import VL53L4
from depz_sensor_sdk.vl53lx.uld.bare.driver import VL53LX

# The three kinds of driver ST ships for this family, in the order a UI should
# list them. What separates them is where the ranging arithmetic runs:
#
#   'uld'       Ultra Lite Driver, ~1000 lines of C: the die computes the
#               distance itself and the driver reads it out of a register.
#   'ulp'       Ultra Low Power, the same shape with power saving on top. ST
#               ships one for the VL53L3CX only.
#   'histogram' the Bare Driver, ~37000 lines: the die hands over 24 raw photon
#               bins and the host turns them into targets, so this is the only
#               kind that gives histograms and more than one target per frame.
#               "Bare" is ST's own word for it and means "no platform layer",
#               not "cut down" - it is the complete driver.
DRIVER_KINDS = ('uld', 'ulp', 'histogram')

# Every product of the 1D family, in the order a UI should list them.
PRODUCTS = ('VL53L0X', 'VL53L1CX', 'VL53L1CB', 'VL53L3CX', 'VL53L4CD',
            'VL53L4CX')


class Product:
    """One row of the table: the facts that belong to the part number.

    `reach_mm` is the datasheet rating of the module — its own limit, not the
    host's. How far a loaded configuration actually sees is a second number and
    belongs to the driver (`SensorDriver.reach_mm()`); the session reports both,
    because a light driver built around a short VCSEL period reaches its own
    product's distance even on a die rated for more.

    `drivers` maps a kind from DRIVER_KINDS to the class implementing it. A kind
    that is absent is absent on purpose: see the module docstring.

    `caveats` is keyed by kind and holds what to tell the user before they range
    with that pair.
    """

    def __init__(self, model_id, reach_mm, drivers, caveats=None):
        self.model_id = model_id
        self.reach_mm = reach_mm
        self.drivers = drivers
        self.caveats = caveats or {}


_HIST_CAVEAT = ('the histogram driver has no calibrations and no detection '
                'thresholds - the light drivers are the ones with those')

# Which pairs exist. Everything measured on hardware; see the plan's note
# "Матрица product x driver снята на железе".
TABLE = {
    'VL53L0X': Product(
        # The only part of the family on its own silicon: no histogram firmware
        # in the die and no Fast Mode Plus on the pad.
        model_id=0x00EE, reach_mm=2000,
        drivers={'uld': VL53L0X},
        caveats={'uld': 'no detection thresholds; there is no ROI on this die '
                        'at all'}),
    'VL53L1CX': Product(
        model_id=0xEACC, reach_mm=4000,
        drivers={'uld': VL53L1, 'histogram': VL53LX},
        caveats={'histogram': _HIST_CAVEAT}),
    # Same die and same ULD as the CX - the CB is the module with the cover
    # glass, so it is the one that usually needs a crosstalk calibration.
    'VL53L1CB': Product(
        model_id=0xEACC, reach_mm=8000,
        drivers={'uld': VL53L1, 'histogram': VL53LX},
        caveats={'histogram': _HIST_CAVEAT}),
    'VL53L3CX': Product(
        # ST ships no ULD for this part, only the ULP and the Bare Driver. For a
        # light driver with modes and a longer reach, say product='VL53L1CX'.
        model_id=0xEAAA, reach_mm=3000,
        drivers={'ulp': VL53L3, 'histogram': VL53LX},
        caveats={'ulp': 'single-target ranging only - the histogram driver '
                        'gives several targets instead',
                 'histogram': _HIST_CAVEAT}),
    'VL53L4CD': Product(
        model_id=0xEBAA, reach_mm=1200,
        drivers={'uld': VL53L4, 'histogram': VL53LX},
        caveats={'histogram': _HIST_CAVEAT}),
    'VL53L4CX': Product(
        # The Bare Driver is what ST ships for this part, and it is the only way
        # the die reaches past the 1.2 m its sibling's blob is built for. To run
        # it light, name the sibling: product='VL53L4CD' (short range, full
        # calibrations) or product='VL53L1CX' (long VCSEL period, modes, ROI).
        model_id=0xEBAA, reach_mm=6000,
        drivers={'histogram': VL53LX},
        caveats={'histogram': _HIST_CAVEAT}),
}

_NAME_RE = re.compile(r'VL53L(\d[A-Z0-9]*)')


def product_from_board_name(name: str):
    """`ToF Sensor VL53L4CD USB v2.1` -> 'VL53L4CD'. None if the name carries
    no product number — an unstamped board, or a name we do not recognise."""
    if not name:
        return None
    m = _NAME_RE.search(name.upper())
    if not m:
        return None
    product = 'VL53L' + m.group(1)
    return product if product in TABLE else None


def product(name: str) -> Product:
    """-> the table row. Raises for a product nobody serves."""
    try:
        return TABLE[name]
    except KeyError:
        raise NotImplementedError(
            f'no such product: {name} - served: '
            f'{", ".join(supported_products())}') from None


def driver_kinds(name: str) -> tuple:
    """-> the driver kinds this product has, in DRIVER_KINDS order."""
    have = product(name).drivers
    return tuple(k for k in DRIVER_KINDS if k in have)


def driver_for(name: str, kind: str):
    """-> (driver class, caveat or None) for one product/driver pair.

    Raises for a pair the table has no row for, naming what the product does
    have: the fix is either another kind or another product, and both are the
    caller's decision to make.
    """
    row = product(name)
    try:
        return row.drivers[kind], row.caveats.get(kind)
    except KeyError:
        raise NotImplementedError(
            f'{name} has no {kind!r} driver - it has '
            f'{", ".join(driver_kinds(name))}') from None


def reach_mm(name: str):
    """The product's rated maximum ranging distance, mm (datasheet)."""
    row = TABLE.get(name)
    return row.reach_mm if row else None


def model_id_ok(name: str, value: int) -> bool:
    """Cross-check the id the sensor answered against the product asked for.
    True only says "not something else entirely": the pairs that share an id
    cannot be told apart this way."""
    row = TABLE.get(name)
    return bool(row) and row.model_id == value


def supported_products() -> tuple:
    return tuple(p for p in PRODUCTS if p in TABLE)
