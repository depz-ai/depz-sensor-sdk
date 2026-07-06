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
- ``docs/{sr04,vl53l8cx,vl53l8ch,bno086}/api.md`` — one focused reference
  per sensor, so each sensor has a full intro/guide/api set. The shared
  surface (transport, common protocol, identity, …) stays only in the root
  reference. The single ``vl53l8`` domain is TWO sensors — the CX base and
  the CH superset — so it is split by symbol name: CH-only symbols land in
  ``vl53l8ch``, everything else in ``vl53l8cx``.

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
    ("vl53l8.hpp", "vl53l8"),
    ("bno086.hpp", "bno086"),
    ("dataset.hpp", "dataset"),
]

DOMAIN_ORDER = [
    "transport", "common", "identity", "usb_ids", "sr04",
    "vl53l8", "bno086", "bootloader", "dataset",
]
DOMAIN_TITLE = {
    "transport": "Transport (framing & CRC)",
    "common": "Common protocol",
    "identity": "Identity",
    "usb_ids": "USB identity",
    "sr04": "SR04",
    "vl53l8": "VL53L8 (ToF)",
    "bno086": "BNO086 (IMU)",
    "bootloader": "Bootloader / firmware",
    "dataset": "Datasets",
}

# The vl53l8 domain is split by symbol name into the CX base and the CH
# superset for the per-sensor references. CNH histogram decode is a documented
# extension point (not yet implemented), so the only CH-specific *symbol* today
# is the CH footer-id offset; the shared decode surface lives under CX.
VL53L8CH_SYMBOLS = {"FOOTER_ID_OFF_CH"}


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
    code = strip_line_comment(stmt)[0]
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
    if cut is not None and text[cut:].strip().startswith("{"):
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
        # nested type (struct/class/enum) inside public? treat as opaque — skip
        if re.match(r"(struct|class|enum)\b", code_clean):
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
        if stripped.startswith("#") or stripped.startswith("namespace") \
                or stripped.startswith("}"):
            pending = []
            i += 1
            continue
        if stripped.startswith("template"):
            # only span.hpp uses templates (skipped); be safe and reset
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
            emit(name, render_struct(name, head, fields, methods, doc))
            i += buf[:end].count("\n") + 1
            continue

        # using alias
        if stripped.startswith("using"):
            buf = "\n".join(lines[i:])
            end = scan_decl_end(buf)
            stmt = buf[:end]
            name = re.match(r"using\s+(\w+)", stripped).group(1)
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
        targets.append(("sr04", "SR04", m, []))
    if "vl53l8" in by:
        _t, m = by["vl53l8"]
        cx = [s for s in m if s.name not in VL53L8CH_SYMBOLS]
        ch = [s for s in m if s.name in VL53L8CH_SYMBOLS]
        targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx, []))
        targets.append((
            "vl53l8ch", "VL53L8CH (ToF + CNH)", ch,
            ["`Vl53l8Ch` is the VL53L8CX superset: it streams the *same*",
             "results-frame layout, so the entire decode surface — frame",
             "reassembly, `decode_frame`, and the advanced-feature DCI codecs —",
             "is shared with the CX and documented in the",
             "[VL53L8CX API reference](../vl53l8cx/api.md). The only CH-specific",
             "symbol today is the CH footer-id offset below; the CH's own",
             "addition, the Compact-Network-Histogram (CNH) decode, is a",
             "documented extension point that is not yet implemented (see",
             "`include/depz/vl53l8.hpp`).",
             ""],
        ))
    if "bno086" in by:
        _t, m = by["bno086"]
        targets.append(("bno086", "BNO086 (IMU)", m, []))
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
        "[SR04](sr04/api.md) · [VL53L8CX](vl53l8cx/api.md) · "
        "[VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).",
        "",
        "This SDK is the *decode layer* — pure codecs, no I/O. Everything here",
        "takes bytes and returns typed values; opening ports and streaming is",
        "left to the host application.",
        "",
    ]
    n_syms = sum(len(m) for _, _, m in groups)
    (DOCS).mkdir(parents=True, exist_ok=True)
    (DOCS / "api.md").write_text(render_reference(groups, root_head))
    print(f"wrote {DOCS / 'api.md'} ({n_syms} symbols across {len(groups)} groups)")

    for folder, title, members, note_lines in sensor_targets(groups):
        sub_head = [
            f"# {title} — API reference",
            "",
            f"The public decode API for the {title} sensor. Transport,",
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
        out.write_text(render_reference([(folder, title, members)], sub_head))
        print(f"wrote {out} ({len(members)} symbols)")


if __name__ == "__main__":
    main()
