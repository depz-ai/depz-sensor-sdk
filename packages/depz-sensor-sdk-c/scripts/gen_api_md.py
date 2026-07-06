#!/usr/bin/env python3
"""Generate the C SDK API reference (Markdown) from the public header.

The single source of truth is ``include/depz_sensor_sdk.h``: this walks its
declarations (functions, ``typedef`` structs/enums, function-pointer typedefs
and ``#define`` constants) together with their doc-comments, and emits, in the
SAME shape as the Python SDK's ``docs/api.md``:

- ``docs/api.md`` — the full reference: a title, a Contents index, one
  ``## <Domain>`` section per header banner (Discovery / Transport / Common
  protocol / SR04 / VL53L8 / BNO086 / Bootloader / Datasets), and per-symbol
  ``### name`` + a fenced ```c signature``` block + the doc text;
- ``docs/{sr04,vl53l8cx,vl53l8ch,bno086}/api.md`` — one focused reference per
  sensor, carrying only that sensor's own symbols (the shared surface stays in
  the root). The one VL53L8 ToF domain is split by symbol into the CX base and
  the CH superset, exactly like the Python generator splits the ToF class.

Doc-comments in the header are authoritative, so this reference never drifts
from the code. Regenerate with ``make docs`` (or ``python3 scripts/gen_api_md.py``).
Python standard library only — no dependencies.
"""

from __future__ import annotations

import re
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
HEADER = HERE / "include" / "depz_sensor_sdk.h"
DOCS = HERE / "docs"

# Domain key -> section title, in reading order. The key is assigned per symbol
# from the header banner it lives under (see _domain_for_banner).
DOMAIN_ORDER = [
    "discovery", "transport", "common", "sr04",
    "vl53l8", "bno086", "bootloader", "dataset",
]
DOMAIN_TITLE = {
    "discovery": "Discovery",
    "transport": "Transport",
    "common": "Common protocol",
    "sr04": "SR04",
    "vl53l8": "VL53L8 (ToF)",
    "bno086": "BNO086 (IMU)",
    "bootloader": "Bootloader / firmware update",
    "dataset": "Datasets (record & replay)",
}

# The VL53L8 ToF domain is ONE header section but TWO sensors — the CX base and
# the CH superset. CH silicon shares the entire CX decode surface; the only
# CH-anchored symbol today is the variant enum that names the split (CNH decode
# is a documented, not-yet-implemented extension point). Everything else ToF is
# CX-base and shared.
VL53L8CH_SYMBOLS = {"depz_vl53l8_variant"}


def _domain_for_banner(title: str, current: str) -> str:
    """Map a header banner title to a domain key (keeps `current` if unknown)."""
    t = title
    if "USB identity" in t or "Firmware-name identity" in t:
        return "discovery"
    if "CRC algorithms" in t or "Packet framing" in t:
        return "transport"
    if "Common command" in t:
        return "common"
    if "SR04" in t:
        return "sr04"
    if ".fwdepz" in t or "Bootloader" in t:
        return "bootloader"
    if "VL53L8" in t:
        return "vl53l8"
    if "BNO086" in t:
        return "bno086"
    if ".depzdata" in t or "dataset" in t:
        return "dataset"
    return current


# --------------------------------------------------------------------------
# Comment cleaning

def _clean_comment(text: str) -> str:
    """Strip C block-comment framing (``/*``, ``*/``, leading ``*``) and trim."""
    out = []
    for ln in text.split("\n"):
        s = ln.strip()
        if s.startswith("* "):
            s = s[2:]
        elif s == "*":
            s = ""
        elif s.startswith("*"):
            s = s[1:]
        out.append(s.rstrip())
    while out and not out[0]:
        out.pop(0)
    while out and not out[-1]:
        out.pop()
    return "\n".join(out)


# --------------------------------------------------------------------------
# Header parsing

def _read_comment(lines: list[str], i: int) -> tuple[str, int]:
    """Read one ``/* ... */`` comment (single- or multi-line) starting at `i`.
    Returns the inner text (between the delimiters) and the next line index."""
    buf = []
    while i < len(lines):
        buf.append(lines[i])
        if "*/" in lines[i]:
            i += 1
            break
        i += 1
    raw = "\n".join(buf)
    inner = raw[raw.index("/*") + 2:]
    inner = inner[:inner.rindex("*/")]
    return inner, i


_TRAILING_COMMENT = re.compile(r"/\*.*?\*/\s*$", re.S)
_SKIP_PREFIXES = ("#include", "#ifndef", "#ifdef", "#endif", "extern")


