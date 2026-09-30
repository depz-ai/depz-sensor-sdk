#!/usr/bin/env python3
"""Generate the C SDK API reference (Markdown) from the public headers.

The sources of truth are ``include/depz_sensor_sdk.h`` (the codec / decode
layer) and ``include/depz_sensor_io.h`` (the live-hardware layer): this walks
their declarations (functions, ``typedef`` structs/enums, function-pointer typedefs
and ``#define`` constants) together with their doc-comments, and emits, in the
SAME shape as the Python SDK's ``docs/api.md``:

- ``docs/api.md`` — the full reference: a title, a Contents index, one
  ``## <Domain>`` section per header banner (Discovery / Transport / Common
  protocol / SR04 / VL53L4 / VL53L8 / BNO086 / Bootloader / Datasets, then
  the live layer's Errors / Byte links / Streams / Device core / Discovery /
  SR04 / VL53L4CD / VL53L8 / VL53L 1D family / BNO055 / BNO086 sensor
  classes), and per-symbol ``### name`` + a fenced ```c signature``` block + the doc text;
- ``docs/<sensor>/api.md`` — one focused reference per sensor, carrying only
  that sensor's own symbols (the shared surface stays in the root). The one
  VL53L8 ToF domain is split by symbol into the CX base and the CH superset
  (the CNH decode), exactly like the Python generator splits the ToF class.
  Boards that share one firmware share one header section: the VL53L5CX /
  VL53L7CX / VL53L7CH section feeds all three L5/L7 pages (the L7CH page adds
  the CNH decode), and the VL53L 1D family section feeds the VL53L0X,
  VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX pages. The live layer's
  sensor-class sections join their sensor's page (SR04, VL53L4CD, BNO055, BNO086, and the
  VL53L 1D family class on its five pages); the one
  multizone class serves the VL53L8CX / CH and the L5/L7 boards, so it joins
  the VL53L8CX page whole, the VL53L8CH page gets its CNH-setup subset and the
  L5/L7 pages their board commands. Its generic sections (errors,
  links, streams, device core, discovery) stay in the root.

Doc-comments in the header are authoritative, so this reference never drifts
from the code. Regenerate with ``make docs`` (or ``python3 scripts/gen_api_md.py``).
Python standard library only — no dependencies.
"""

from __future__ import annotations

import re
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
HEADER = HERE / "include" / "depz_sensor_sdk.h"
IO_HEADER = HERE / "include" / "depz_sensor_io.h"
DOCS = HERE / "docs"

# Domain key -> section title, in reading order. The key is assigned per symbol
# from the header banner it lives under (see _domain_for_banner).
DOMAIN_ORDER = [
    "discovery", "transport", "common", "sr04",
    "vl53l4", "vl53l8", "vl53l7", "vl53lx", "bno086", "bno055",
    "bootloader", "dataset",
    # depz_sensor_io.h — the live-hardware layer
    "io_errors", "io_links", "io_streams", "io_device", "io_discovery", "sr04_io",
    "vl53l4_io", "vl53l8_io", "vl53lx_io", "bno055_io", "bno086_io",
]
DOMAIN_TITLE = {
    "discovery": "Discovery",
    "transport": "Transport",
    "common": "Common protocol",
    "sr04": "SR04",
    "vl53l4": "VL53L4CD (ToF)",
    "vl53l8": "VL53L8 (ToF)",
    "vl53l7": "VL53L5CX / VL53L7CX / VL53L7CH (ToF)",
    "vl53lx": "VL53L 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX)",
    "bno086": "BNO086 (IMU)",
    "bno055": "BNO055 (IMU)",
    "bootloader": "Bootloader / firmware update",
    "dataset": "Datasets (record & replay)",
    "io_errors": "Live layer: errors",
    "io_links": "Live layer: byte links",
    "io_streams": "Live layer: streams",
    "io_device": "Live layer: device core",
    "io_discovery": "Live layer: discovery",
    "sr04_io": "SR04 sensor class (live layer)",
    "vl53l4_io": "VL53L4CD sensor class (live layer)",
    "vl53l8_io": "VL53L8 multizone sensor class (live layer)",
    "bno055_io": "BNO055 sensor class (live layer)",
    "bno086_io": "BNO085 / BNO086 sensor class (live layer)",
    "vl53lx_io": "VL53L 1D family sensor class (live layer)",
}

