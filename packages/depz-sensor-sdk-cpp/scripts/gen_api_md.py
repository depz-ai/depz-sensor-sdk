"""Generate the C++ SDK API reference (Markdown) from the public headers.

The public contract of the C++ SDK is the set of headers under
``include/depz/`` (the ``detail/`` subdirectory is private, and
``span.hpp`` is a vendored ``std::span`` shim documented in the guide, not
here). This walks those headers, extracts every namespace-scope
declaration together with its ``//`` doc-comment, and writes:

- ``docs/api.md`` — the full reference, grouped by header/domain, in the
  same shape as the Python SDK's ``docs/api.md`` (title, a Contents index
  of ``## Domain`` sections, then ``### name`` + a fenced ``cpp`` signature
  + the doc-comment);
- ``docs/<sensor>/api.md`` — one focused reference per sensor, so each
  sensor has a full intro/guide/api set. The shared surface (transport,
  common protocol, identity, …) stays only in the root reference. The single
  ``vl53l8`` domain is TWO sensors — the CX base and the CH superset — so it
  is split by symbol name: CH-only symbols (the CH footer offset and the CNH
  decode) land in ``vl53l8ch``, everything else in ``vl53l8cx``. Boards that
  share one firmware share one header: ``vl53l7.hpp`` feeds the VL53L5CX /
  VL53L7CX / VL53L7CH pages (the L7CH page adds the CNH decode) and
  ``vl53lx.hpp`` the VL53L0X / L1CX / L1CB / L3CX / L4CX pages. The
  live-hardware header ``device.hpp`` is split the same way: links, the
  device core, streams, errors and discovery stay in the root reference,
  and each sensor class is also listed on its sensor's page next to the
  codecs: ``Sr04`` / ``Sr04Measurement`` / ``open_sr04`` on the SR04 page,
  ``Vl53l4cd`` / ``Vl53l4cdMeasurement`` / ``DetectionThresholds`` /
  ``DetectionWindow`` / ``open_vl53l4cd`` on the VL53L4CD page, and the one
  VL53L8 class split like its codecs: ``Vl53l8`` / ``Vl53l8Model`` /
  ``Vl53l8Motion`` / ``Vl53l8LiveFrame`` / ``Vl53l8Progress`` /
  ``open_vl53l8`` on the VL53L8CX page, the CH-only ``CnhSetup`` on the
  VL53L8CH and VL53L7CH pages. The L5/L7 pages link the shared class.
  ``Bno055`` / ``Bno055Sample`` / ``Bno055Status`` / ``open_bno055`` go on
  the BNO055 page, ``Bno086`` / ``Bno086Report`` / ``open_bno086`` and the
  ``depz::bno086`` value types on the BNO086 page, ``Vl53lx`` /
  ``Vl53lxMeasurement`` / ``Vl53lxTarget`` / ``Vl53lxBins`` / ``Vl53lxCap`` /
  ``open_vl53lx`` on each of the five VL53L 1D-family pages.

The doc-comments in the headers are the single source of truth, so the
reference never drifts from the code. Regenerate with

    cmake --build build --target docs        # or:
    python3 scripts/gen_api_md.py

Stdlib only — no doc-tool dependencies, no compiler required (it is a
lightweight declaration parser, not a full C++ front end; it understands
exactly the constructs the DEPZ headers use).
"""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = ROOT / "include" / "depz"
DOCS = ROOT / "docs"

# Header -> domain key. ``detail/`` and ``span.hpp`` are intentionally omitted
# (private / vendored shim). Files are processed in this order; a domain that
# spans two files (transport = framing + crc) keeps that order.
FILE_DOMAIN = [
    ("framing.hpp", "transport"),
    ("crc.hpp", "transport"),
    ("common.hpp", "common"),
    ("identity.hpp", "identity"),
    ("usb_ids.hpp", "usb_ids"),
    ("sr04.hpp", "sr04"),
    ("fwdepz.hpp", "bootloader"),
    ("vl53l4.hpp", "vl53l4"),
    ("vl53l8.hpp", "vl53l8"),
    ("vl53l7.hpp", "vl53l7"),
    ("vl53lx.hpp", "vl53lx"),
    ("bno086.hpp", "bno086"),
    ("bno055.hpp", "bno055"),
    ("dataset.hpp", "dataset"),
    ("device.hpp", "device"),
]

