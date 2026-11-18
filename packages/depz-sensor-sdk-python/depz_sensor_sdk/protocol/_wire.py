"""Wire-field checks shared by the codecs (contract 07 §3).

Encoding: every argument is checked against its wire width before a byte is
written — never truncated, wrapped or masked (`ValueError`, which the SDK
reports as `invalid_argument`). Decoding: a payload shorter than the fields
the contract defines is rejected (`ValueError`); trailing bytes are ignored
by the callers (`struct.unpack_from`).
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from typing import Callable, Protocol


def check_uint(name: str, value: int, bits: int) -> int:
    """`value` as an unsigned integer that fits `bits` bits, else ValueError."""
    if not isinstance(value, int) or not 0 <= value < 1 << bits:
        raise ValueError(f"{name}: {value} out of range for u{bits}")
    return int(value)


def check_int(name: str, value: int, bits: int) -> int:
    """`value` as a signed integer that fits `bits` bits, else ValueError."""
    lim = 1 << (bits - 1)
    if not isinstance(value, int) or not -lim <= value < lim:
        raise ValueError(f"{name}: {value} out of range for i{bits}")
    return int(value)


def need_bytes(what: str, payload: bytes, n: int) -> None:
    """Reject a device payload shorter than the `n` bytes its fields need."""
    if len(payload) < n:
        raise ValueError(f"{what}: need {n} bytes, got {len(payload)}")


_UNSIGNED = {"B": 8, "H": 16, "I": 32, "Q": 64}
_SIGNED = {"b": 8, "h": 16, "i": 32, "q": 64}


def checked_pack(fmt: str, **fields: int) -> bytes:
    """`struct.pack(fmt, *fields.values())` with every integer field checked
    against its width first (ValueError naming the field, never struct.error).
    `fmt` is a byte-order prefix plus one integer code per field."""
    codes = fmt.lstrip("<>=!@")
    if len(codes) != len(fields):
        raise TypeError(f"{fmt!r} has {len(codes)} fields, got {len(fields)}")
    for code, (name, value) in zip(codes, fields.items()):
        if code in _UNSIGNED:
            check_uint(name, value, _UNSIGNED[code])
        else:
            check_int(name, value, _SIGNED[code])
    return struct.pack(fmt, *fields.values())


class RegReply(Protocol):
    """What a REG_DATA report decodes to: the bytes read and the MCU time of
    the read — every bridge protocol's `RegData` has these two."""

    @property
    def timestamp_us(self) -> int: ...

    @property
    def data(self) -> bytes: ...


@dataclass(frozen=True)
class BridgeCmds:
    """The register-bridge opcodes of one protocol: what `RegisterBridge`
    (`depz_sensor_sdk/_bridge.py`) sends and expects. One table per bridge
    protocol module, next to the opcodes it names."""

    read_cmd: int
    write_cmd: int
    reg_data_rpt: int
    unpack: Callable[[bytes], RegReply]
    pack_read: Callable[[int, int], bytes]
    pack_write: Callable[[int, bytes], bytes]
    addr_bytes: int  # register address width on the wire (error messages)