# The VL53L8 ToF domain is ONE header section but TWO sensors — the CX base and
# the CH superset. CH silicon shares the entire CX decode surface; the
# CH-anchored symbols are the variant enum that names the split and the CNH
# (compact-network-histogram) decode. Everything else ToF is CX-base and shared.
VL53L8CNH_SYMBOLS = {
    "DEPZ_VL53L8_CNH_MAX_AGGREGATES", "DEPZ_VL53L8_CNH_MAX_FEATURE",
    "depz_vl53l8ch_cnh_config", "depz_vl53l8ch_cnh_frame",
    "depz_vl53l8ch_decode_cnh",
}
VL53L8CH_SYMBOLS = {"depz_vl53l8_variant", "depz_vl53l8ch_decode_frame"} | VL53L8CNH_SYMBOLS
# The live VL53L8 class (depz_sensor_io.h) is one implementation for both
# models: it lands whole on the CX page; the CH page repeats the model enum and
# the CNH setup — the part only a VL53L8CH runs.
VL53L8CH_IO_SYMBOLS = {
    "depz_vl53l8_model", "DEPZ_VL53L8_CNH_MAX_BYTES", "depz_vl53l8_cnh_setup",
    "depz_vl53l8_cnh_init_config", "depz_vl53l8_cnh_create_agg_map",
    "depz_vl53l8_cnh_required_memory", "depz_vl53l8_cnh_pack",
    "depz_vl53l8_cnh_decode_config", "depz_vl53l8_configure_cnh",
}
# The same class also serves the L5/L7 boards; their board commands are no
# VL53L8 API and go to the L5/L7 pages instead of the VL53L8CX one.
VL53L7_IO_SYMBOLS = {
    "depz_vl53l8_module_type", "depz_vl53l7_bridge_info",
    "depz_vl53l7_set_i2c_speed_khz", "depz_vl53l7_pin_ctrl",
}

# Boards that share one firmware (and so one header section).
VL53L7_BOARDS = [
    ("vl53l5cx", "VL53L5CX (ToF, 8×8, 63°)"),
    ("vl53l7cx", "VL53L7CX (ToF, 8×8, 90°)"),
    ("vl53l7ch", "VL53L7CH (ToF + CNH)"),
]
VL53LX_BOARDS = [
    ("vl53l0x", "VL53L0X (ToF, single-zone)"),
    ("vl53l1cx", "VL53L1CX (ToF, single-zone)"),
    ("vl53l1cb", "VL53L1CB (ToF, single-zone)"),
    ("vl53l3cx", "VL53L3CX (ToF, multi-target)"),
    ("vl53l4cx", "VL53L4CX (ToF, multi-target)"),
]


def _domain_for_banner(title: str, current: str) -> str:
    """Map a header banner title to a domain key (keeps `current` if unknown)."""
    t = title
    # Checked first: these banners mention their VL53L4 / VL53L8 parents.
    if "BNO055" in t:
        return "bno055"
    if "VL53L 1D family" in t:
        return "vl53lx"
    if "VL53L5CX / VL53L7CX" in t:
        return "vl53l7"
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
    if "VL53L4" in t:
        return "vl53l4"
    if "VL53L8" in t:
        return "vl53l8"
    if "BNO086" in t:
        return "bno086"
    if ".depzdata" in t or "dataset" in t:
        return "dataset"
    return current


def _io_domain_for_banner(title: str, current: str) -> str:
    """Map a depz_sensor_io.h banner title to a domain key."""
    t = title
    # Checked first: this banner names VL53L4CX / VL53L4CD too.
    if "1D ToF family" in t:
        return "vl53lx_io"
    if "SR04" in t:
        return "sr04_io"
    if "VL53L4CD" in t:
        return "vl53l4_io"
    if "VL53L8" in t:
        return "vl53l8_io"
    if "BNO055" in t:
        return "bno055_io"
    if "BNO086" in t:
        return "bno086_io"
    if t.startswith("Errors"):
        return "io_errors"
    if t.startswith("Byte links"):
        return "io_links"
    if "streams" in t:
        return "io_streams"
    if t.startswith("Device core"):
        return "io_device"
    if t.startswith("Discovery"):
        return "io_discovery"
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
_SKIP_PREFIXES = ("#include", "#ifndef", "#ifdef", "#endif", 'extern "C"')