DOMAIN_ORDER = [
    "transport", "common", "identity", "usb_ids", "sr04",
    "vl53l4", "vl53l8", "vl53l7", "vl53lx", "bno086", "bno055",
    "bootloader", "dataset", "device",
]
DOMAIN_TITLE = {
    "transport": "Transport (framing & CRC)",
    "common": "Common protocol",
    "identity": "Identity",
    "usb_ids": "USB identity",
    "sr04": "SR04",
    "vl53l4": "VL53L4CD (ToF)",
    "vl53l8": "VL53L8 (ToF)",
    "vl53l7": "VL53L5CX / VL53L7CX / VL53L7CH (ToF)",
    "vl53lx": "VL53L 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX)",
    "bno086": "BNO086 (IMU)",
    "bno055": "BNO055 (IMU)",
    "bootloader": "Bootloader / firmware",
    "dataset": "Datasets",
    "device": "Live hardware (links, device, discovery)",
}

# The sensor classes of the live-hardware header: also listed on their
# sensor's page (the rest of device.hpp is cross-sensor and stays in the root
# reference only).
SR04_LIVE_SYMBOLS = {"Sr04Measurement", "Sr04", "open_sr04"}
VL53L4CD_LIVE_SYMBOLS = {
    "Vl53l4cdMeasurement", "DetectionWindow", "DetectionThresholds",
    "Vl53l4cd", "open_vl53l4cd",
}
# One live class serves the VL53L8CX and the VL53L8CH: it goes on the CX page
# with the shared decode surface; the CH-only CNH setup goes on the CH page.
VL53L8_LIVE_SYMBOLS = {
    "Vl53l8Model", "Vl53l8Motion", "Vl53l8LiveFrame", "Vl53l8Progress",
    "Vl53l8", "open_vl53l8",
}
VL53L8CH_LIVE_SYMBOLS = {"CnhSetup"}
BNO055_LIVE_SYMBOLS = {"Bno055Sample", "Bno055Status", "Bno055", "open_bno055"}
# The VL53L 1D family class: on each of the five 1D pages.
VL53LX_LIVE_SYMBOLS = {
    "Vl53lxCap", "Vl53lxTarget", "Vl53lxBins", "Vl53lxMeasurement", "Vl53lx", "open_vl53lx",
}
# The BNO085 / BNO086 class and the `depz::bno086::` value types beside it.
BNO086_LIVE_SYMBOLS = {
    "SensorId", "TareBasis", "TARE_X", "Feature", "FeatureRequest", "ProductId",
    "CalibrationConfig", "ErrorRecord", "Counts", "CommandResponse", "SensorMetadata",
    "q_point", "Bno086Report", "Bno086", "open_bno086",
}


def live_note(cls: str, extra: str = "") -> list[str]:
    """The note on a sensor page that lists its live class."""
    return [f"The live class (`depz::{cls}`) derives from `depz::Device`; the",
            "device core it inherits (common commands, time sync, events),",
            f"the links, streams, errors and discovery{extra} are in the",
            "[top-level API reference](../api.md#live-hardware-links-device-discovery).",
            ""]

# The vl53l8 domain is split by symbol name into the CX base and the CH
# superset for the per-sensor references: the CH footer-id offset and the CNH
# (compact-network-histogram) decode are CH-specific; the shared decode surface
# lives under CX.
VL53L8CNH_SYMBOLS = {
    "CNH_DATA_IDX", "CNH_MAX_AGGREGATES", "CNH_MAX_FEATURE_LENGTH", "CNH_PER_HEADER_WORDS", "CNH_PER_BUFFER_HEADER_WORDS",
    "CNH_PER_HEADER_BUFFER_INFO_IDX", "CNH_PER_HEADER_FLAGS_IDX",
    "CNH_BUFFER_INFO_WORDS_MASK", "CNH_MI_STATE_PING",
    "CnhAggregate", "CnhFrame", "decode_cnh",
}
VL53L8CH_SYMBOLS = {"FOOTER_ID_OFF_CH"} | VL53L8CNH_SYMBOLS

# Boards that share one firmware (and so one header).
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


# --------------------------------------------------------------------------
# Symbol model


