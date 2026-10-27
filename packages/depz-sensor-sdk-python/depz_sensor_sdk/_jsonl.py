"""The line rules both file formats share (contract 08 «Line rules», 09).

A line ends at LF only; a line of nothing but JSON whitespace (space, tab,
CR) is blank; every other line is one strict RFC 8259 JSON value — no raw
control characters in strings and no NaN / Infinity.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

JSON_WS = " \t\r\n"


def split_lines(text: str) -> list[str]:
    """`text` cut at LF only (not at CR, VT, FF, U+2028 … as str.splitlines)."""
    return text.split("\n")


def is_blank(line: str) -> bool:
    return not line.strip(JSON_WS)


def _no_constant(name: str) -> Any:
    raise ValueError(f"{name} is not JSON")


def json_object(path: str | Path, lineno: int, line: str) -> dict[str, Any]:
    """`line` as the JSON object a file line must be, else ValueError naming
    the file and line (a damaged file is the caller's bad argument, contract
    07 §3 — never a KeyError / TypeError / AttributeError)."""
    try:
        obj = json.loads(line, parse_constant=_no_constant)
    except ValueError as bad:
        raise ValueError(f"{path}:{lineno}: not JSON: {bad}") from None
    if not isinstance(obj, dict):
        raise ValueError(f"{path}:{lineno}: not a JSON object")
    return obj