def _func_name(sig: str) -> str:
    """Identifier immediately before the first ``(`` in a function decl."""
    head = sig.split("(", 1)[0]
    ids = re.findall(r"[A-Za-z_]\w*", head)
    return ids[-1] if ids else ""


_INCLUDE_GUARD = re.compile(r"#define\s+DEPZ_SENSOR_\w+_H\s*$")


def parse_header(path: Path, banner_domain=_domain_for_banner,
                 domain: str = "transport") -> list[dict]:
    """Parse a public header into ordered symbol records:
    ``{name, kind, sig, doc, domain}``. `banner_domain` maps each ``===``
    banner title to a domain key; `domain` applies before the first banner."""
    lines = path.read_text().split("\n")
    n = len(lines)
    symbols: list[dict] = []
    pending_doc: str | None = None
    i = 0
    while i < n:
        raw = lines[i]
        stripped = raw.strip()

        if not stripped or stripped.startswith(_SKIP_PREFIXES) \
                or stripped.startswith("}") \
                or _INCLUDE_GUARD.match(stripped):
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
            # A banner is a rule of '=' signs; a doc-comment may itself say
            # `x == 0`, which must not open a new section.
            if re.search(r"={8,}", full):
                title_lines = [ln.strip() for ln in _clean_comment(full).split("\n")
                               if ln.strip() and set(ln.strip()) - set("= ")]
                domain = banner_domain(" ".join(title_lines), domain)
                pending_doc = None
            else:
                pending_doc = _clean_comment(full)
            continue

        # ---- declarations -------------------------------------------------
        if stripped.startswith("#define"):
            m = re.match(r"#define\s+(\w+)", stripped)
            name = m.group(1)
            body = raw
            # a trailing comment may run on to further lines
            if "/*" in body and "*/" not in body[body.index("/*"):]:
                while i + 1 < n:
                    i += 1
                    body += "\n" + lines[i]
                    if "*/" in lines[i]:
                        break
            doc = pending_doc
            tc = re.search(r"/\*(.*?)\*/", body, re.S)
            if tc:
                inline = re.sub(r"\s+", " ", tc.group(1)).strip()
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

        if stripped.startswith("extern"):
            # exported constant table, e.g. extern const uint8_t X[8][2];
            buf = [raw]
            while ";" not in buf[-1] and i + 1 < n:
                i += 1
                buf.append(lines[i])
            sig = re.sub(r"\s+", " ", " ".join(x.strip() for x in buf)).strip()
            mname = re.search(r"(\w+)\s*(\[[^;]*)?;", sig)
            if mname:
                symbols.append({"name": mname.group(1), "kind": "variable",
                                "sig": sig, "doc": pending_doc, "domain": domain})
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
    if by_domain.get("sr04") and by_domain.get("sr04_io"):
        targets.append(("sr04", "SR04", [
            ("sr04", "SR04", by_domain["sr04"]),
            ("sr04_io", DOMAIN_TITLE["sr04_io"], by_domain["sr04_io"]),
        ], []))
    elif by_domain.get("sr04"):
        targets.append(("sr04", "SR04", by_domain["sr04"], []))
    if by_domain.get("vl53l4") and by_domain.get("vl53l4_io"):
        targets.append(("vl53l4cd", "VL53L4CD (ToF, single-zone)", [
            ("vl53l4", DOMAIN_TITLE["vl53l4"], by_domain["vl53l4"]),
            ("vl53l4_io", DOMAIN_TITLE["vl53l4_io"], by_domain["vl53l4_io"]),
        ], []))
    elif by_domain.get("vl53l4"):
        targets.append(("vl53l4cd", "VL53L4CD (ToF, single-zone)",
                        by_domain["vl53l4"], []))
    if by_domain.get("vl53l8"):
        tof = by_domain["vl53l8"]
        io = [s for s in by_domain.get("vl53l8_io", []) if s["name"] not in VL53L7_IO_SYMBOLS]
        cx = [s for s in tof if s["name"] not in VL53L8CH_SYMBOLS]
        ch = [s for s in tof if s["name"] in VL53L8CH_SYMBOLS]
        ch_io = [s for s in io if s["name"] in VL53L8CH_IO_SYMBOLS]
        cx_groups = [("vl53l8", "VL53L8CX (ToF)", cx)]
        ch_groups = [("vl53l8", "VL53L8CH (ToF + CNH)", ch)]
        cx_note: list[str] = []
        if io:
            cx_groups.append(("vl53l8_io", DOMAIN_TITLE["vl53l8_io"], io))
            ch_groups.append(("vl53l8_io", "VL53L8CH CNH setup (live layer)", ch_io))
            cx_note = [
                "The sensor class below is one implementation for every",
                "multizone model: the VL53L8CH uses every call of it too, plus",
                "the CNH setup, and so do the VL53L5CX / L7CX / L7CH boards,",
                "plus their board commands (on their own pages).",
                "",
            ]
        targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx_groups, cx_note))
        targets.append((
            "vl53l8ch", "VL53L8CH (ToF + CNH)", ch_groups,
            ["`depz_vl53l8_model` picks the live class's model (the firmware",
             "blob it downloads); `depz_vl53l8_variant` names the CX/CH split",
             "for the decode layer. The VL53L8CH shares the ENTIRE VL53L8CX",
             "surface — the sensor class, chunk parse, frame reassembly and the",
             "advanced DCI codecs — documented in the",
             "[VL53L8CX API reference](../vl53l8cx/api.md). CH's own additions",
             "are the CH frame decoder, the CNH (compact-network-histogram)",
             "decode and the live CNH setup below.",
             ""],
        ))
    if by_domain.get("vl53l7"):
        l7 = by_domain["vl53l7"]
        cnh = [s for s in by_domain.get("vl53l8", []) if s["name"] in VL53L8CNH_SYMBOLS]
        l7_io = [s for s in by_domain.get("vl53l8_io", []) if s["name"] in VL53L7_IO_SYMBOLS]
        shared = [
            "One board firmware (`APP_VL53L7`) serves the VL53L5CX, VL53L7CX and",
            "VL53L7CH, so the three share this section. Chunk parse, frame",
            "reassembly and the `depz_vl53l8_frame` result struct are the VL53L8",
            "ones — see the [VL53L8CX API reference](../vl53l8cx/api.md).",
        ]
        for folder, title in VL53L7_BOARDS:
            groups = [("vl53l7", DOMAIN_TITLE["vl53l7"], l7)]
            note = list(shared)
            if folder == "vl53l7ch" and cnh:
                groups.append(("cnh", "CNH decode (shared with VL53L8CH)", cnh))
                note += ["The CNH histogram decode is the VL53L8CH one, listed",
                         "below as well."]
            if l7_io:
                groups.append(("vl53l7_io", "L5/L7 board commands (live layer)", l7_io))
                note += ["The live layer drives these boards with the VL53L8",
                         "sensor class (`depz_vl53l8_*`, see the VL53L8CX",
                         "reference); only the board commands below are",
                         "L5/L7-specific."]
            targets.append((folder, title, groups, note + [""]))
    if by_domain.get("vl53lx"):
        lx = by_domain["vl53lx"]
        note = [
            "One bridge firmware (`APP_VL53L0_4`) serves the whole VL53L 1D",
            "family, so VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX share",
            "this section. READ_REG / WRITE_REG / XSHUT / STOP_STREAM /",
            "SET_I2C_SPEED, the REG_DATA / STREAM reports and the",
            "`depz_vl53l4_result` struct are the VL53L4CD codecs — see the",
            "[VL53L4CD API reference](../vl53l4cd/api.md).",
            "",
        ]
        live = by_domain.get("vl53lx_io")
        if live:
            note = note[:-1] + [
                "The live sensor class (`depz_sensor_io.h`, `depz_vl53lx_*`) follows",
                "the codecs; one class serves every product of the family.",
                "",
            ]
        for folder, title in VL53LX_BOARDS:
            groups = [("vl53lx", DOMAIN_TITLE["vl53lx"], lx)]
            if live:
                groups.append(("vl53lx_io", DOMAIN_TITLE["vl53lx_io"], live))
            targets.append((folder, title, groups, note))
    if by_domain.get("bno086") and by_domain.get("bno086_io"):
        targets.append(("bno086", "BNO086 (IMU)", [
            ("bno086", DOMAIN_TITLE["bno086"], by_domain["bno086"]),
            ("bno086_io", DOMAIN_TITLE["bno086_io"], by_domain["bno086_io"]),
        ], [
            "The codecs come first (`depz_sensor_sdk.h`: SHTP framing, SH-2",
            "encoders and parsers, input reports, scaling); the live sensor",
            "class (`depz_sensor_io.h`) is built from them and follows. One",
            "class serves the BNO085 and the BNO086.",
            "",
        ]))
    elif by_domain.get("bno086"):
        targets.append(("bno086", "BNO086 (IMU)", by_domain["bno086"], []))
    if by_domain.get("bno055") and by_domain.get("bno055_io"):
        targets.append(("bno055", "BNO055 (IMU)", [
            ("bno055", DOMAIN_TITLE["bno055"], by_domain["bno055"]),
            ("bno055_io", DOMAIN_TITLE["bno055_io"], by_domain["bno055_io"]),
        ], [
            "The codecs come first (`depz_sensor_sdk.h`: wire commands and",
            "reports, register codecs, window decode); the live sensor class",
            "(`depz_sensor_io.h`) is built from them and follows.",
            "",
        ]))
    elif by_domain.get("bno055"):
        targets.append(("bno055", "BNO055 (IMU)", by_domain["bno055"], []))
    return targets