class Symbol:
    def __init__(self, name: str, domain: str):
        self.name = name
        self.domain = domain
        self.md: list[str] = []  # the rendered ``### ...`` block

    # Overloaded free functions (e.g. two ``to_string``) merge extra
    # signature fences under one heading instead of colliding anchors.
    def add_overload(self, sig: str, doc: str) -> None:
        self.md += ["```cpp", sig, "```", ""]
        if doc:
            self.md += [doc, ""]


def anchor(text: str) -> str:
    """GitHub heading-anchor (github-slugger): lowercase, drop punctuation,
    then replace EACH whitespace char with '-' (no run-collapsing — so
    ``a & b`` yields ``a--b``, exactly like GitHub)."""
    a = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"\s", "-", a)


# --------------------------------------------------------------------------
# Low-level source scanning


def strip_line_comment(line: str) -> tuple[str, str]:
    """Split ``code  // comment`` -> (code, comment_text). No string literals
    in these headers contain ``//``, so a plain find is safe."""
    idx = line.find("//")
    if idx < 0:
        return line, ""
    return line[:idx], line[idx + 2:].strip()


def comment_body(stripped_line: str) -> str:
    """Text of a ``// ...`` comment line, minus the marker + one space."""
    return re.sub(r"^//\s?", "", stripped_line).rstrip()


def read_statement(text: str) -> tuple[str, str, int]:
    """Read one declaration from ``text``: returns (code, trailing_comment,
    consumed) where ``consumed`` also swallows a same-line trailing
    ``// comment`` so it attaches to this statement, not the next."""
    end = scan_decl_end(text)
    stmt = text[:end]
    # swallow a trailing same-line comment
    rest = text[end:]
    m = re.match(r"[ \t]*//([^\n]*)", rest)
    trailing = ""
    if m:
        trailing = m.group(1).strip()
        end += m.end()
    # drop `//` comments line by line (an initializer may carry one per row)
    code = "\n".join(strip_line_comment(ln)[0] for ln in stmt.split("\n"))
    return code, trailing, end


def clean_doc(comment_lines: list[str]) -> str:
    """Join accumulated ``//`` comment lines into a doc paragraph, dropping
    pure divider lines (``// ─────`` / ``// ----``)."""
    out: list[str] = []
    for c in comment_lines:
        if c and set(c) <= {"-", "─", " "}:
            continue
        out.append(c)
    # trim leading/trailing blanks
    while out and not out[0].strip():
        out.pop(0)
    while out and not out[-1].strip():
        out.pop()
    return "\n".join(out).strip()


def scan_decl_end(text: str) -> int:
    """Index just past the first complete declaration in ``text`` — a
    statement ending in ``;`` at top level, or a function definition whose
    body brace closes. Initializer braces (``= {...}``) are not bodies."""
    paren = brace = 0
    saw_params = body = False
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == "(":
            paren += 1
        elif c == ")":
            paren -= 1
            if paren == 0:
                saw_params = True
        elif c == "{":
            if paren == 0 and saw_params and not body:
                body = True
            brace += 1
        elif c == "}":
            brace -= 1
            if body and brace == 0:
                return i + 1
        elif c == ";" and paren == 0 and brace == 0:
            return i + 1
        i += 1
    return n


def normalize_sig(text: str) -> str:
    """Collapse a (possibly multi-line) declaration to one line, strip an
    inline function body, and ensure a trailing ``;``."""
    # drop a trailing definition body { ... }
    depth = 0
    cut = None
    for i, c in enumerate(text):
        if c == "{":
            if depth == 0:
                # keep an initializer brace only if it is part of the params
                # region; a top-level body brace starts here
                cut = i
            depth += 1
        elif c == "}":
            depth -= 1
    # `= {...}` is an initializer, not a body: keep it
    if cut is not None and text[cut:].strip().startswith("{") \
            and not re.search(r"=\s*$", text[:cut]):
        text = text[:cut]
    s = re.sub(r"\s+", " ", text).strip().rstrip(";").strip()
    # strip a constructor member-initializer list: `) : base(x), m(y)`
    s = re.sub(r"\)\s*:\s.*$", ")", s)
    return s + ";"


def func_name(sig: str) -> str | None:
    """Name of a free/member function: the identifier before its first
    ``(`` (templates use ``<>``, so the first paren is the arg list)."""
    m = re.search(r"([A-Za-z_]\w*)\s*\(", sig)
    return m.group(1) if m else None