def _func_name(sig: str) -> str:
    """Identifier immediately before the first ``(`` in a function decl."""
    head = sig.split("(", 1)[0]
    ids = re.findall(r"[A-Za-z_]\w*", head)
    return ids[-1] if ids else ""


def parse_header(path: Path) -> list[dict]:
    """Parse the public header into ordered symbol records:
    ``{name, kind, sig, doc, domain}``."""
    lines = path.read_text().split("\n")
    n = len(lines)
    symbols: list[dict] = []
    domain = "transport"     # first banner (CRCs) sets this anyway
    pending_doc: str | None = None
    i = 0
    while i < n:
        raw = lines[i]
        stripped = raw.strip()

        if not stripped or stripped.startswith(_SKIP_PREFIXES) \
                or stripped.startswith("}") \
                or stripped == "#define DEPZ_SENSOR_SDK_H":
            pending_doc = None
            i += 1
            continue

        # A run of full-line comments: either a banner (=== rules) or a doc.
        if stripped.startswith("/*"):
            run = []
            while i < n and lines[i].lstrip().startswith("/*"):
                ctext, i = _read_comment(lines, i)
                run.append(ctext)
            full = "\n".join(run)
            if "==" in full:
                title_lines = [ln.strip() for ln in _clean_comment(full).split("\n")
                               if ln.strip() and set(ln.strip()) - set("= ")]
                domain = _domain_for_banner(" ".join(title_lines), domain)
                pending_doc = None
            else:
                pending_doc = _clean_comment(full)
            continue

        # ---- declarations -------------------------------------------------
        if stripped.startswith("#define"):
            m = re.match(r"#define\s+(\w+)", stripped)
            name = m.group(1)
            body = raw
            doc = pending_doc
            tc = re.search(r"/\*(.*?)\*/", body)
            if tc:
                inline = tc.group(1).strip()
                body = body[:tc.start()].rstrip()
                doc = (doc + "\n" + inline) if doc else inline
            symbols.append({"name": name, "kind": "macro",
                            "sig": body.strip(), "doc": doc, "domain": domain})
            pending_doc = None
            i += 1
            continue

        if stripped.startswith("typedef") and "(*" in stripped:
            # function-pointer typedef, single line
            buf = [raw]
            while ";" not in buf[-1] and i + 1 < n:
                i += 1
                buf.append(lines[i])
            sig = re.sub(r"\s+", " ", " ".join(x.strip() for x in buf)).strip()
            name = re.search(r"\(\*(\w+)\)", sig).group(1)
            symbols.append({"name": name, "kind": "typedef",
                            "sig": sig, "doc": pending_doc, "domain": domain})
            pending_doc = None
            i += 1
            continue

        if stripped.startswith("typedef") and "{" in stripped:
            # struct/enum with a body: accumulate until the closing "} name;"
            buf = [raw]
            while "}" not in buf[-1]:
                i += 1
                buf.append(lines[i])
            close = buf[-1]
            name = re.search(r"\}\s*(\w+)\s*;", close).group(1)
            kind = "enum" if "enum" in stripped else "struct"
            block = "\n".join(x.rstrip() for x in buf)
            symbols.append({"name": name, "kind": kind,
                            "sig": block, "doc": pending_doc, "domain": domain})
            pending_doc = None
            i += 1
            continue

        if stripped.startswith("typedef"):
            # opaque one-line typedef, e.g. typedef struct X X;
            name = re.findall(r"[A-Za-z_]\w*", stripped.rstrip(";"))[-1]
            symbols.append({"name": name, "kind": "typedef",
                            "sig": stripped.rstrip(), "doc": pending_doc,
                            "domain": domain})
            pending_doc = None
            i += 1
            continue

        # Otherwise: a function declaration, possibly multi-line, until ';'.
        buf = [raw]
        while ";" not in buf[-1] and i + 1 < n:
            i += 1
            buf.append(lines[i])
        joined = " ".join(x.strip() for x in buf)
        doc = pending_doc
        tc = _TRAILING_COMMENT.search(joined)
        if tc:
            inline = _clean_comment(joined[tc.start():]).strip()
            joined = joined[:tc.start()].strip()
            doc = (doc + "\n" + inline) if doc else inline
        sig = re.sub(r"\s+", " ", joined).strip()
        name = _func_name(sig)
        if name and "(" in sig:
            symbols.append({"name": name, "kind": "function",
                            "sig": sig, "doc": doc, "domain": domain})
        pending_doc = None
        i += 1

    return symbols


