"""The line rules both file formats share (contract 08 «Line rules», 09).

A line ends at LF only; a line of nothing but JSON whitespace (space, tab,
CR) is blank; every other line is one strict RFC 8259 JSON value — no raw
control characters in strings and no NaN / Infinity — and arrays and
objects nested at most MAX_DEPTH deep.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

JSON_WS = " \t\r\n"

# Contract 08 «Line rules»: the line's outermost value is level 1. The SDKs
# that parse by recursive descent stop there instead of overflowing the stack;
# json.loads would go on to ~1000 and then raise RecursionError.
MAX_DEPTH = 256

_STRING = re.compile(r'"(?:[^"\\]|\\.)*"')


def split_lines(text: str) -> list[str]:
    """`text` cut at LF only (not at CR, VT, FF, U+2028 … as str.splitlines)."""
    return text.split("\n")


def is_blank(line: str) -> bool:
    return not line.strip(JSON_WS)


def _no_constant(name: str) -> Any:
    raise ValueError(f"{name} is not JSON")


def _too_deep(line: str) -> bool:
    """Whether `line` opens more than MAX_DEPTH arrays/objects at once
    (brackets inside strings do not count)."""
    depth = 0
    for c in _STRING.sub("", line):
        if c in "[{":
            depth += 1
            if depth > MAX_DEPTH:
                return True
        elif c in "]}":
            depth -= 1
    return False


def json_object(path: str | Path, lineno: int, line: str) -> dict[str, Any]:
    """`line` as the JSON object a file line must be, else ValueError naming
    the file and line (a damaged file is the caller's bad argument, contract
    07 §3 — never a KeyError / TypeError / AttributeError)."""
    if _too_deep(line):
        raise ValueError(f"{path}:{lineno}: not JSON: nested deeper than {MAX_DEPTH}")
    try:
        obj = json.loads(line, parse_constant=_no_constant)
    except RecursionError:
        raise ValueError(f"{path}:{lineno}: not JSON: nested deeper than {MAX_DEPTH}") from None
    except ValueError as bad:
        raise ValueError(f"{path}:{lineno}: not JSON: {bad}") from None
    if not isinstance(obj, dict):
        raise ValueError(f"{path}:{lineno}: not a JSON object")
    return obj