# --------------------------------------------------------------------------
# Struct / class body parsing


def parse_struct_body(body: str) -> tuple[list[str], list[tuple[str, str, str]]]:
    """Return (field_source_lines, methods) for the public interface of a
    struct/class body. ``methods`` is ``[(name, signature, doc), ...]``.
    Fields are kept as trimmed source lines (with their inline comments)."""
    access = "public"  # struct default; callers flip for ``class``
    fields: list[str] = []
    methods: list[tuple[str, str, str]] = []
    pending: list[str] = []
    i = 0
    n = len(body)
    while i < n:
        # skip whitespace/newlines (a blank line breaks a doc block)
        if body[i] in " \t\r\n":
            if body[i] == "\n" and i + 1 < n and body[i + 1] == "\n":
                pending = []
            i += 1
            continue
        # comment line?
        if body.startswith("//", i):
            eol = body.find("\n", i)
            if eol < 0:
                eol = n
            pending.append(comment_body(body[i:eol].strip()))
            i = eol
            continue
        # access label?
        m = re.match(r"(public|private|protected)\s*:", body[i:])
        if m:
            access = m.group(1)
            pending = []
            i += m.end()
            continue
        # a declaration statement (swallowing its trailing same-line comment)
        code, trailing, end = read_statement(body[i:])
        i += end
        code_clean = re.sub(r"\s+", " ", code).strip()
        if not code_clean:
            pending = []
            continue
        doc = clean_doc(pending) if pending else trailing.strip()
        pending = []
        if access != "public":
            continue
        # a nested enum is part of the field layout: show it on one line
        if re.match(r"enum\b", code_clean):
            fields.append(code_clean.rstrip(";").rstrip() + ";")
            continue
        # nested struct/class inside public? treat as opaque — skip
        if re.match(r"(struct|class)\b", code_clean):
            continue
        # inherited constructors (`using Base::Base;`), deleted / defaulted
        # special members, destructors, operators and move constructors
        # (handles are move-only by design) are not API to document
        if code_clean.startswith(("using ", "friend ")) \
                or re.search(r"(^|\s)~\w+\s*\(", code_clean) \
                or re.search(r"=\s*(delete|default)\s*;?$", code_clean) \
                or re.search(r"\boperator\b", code_clean) \
                or re.match(r"(explicit\s+)?\w+\s*\(\s*\w+\s*&&", code_clean):
            continue
        if "(" in code_clean:  # member function
            sig = normalize_sig(code)
            name = func_name(sig)
            if name:
                methods.append((name, sig, doc))
        else:  # data field — keep the source line + any inline comment
            src = code_clean.rstrip(";") + ";"
            if trailing:
                src += "  // " + trailing
            fields.append(src)
    return fields, methods


# --------------------------------------------------------------------------
# Rendering


def render_enum(name: str, head: str, body_lines: list[str], doc: str) -> list[str]:
    fence = [head.rstrip("{").rstrip() + " {"]
    for bl in body_lines:
        fence.append("    " + bl.strip())
    fence.append("};")
    out = [f"### {name}", "", "```cpp", *fence, "```", ""]
    if doc:
        out += [doc, ""]
    return out


def render_struct(name: str, head: str, fields: list[str],
                  methods: list[tuple[str, str, str]], doc: str) -> list[str]:
    fence = [head.rstrip("{").rstrip() + " {"]
    if fields:
        for f in fields:
            fence.append("    " + f)
    else:
        fence.append("    // (no public data members)")
    fence.append("};")
    out = [f"### {name}", "", "```cpp", *fence, "```", ""]
    if doc:
        out += [doc, ""]
    for mname, sig, mdoc in methods:
        out += [f"#### {name}.{mname}", "", "```cpp", sig, "```", ""]
        if mdoc:
            out += [mdoc, ""]
    return out


def render_callable(name: str, sig: str, doc: str) -> list[str]:
    out = [f"### {name}", "", "```cpp", sig, "```", ""]
    if doc:
        out += [doc, ""]
    return out


def render_value(name: str, sig: str, doc: str) -> list[str]:
    out = [f"### {name}", "", "```cpp", sig, "```", ""]
    if doc:
        out += [doc, ""]
    return out


