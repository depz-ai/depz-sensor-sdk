"""
uld/vl53lx/image.py — the device state the BareDriver keeps, and how it moves.

The C driver holds one struct per register block inside `VL53LX_LLDriverData_t`
and never reads a configuration register on its own: it edits the struct, then
pushes whole blocks. Ranging start is the extreme case — `VL53LX_init_and_start_
range()` encodes static_nvm, customer_nvm, static_config, general_config,
timing_config, dynamic_config and system_control back to back and writes
0x0001..0x0087 in a single 135-byte transfer.

That matters here because every transfer costs a USB round trip. Block moves
keep the count where the C driver has it; register-at-a-time would multiply it
by fifty.

The blocks are almost contiguous: static_nvm ends at 0x000C and customer_nvm
starts at 0x000D. The C driver does not special-case that hole — it zeroes the
whole buffer first and drops each block at its own offset, so the reserved byte
goes out as 0x00. `push_range()` does the same, for the same bytes on the wire.
"""

from depz_sensor_sdk.vl53lx.uld.bare.regs import BLOCKS, RegBlock

# The blocks `init_and_start_range` pushes, in address order. Named here because
# the order is what makes them one transfer.
RANGE_START_BLOCKS = ('static_nvm_managed', 'customer_nvm_managed',
                      'static_config', 'general_config', 'timing_config',
                      'dynamic_config', 'system_control')


class DeviceImage:
    """The device's register image, block by block.

    Blocks are reached as attributes under their C name minus the type suffix:
    `img.static_config.dss_config__target_total_rate_mcps`. Editing a field only
    touches the host copy; `push()` is what reaches the sensor.
    """

    def __init__(self, platform):
        self.p = platform
        self._blocks = {name: RegBlock(name) for name in BLOCKS}

    def __getattr__(self, key):
        try:
            return self.__dict__['_blocks'][key]
        except KeyError:
            raise AttributeError(f'no register block {key}') from None

    # ── one block ──
    def pull(self, name: str) -> RegBlock:
        """Read one block off the sensor into the image."""
        blk = self._blocks[name]
        return blk.decode(self.p.rd_multi(blk.base, blk.size))

    def push(self, name: str):
        """Write one block from the image to the sensor."""
        blk = self._blocks[name]
        if not BLOCKS[name].writable:
            raise ValueError(f'{name} is read-only')
        self.p.wr_multi(blk.base, blk.encode())

    # ── a run of blocks, as one transfer ──
    @staticmethod
    def _span(names):
        """-> (base, total size) covering the named blocks. Any reserved bytes
        between them are part of the span, exactly as in the C driver."""
        base = min(BLOCKS[n].base for n in names)
        end = max(BLOCKS[n].base + BLOCKS[n].size for n in names)
        return base, end - base

    def pull_range(self, names):
        base, size = self._span(names)
        raw = self.p.rd_multi(base, size)
        for name in names:
            blk = self._blocks[name]
            off = blk.base - base
            blk.decode(raw[off:off + blk.size])

    def push_range(self, names):
        base, size = self._span(names)
        buf = bytearray(size)
        for name in names:
            if not BLOCKS[name].writable:
                raise ValueError(f'{name} is read-only')
            blk = self._blocks[name]
            off = blk.base - base
            buf[off:off + blk.size] = blk.encode()
        self.p.wr_multi(base, bytes(buf))

    # ── diagnostics ──
    def dump(self, names=None) -> str:
        """Every field of the named blocks (default: all), one per line."""
        lines = []
        for name in (names or BLOCKS):
            blk = self._blocks[name]
            lines.append(f'--- {name} @ 0x{blk.base:04X} ({blk.size} B)')
            for field, value in blk.items():
                lines.append(f'  {field:<52} {value}')
        return '\n'.join(lines)