def main() -> None:
    symbols = parse_header(HEADER)
    if IO_HEADER.exists():
        symbols += parse_header(IO_HEADER, _io_domain_for_banner, "io_errors")
    by_domain: dict[str, list[dict]] = {}
    for s in symbols:
        by_domain.setdefault(s["domain"], []).append(s)

    ordered = DOMAIN_ORDER + [d for d in by_domain if d not in DOMAIN_ORDER]
    groups = [(d, DOMAIN_TITLE.get(d, d.title()), by_domain[d])
              for d in ordered if by_domain.get(d)]

    root_head = [
        "# API reference",
        "",
        "Auto-generated from the public headers `include/depz_sensor_sdk.h`",
        "(codecs) and `include/depz_sensor_io.h` (live layer) — their",
        "declarations and doc-comments — by `scripts/gen_api_md.py`; run",
        "`make docs` to regenerate. Edit the doc-comments in the headers, not",
        "this file.",
        "",
        "Each sensor also has a focused reference with just its own symbols:",
        "[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · "
        "[VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · "
        "[VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · "
        "[VL53L7CH](vl53l7ch/api.md) · [VL53L0X](vl53l0x/api.md) · "
        "[VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · "
        "[VL53L3CX](vl53l3cx/api.md) · [VL53L4CX](vl53l4cx/api.md) · "
        "[BNO086](bno086/api.md) · [BNO055](bno055/api.md).",
        "",
    ]
    DOCS.mkdir(parents=True, exist_ok=True)
    (DOCS / "api.md").write_text(_render_reference(groups, root_head))
    print(f"wrote {DOCS / 'api.md'} ({len(symbols)} symbols across {len(groups)} groups)")

    for folder, title, syms, note_lines in _sensor_targets(by_domain):
        # `syms` is a symbol list (one group titled like the sensor) or a
        # ready list of (domain, title, symbols) groups.
        if syms and isinstance(syms[0], tuple):
            groups_out = syms
        else:
            groups_out = [(folder, title, syms)]
        n_syms = sum(len(g[2]) for g in groups_out)
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
        out.write_text(_render_reference(groups_out, sub_head))
        print(f"wrote {out} ({n_syms} symbols)")


if __name__ == "__main__":
    main()