# --------------------------------------------------------------------------
# Header parsing


def parse_header(path: Path, domain: str) -> list[Symbol]:
    """Namespace-scope symbols of one header, in source order."""
    text = path.read_text()
    lines = text.split("\n")
    symbols: list[Symbol] = []
    by_name: dict[str, Symbol] = {}
    pending: list[str] = []
    template_prefix = ""

    def emit(name: str, md: list[str], mergeable_sig: str | None = None) -> None:
        if mergeable_sig is not None and name in by_name:
            # overloaded free function: append another signature
            by_name[name].add_overload(mergeable_sig, clean_doc(pending))
            return
        s = Symbol(name, domain)
        s.md = md
        symbols.append(s)
        by_name[name] = s

    i = 0
    n = len(lines)
    while i < n:
        raw = lines[i]
        stripped = raw.strip()
        if stripped == "":
            pending = []
            i += 1
            continue
        if stripped.startswith("//"):
            pending.append(comment_body(stripped))
            i += 1
            continue
        if re.match(r'extern\s+"C"\s*\{', stripped) \
                or re.match(r"namespace\s+detail\s*\{", stripped):
            # C forward declarations / private helpers: skip the whole block
            buf = "\n".join(lines[i:])
            k = buf.find("{")
            depth, j = 0, k
            while j < len(buf):
                if buf[j] == "{":
                    depth += 1
                elif buf[j] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            pending = []
            i += buf[:j].count("\n") + 1
            continue
        if stripped.startswith("#") or stripped.startswith("namespace") \
                or stripped.startswith("}"):
            pending = []
            i += 1
            continue
        if stripped.startswith("template"):
            # a class template keeps its doc and shows the template head;
            # anything else (span.hpp is not parsed) resets
            nxt = lines[i + 1].strip() if i + 1 < n else ""
            if re.match(r"(struct|class)\s+\w+", nxt):
                template_prefix = stripped + " "
            else:
                pending = []
            i += 1
            continue

        doc = clean_doc(pending)
        pending = []

        # enum
        m = re.match(r"enum\b", stripped)
        if m:
            head_end = raw.find("{")
            head = raw[:head_end] if head_end >= 0 else raw
            name = re.search(r"enum(?:\s+class)?\s+(\w+)", stripped).group(1)
            body_lines: list[str] = []
            # collect the enum body (up to the matching closing brace)
            buf = "\n".join(lines[i:])
            k = buf.find("{")
            end = scan_decl_end(buf[k:]) + k if k >= 0 else len(buf)
            enum_text = buf[k + 1:end].rsplit("}", 1)[0]
            for bl in enum_text.split("\n"):
                t = bl.strip()
                if not t:
                    continue
                if t.startswith("//") and set(comment_body(t)) <= {"-", "─", " "}:
                    continue  # drop pure divider comments
                body_lines.append(t)
            emit(name, render_enum(name, head, body_lines, doc))
            # advance i past the enum
            consumed = buf[:end].count("\n")
            i += consumed + 1
            continue

        # struct / class
        m = re.match(r"(struct|class)\s+(\w+)", stripped)
        if m and ("{" in stripped or "{" in "".join(lines[i:i + 3])):
            kind, name = m.group(1), m.group(2)
            buf = "\n".join(lines[i:])
            k = buf.find("{")
            head = buf[:k].strip()
            end = scan_decl_end(buf[k:]) + k
            body = buf[k + 1:end]
            # trim the closing };
            body = body.rsplit("}", 1)[0]
            fields, methods = parse_struct_body(body)
            if kind == "class":
                # class default is private: re-parse with private start by
                # prepending a private label unless the body opens with one.
                if not re.match(r"\s*(public|private|protected)\s*:", body):
                    fields, methods = parse_struct_body("private:\n" + body)
            if not doc:
                # a one-line class with a trailing `// comment` after its `};`
                m3 = re.match(r"[ \t]*;?[ \t]*//([^\n]*)", buf[end:])
                if m3:
                    doc = m3.group(1).strip()
            head = template_prefix + head
            template_prefix = ""
            emit(name, render_struct(name, head, fields, methods, doc))
            i += buf[:end].count("\n") + 1
            continue

        # using alias
        if stripped.startswith("using"):
            buf = "\n".join(lines[i:])
            end = scan_decl_end(buf)
            stmt = buf[:end]
            # `using X = ...;` names X; `using ns::X;` re-exports X
            name = re.match(r"using\s+([\w:]+)", stripped).group(1).split("::")[-1]
            sig = normalize_sig(strip_line_comment(stmt)[0])
            emit(name, render_value(name, sig, doc))
            i += stmt.count("\n") + 1
            continue

        # function / constant / variable
        buf = "\n".join(lines[i:])
        code, trailing, end = read_statement(buf)
        stmt = buf[:end]
        code_clean = re.sub(r"\s+", " ", code).strip()
        if not doc and trailing:
            doc = trailing.strip()
        if "(" in code_clean and not code_clean.startswith("using"):
            sig = normalize_sig(code)
            name = func_name(sig)
            if name:
                emit(name, render_callable(name, sig, doc), mergeable_sig=sig)
        else:
            # constant / variable
            m2 = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*=", code_clean)
            if not m2:
                m2 = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;", code_clean)
            if m2:
                name = m2.group(1)
                sig = normalize_sig(code)
                emit(name, render_value(name, sig, doc))
        i += stmt.count("\n") + 1
        continue

    return symbols