# --------------------------------------------------------------------------
# Rendering

def _anchor(text: str) -> str:
    a = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"\s+", "-", a)


def _render_symbol(sym: dict) -> list[str]:
    out = [f"### {sym['name']}", "", "```c", sym["sig"], "```", ""]
    if sym["doc"]:
        out += [sym["doc"], ""]
    return out


def _render_reference(groups: list[tuple[str, str, list[dict]]],
                      header_lines: list[str]) -> str:
    """`groups` is `[(domain, title, symbols), ...]`."""
    head = list(header_lines) + ["## Contents", ""]
    for _domain, title, syms in groups:
        links = ", ".join(f"[`{s['name']}`](#{_anchor(s['name'])})" for s in syms)
        head.append(f"- **{title}**: {links}")
    head.append("")

    body: list[str] = []
    for _domain, title, syms in groups:
        body += [f"## {title}", ""]
        for s in syms:
            body += _render_symbol(s)
    return "\n".join(head + body).rstrip() + "\n"


def _sensor_targets(by_domain: dict[str, list[dict]]):
    """`[(folder, title, symbols, note_lines), ...]` — one per per-sensor api.md.
    The ToF domain is split by symbol into the CX base and the CH superset."""
    targets = []
    if by_domain.get("sr04"):
        targets.append(("sr04", "SR04", by_domain["sr04"], []))
    if by_domain.get("vl53l8"):
        tof = by_domain["vl53l8"]
        cx = [s for s in tof if s["name"] not in VL53L8CH_SYMBOLS]
        ch = [s for s in tof if s["name"] in VL53L8CH_SYMBOLS]
        targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx, []))
        targets.append((
            "vl53l8ch", "VL53L8CH (ToF + CNH)", ch,
            ["`depz_vl53l8_variant` names the CX/CH split. The VL53L8CH silicon",
             "shares the ENTIRE CX register-bridge decode surface — chunk parse,",
             "frame reassembly, the ranging-frame decoder and the advanced DCI",
             "codecs are all CX/CH-shared and documented in the",
             "[VL53L8CX API reference](../vl53l8cx/api.md). CH's own addition —",
             "CNH (compact-network-histogram) decode — is a documented,",
             "not-yet-implemented extension point, so no CH-exclusive decode",
             "symbols exist yet.",
             ""],
        ))
    if by_domain.get("bno086"):
        targets.append(("bno086", "BNO086 (IMU)", by_domain["bno086"], []))
    return targets


def main() -> None:
    symbols = parse_header(HEADER)
    by_domain: dict[str, list[dict]] = {}
    for s in symbols:
        by_domain.setdefault(s["domain"], []).append(s)

    ordered = DOMAIN_ORDER + [d for d in by_domain if d not in DOMAIN_ORDER]
    groups = [(d, DOMAIN_TITLE.get(d, d.title()), by_domain[d])
              for d in ordered if by_domain.get(d)]

    root_head = [
        "# API reference",
        "",
        "Auto-generated from the public header `include/depz_sensor_sdk.h`",
        "(its declarations and doc-comments) by `scripts/gen_api_md.py` — run",
        "`make docs` to regenerate. Edit the doc-comments in the header, not",
        "this file.",
        "",
        "Each sensor also has a focused reference with just its own symbols:",
        "[SR04](sr04/api.md) · [VL53L8CX](vl53l8cx/api.md) · "
        "[VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).",
        "",
    ]
    DOCS.mkdir(parents=True, exist_ok=True)
    (DOCS / "api.md").write_text(_render_reference(groups, root_head))
    print(f"wrote {DOCS / 'api.md'} ({len(symbols)} symbols across {len(groups)} groups)")

    for folder, title, syms, note_lines in _sensor_targets(by_domain):
        sub_head = [
            f"# {title} — API reference",
            "",
            f"The public API for the {title} sensor. Transport, discovery,",
            "the common command/report codecs and other cross-sensor symbols",
            "live in the [top-level API reference](../api.md).",
            "",
            *note_lines,
            "Auto-generated by `scripts/gen_api_md.py` — edit the doc-comments",
            "in the header, not this file.",
            "",
        ]
        out = DOCS / folder / "api.md"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(_render_reference([(folder, title, syms)], sub_head))
        print(f"wrote {out} ({len(syms)} symbols)")


if __name__ == "__main__":
    main()