# --------------------------------------------------------------------------
# Assembly


def collect() -> list[tuple[str, str, list[Symbol]]]:
    """``[(domain, title, [Symbol, ...]), ...]`` in DOMAIN_ORDER."""
    by_domain: dict[str, list[Symbol]] = {}
    for fname, domain in FILE_DOMAIN:
        path = INCLUDE / fname
        for s in parse_header(path, domain):
            by_domain.setdefault(domain, []).append(s)
    groups = []
    for d in DOMAIN_ORDER:
        if by_domain.get(d):
            groups.append((d, DOMAIN_TITLE[d], by_domain[d]))
    return groups


def render_reference(groups: list[tuple[str, str, list[Symbol]]],
                     header_lines: list[str]) -> str:
    head = list(header_lines) + ["## Contents", ""]
    for _d, title, members in groups:
        links = ", ".join(f"[`{s.name}`](#{anchor(s.name)})" for s in members)
        head.append(f"- **{title}**: {links}")
    head.append("")

    body: list[str] = []
    for _d, title, members in groups:
        body += [f"## {title}", ""]
        for s in members:
            body += s.md
    return "\n".join(head + body).rstrip() + "\n"


def sensor_targets(groups):
    """``[(folder, title, [Symbol], note_lines), ...]`` — one per sensor
    api.md. The vl53l8 domain is split by symbol name into CX and CH."""
    by = {d: (title, members) for d, title, members in groups}
    targets = []
    if "sr04" in by:
        _t, m = by["sr04"]
        live = [s for s in by.get("device", ("", []))[1] if s.name in SR04_LIVE_SYMBOLS]
        if live:
            targets.append((
                "sr04", "SR04",
                [("sr04", "SR04 codecs (depz/sr04.hpp)", m),
                 ("sr04_live", "SR04 live class (depz/device.hpp)", live)],
                live_note("Sr04"),
            ))
        else:
            targets.append(("sr04", "SR04", m, []))
    if "vl53l4" in by:
        _t, m = by["vl53l4"]
        live = [s for s in by.get("device", ("", []))[1]
                if s.name in VL53L4CD_LIVE_SYMBOLS]
        if live:
            targets.append((
                "vl53l4cd", "VL53L4CD (ToF, single-zone)",
                [("vl53l4", "VL53L4CD codecs and ULD math (depz/vl53l4.hpp)", m),
                 ("vl53l4cd_live", "VL53L4CD live class (depz/device.hpp)", live)],
                live_note("Vl53l4cd"),
            ))
        else:
            targets.append(("vl53l4cd", "VL53L4CD (ToF, single-zone)", m, []))
    if "vl53l8" in by:
        _t, m = by["vl53l8"]
        cx = [s for s in m if s.name not in VL53L8CH_SYMBOLS]
        ch = [s for s in m if s.name in VL53L8CH_SYMBOLS]
        device = by.get("device", ("", []))[1]
        live = [s for s in device if s.name in VL53L8_LIVE_SYMBOLS]
        ch_live = [s for s in device if s.name in VL53L8CH_LIVE_SYMBOLS]
        if live:
            targets.append((
                "vl53l8cx", "VL53L8CX (ToF)",
                [("vl53l8", "VL53L8 codecs (depz/vl53l8.hpp)", cx),
                 ("vl53l8_live", "VL53L8 live class (depz/device.hpp)", live)],
                live_note("Vl53l8") + [
                    "`depz::Vl53l8` drives the VL53L8CX and the VL53L8CH alike, and",
                    "the VL53L5CX / VL53L7CX / VL53L7CH on the I2C board",
                    "(`module_type`, `bridge_info`, `set_i2c_speed_khz` and",
                    "`pin_ctrl` are for that board only); `CnhSetup`, the CH-only",
                    "CNH configuration, is in the",
                    "[VL53L8CH API reference](../vl53l8ch/api.md).",
                    ""],
            ))
        else:
            targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx, []))
        ch_note = [
            "The VL53L8CH is the VL53L8CX superset: it streams the *same*",
            "results-frame layout, so the entire decode surface — frame",
            "reassembly, `decode_frame`, and the advanced-feature DCI codecs —",
            "is shared with the CX and documented in the",
            "[VL53L8CX API reference](../vl53l8cx/api.md). The CH-specific",
            "symbols are the CH footer-id offset and the CH's own addition,",
            "the Compact-Network-Histogram (CNH) decode, below.",
        ]
        if ch_live:
            ch_note += [
                "The live class `depz::Vl53l8` is shared too (see the CX",
                "reference); the CNH setup it arms with `configure_cnh()` is",
                "listed below.",
            ]
            targets.append((
                "vl53l8ch", "VL53L8CH (ToF + CNH)",
                [("vl53l8ch", "VL53L8CH codecs (depz/vl53l8.hpp)", ch),
                 ("vl53l8ch_live", "VL53L8CH live: CNH setup (depz/device.hpp)", ch_live)],
                ch_note + [""],
            ))
        else:
            targets.append(("vl53l8ch", "VL53L8CH (ToF + CNH)", ch, ch_note + [""]))
    if "vl53l7" in by:
        _t, m = by["vl53l7"]
        cnh = [s for s in by.get("vl53l8", ("", []))[1] if s.name in VL53L8CNH_SYMBOLS]
        shared = [
            "One board firmware (`APP_VL53L7`) serves the VL53L5CX, VL53L7CX and",
            "VL53L7CH, so the three share `depz/vl53l7.hpp`. Chunk parse",
            "(`vl53l8::FrameChunk`), reassembly (`vl53l8::FrameReassembler`) and",
            "the `vl53l8::Vl53l8Frame` result struct are the VL53L8 ones — see",
            "the [VL53L8CX API reference](../vl53l8cx/api.md).",
        ]
        device = by.get("device", ("", []))[1]
        has_live = any(s.name == "Vl53l8" for s in device)
        ch_live = [s for s in device if s.name in VL53L8CH_LIVE_SYMBOLS]
        for folder, title in VL53L7_BOARDS:
            groups = [("vl53l7", DOMAIN_TITLE["vl53l7"], m)]
            note = list(shared)
            if has_live:
                note += ["The live class is the VL53L8 one, `depz::Vl53l8` (models",
                         "`L5CX` / `L7CX` / `L7CH`), listed in the VL53L8CX reference",
                         "with its I2C-board methods (`module_type`, `bridge_info`,",
                         "`set_i2c_speed_khz`, `pin_ctrl`)."]
            if folder == "vl53l7ch" and cnh:
                groups.append(("cnh", "CNH decode (shared with VL53L8CH)", cnh))
                note += ["The CNH histogram decode is the VL53L8CH one, listed",
                         "below as well."]
            if folder == "vl53l7ch" and ch_live:
                groups.append(("vl53l7ch_live", "VL53L7CH live: CNH setup (depz/device.hpp)",
                               ch_live))
                note += ["So is `CnhSetup`, the CNH configuration the live class",
                         "arms with `configure_cnh()`."]
            targets.append((folder, title, groups, note + [""]))
    if "vl53lx" in by:
        _t, m = by["vl53lx"]
        note = [
            "One bridge firmware (`APP_VL53L0_4`) serves the whole VL53L 1D",
            "family, so VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX and VL53L4CX share",
            "`depz/vl53lx.hpp`. The re-exported READ_REG / WRITE_REG / XSHUT /",
            "SET_I2C_SPEED encoders, `RegData`, `StreamData` and the",
            "`Vl53l4Result` behind `DieResult` are the VL53L4CD codecs — see the",
            "[VL53L4CD API reference](../vl53l4cd/api.md).",
            "",
        ]
        live = [s for s in by.get("device", ("", []))[1] if s.name in VL53LX_LIVE_SYMBOLS]
        groups = [("vl53lx", DOMAIN_TITLE["vl53lx"], m)]
        if live:
            groups.append(("vl53lx_live", "VL53L 1D family live class (depz/device.hpp)", live))
            note = live_note("Vl53lx") + note
        for folder, title in VL53LX_BOARDS:
            targets.append((folder, title, groups, note))
    if "bno086" in by:
        _t, m = by["bno086"]
        live = [s for s in by.get("device", ("", []))[1] if s.name in BNO086_LIVE_SYMBOLS]
        if live:
            targets.append((
                "bno086", "BNO086 (IMU)",
                [("bno086", "BNO086 codecs (depz/bno086.hpp)", m),
                 ("bno086_live", "BNO085 / BNO086 live class (depz/device.hpp)", live)],
                live_note("Bno086") + [
                    "The value types next to it (`SensorId`, `Feature`, `ProductId`,",
                    "...) live in `namespace depz::bno086`; one class serves the",
                    "BNO085 and the BNO086.", ""],
            ))
        else:
            targets.append(("bno086", "BNO086 (IMU)", m, []))
    if "bno055" in by:
        _t, m = by["bno055"]
        live = [s for s in by.get("device", ("", []))[1] if s.name in BNO055_LIVE_SYMBOLS]
        if live:
            targets.append((
                "bno055", "BNO055 (IMU)",
                [("bno055", "BNO055 codecs (depz/bno055.hpp)", m),
                 ("bno055_live", "BNO055 live class (depz/device.hpp)", live)],
                live_note("Bno055"),
            ))
        else:
            targets.append(("bno055", "BNO055 (IMU)", m, []))
    return targets


def main() -> None:
    groups = collect()
    root_head = [
        "# API reference",
        "",
        "Auto-generated from the public headers under `include/depz/` (their",
        "declarations and `//` doc-comments) by `scripts/gen_api_md.py` — run",
        "`cmake --build build --target docs` (or `python3 scripts/gen_api_md.py`)",
        "to regenerate. Edit the doc-comments in the headers, not this file.",
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
        "Two layers: the *decode layer* (every header but `depz/device.hpp`)",
        "takes bytes and returns typed values, no I/O; the *live-hardware",
        "layer* (`depz/device.hpp`, a wrapper over the C SDK) opens a board,",
        "runs its commands and delivers its data — see the last section.",
        "",
    ]
    n_syms = sum(len(m) for _, _, m in groups)
    (DOCS).mkdir(parents=True, exist_ok=True)
    (DOCS / "api.md").write_text(render_reference(groups, root_head))
    print(f"wrote {DOCS / 'api.md'} ({n_syms} symbols across {len(groups)} groups)")

    for folder, title, members, note_lines in sensor_targets(groups):
        # `members` is a Symbol list (one group titled like the sensor) or a
        # ready list of (domain, title, symbols) groups.
        if members and isinstance(members[0], tuple):
            groups_out = members
        else:
            groups_out = [(folder, title, members)]
        n_members = sum(len(g[2]) for g in groups_out)
        sub_head = [
            f"# {title} — API reference",
            "",
            (f"The public API for the {title} sensor: its codecs and its live class."
             if any(g[0].endswith("_live") for g in groups_out)
             else f"The public decode API for the {title} sensor.") + " Transport,",
            "common-protocol, identity and other cross-sensor symbols shared by",
            "every sensor live in the [top-level API reference](../api.md).",
            "",
            *note_lines,
            "Auto-generated by `scripts/gen_api_md.py` — edit the doc-comments in",
            "the headers, not this file.",
            "",
        ]
        out = DOCS / folder / "api.md"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(render_reference(groups_out, sub_head))
        print(f"wrote {out} ({n_members} symbols)")


if __name__ == "__main__":
    main()
