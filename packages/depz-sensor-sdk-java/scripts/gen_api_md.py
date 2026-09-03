#!/usr/bin/env python3
"""Generate the Java SDK API reference (Markdown) from the public source.

A pure-stdlib static parser over the Java sources under
``src/main/java/ai/depz/sensor/``. It extracts every public type
(class / interface / enum / record), their public members (methods,
constants / fields, enum constants) and the attached Javadoc, then emits
Markdown in the same shape as the Python SDK's ``docs/api.md``:

- a title and a short "auto-generated" banner,
- a ``## Contents`` index of the public symbols per domain,
- ``## Domain`` sections, and
- ``### Name`` entries, each with a fenced ```` ```java ```` signature and
  its doc; methods / constants / enum constants render as ``####`` members.

There is no build tool or reflection: the parser reads the source text so the
reference never drifts from the code. Per-sensor references are written too —
one focused ``docs/<sensor>/api.md`` per sensor — with the shared transport /
protocol surface staying only in the root file. The ToF domain is one Java
package but *two* sensors: the shared VL53L8CX base decode plus the CH-only
additions (CNH), so it is split by symbol exactly like the Python generator.

Truthful about scope: this SDK is decode-layer only. The live ULD init /
register-bridge driver and the VL53L8CH CNH histogram decode are extension
points and are surfaced here as documented stubs, not working code.

Regenerate:

    python3 scripts/gen_api_md.py

Edit the Javadoc in the source, not the generated Markdown.
"""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "main" / "java" / "ai" / "depz" / "sensor"
DOCS = ROOT / "docs"

# Domain (grouping) → section title, in reading order. A type's domain is its
# package's leaf, with a couple of per-class overrides so the sensor-specific
# codecs that happen to live in the shared ``protocol`` package land in their
# own sensor domain (mirrors how the Python generator groups by defining
# submodule, not by re-export).
DOMAIN_ORDER = [
    "transport", "usb", "protocol", "sr04", "fwdepz",
    "vl53l4", "vl53l8", "bno086", "dataset",
]
DOMAIN_TITLE = {
    "transport": "Transport",
    "usb": "USB identity",
    "protocol": "Protocol codecs (common)",
    "sr04": "SR04",
    "fwdepz": "Firmware container",
    "vl53l4": "VL53L4CD (ToF)",
    "vl53l8": "VL53L8 (ToF)",
    "bno086": "BNO086 (IMU)",
    "dataset": "Datasets (record & replay)",
}
# Per-class domain overrides (fully-qualified simple name of the top-level type).
DOMAIN_OF_TYPE = {
    "Sr04": "sr04",
    "Vl53l4": "vl53l4",
    "FwDepz": "fwdepz",
}

# The single ``vl53l8`` package is TWO sensors. Everything shared is the CX base;
# these symbols (types or ``Type.method`` members) are the CH-only additions and
# render on the VL53L8CH page instead of / in addition to the CX one.
VL53L8CH_PICKS = ["Vl53l8Uld.Variant", "Vl53l8Uld.cnhHistogramDecodeStubbed"]


# ── Javadoc cleanup ────────────────────────────────────────────────────────

def _clean_doc(raw: str) -> str:
    """Turn a raw Javadoc comment body into readable Markdown-ish text."""
    lines = []
    for ln in raw.split("\n"):
        ln = ln.strip()
        if ln.startswith("/**"):
            ln = ln[3:]
        if ln.endswith("*/"):
            ln = ln[:-2]
        ln = re.sub(r"^\* *", "", ln.strip())
        lines.append(ln)
    text = "\n".join(lines).strip()

    # Inline tags → backticked code; drop the leading '#' of member refs.
    # Allow one level of nested braces (e.g. {@code {"d","t"}}).
    text = re.sub(r"\{@(?:code|link|literal)\s+#?((?:[^{}]|\{[^{}]*\})*)\}",
                  lambda m: "`%s`" % m.group(1).strip(), text)
    # Simple HTML → text / Markdown.
    text = text.replace("<p>", "\n\n").replace("</p>", "")
    text = re.sub(r"</?(?:b|i|em|strong|code|tt)>", "", text)
    text = re.sub(r"<ul>|</ul>", "", text)
    text = re.sub(r"\s*<li>\s*", "\n- ", text)
    text = text.replace("</li>", "")
    text = text.replace("&lt;", "<").replace("&gt;", ">").replace("&amp;", "&")
    # Block tags: keep @param/@return as bullets, drop the rest.
    out = []
    for ln in text.split("\n"):
        s = ln.strip()
        m = re.match(r"@param\s+(\S+)\s+(.*)", s)
        if m:
            out.append("- `%s` — %s" % (m.group(1), m.group(2)))
            continue
        m = re.match(r"@return\s+(.*)", s)
        if m:
            out.append("- Returns: %s" % m.group(1))
            continue
        if re.match(r"@(throws|exception|see|since|author|deprecated)\b", s):
            continue
        out.append(ln)
    text = "\n".join(out)
    text = re.sub(r"\n{3,}", "\n\n", text).strip()
    return text


def _norm(sig: str) -> str:
    """Collapse whitespace in a declaration head; tidy paren padding."""
    s = re.sub(r"\s+", " ", sig).strip()
    s = re.sub(r"\(\s+", "(", s)
    s = re.sub(r"\s+\)", ")", s)
    return s


def _is_public(head: str) -> bool:
    return bool(re.match(r"(public|protected)\b", head))


# ── Java source scanner ────────────────────────────────────────────────────

_TYPE_RE = re.compile(
    r"(?:^|[\s])(?:public|protected|private|final|abstract|static|sealed"
    r"|non-sealed|strictfp)\s+.*?\b(class|interface|enum|record)\s+"
    r"([A-Za-z_]\w*)")
_METHOD_NAME_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
_JAVA_KEYWORDS = {
    "if", "for", "while", "switch", "catch", "synchronized", "return",
    "new", "else", "try", "do", "super", "this",
}


def _skip_braces(text: str, i: int) -> int:
    """``text[i-1]`` was ``{``; return the index just past its match, honoring
    string / char literals and comments so initializer braces don't desync."""
    depth = 1
    n = len(text)
    while i < n and depth:
        c = text[i]
        if c == '"' or c == "'":
            i = _skip_string(text, i)
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        i += 1
    return i


def _skip_string(text: str, i: int) -> int:
    """``text[i]`` opens a string / char literal; return index past its close."""
    q = text[i]
    i += 1
    n = len(text)
    while i < n:
        c = text[i]
        if c == "\\":
            i += 2
            continue
        if c == q:
            return i + 1
        i += 1
    return i


def _split_top(s: str, seps=(",",)) -> list[str]:
    """Split ``s`` on the separators at bracket depth 0 (``()``/``<>``/``[]``)."""
    out, buf, depth = [], [], 0
    for c in s:
        if c in "(<[":
            depth += 1
        elif c in ")>]":
            depth -= 1
        if c in seps and depth == 0:
            out.append("".join(buf))
            buf = []
        else:
            buf.append(c)
    if buf:
        out.append("".join(buf))
    return out


def parse_java(text: str):
    """Return the list of top-level type dicts parsed from one source file.

    Each type dict: ``name, kind, sig, doc, members, children``. ``members``
    is a list of ``{kind, name, sig, doc, value?}`` for methods, fields /
    constants and the enum-constant block."""
    i, n = 0, len(text)
    buf: list[str] = []
    pending_doc = None
    tops: list[dict] = []
    # Stack entries: dicts with kind "type"/"block". Type entries carry "type".
    stack: list[dict] = []

    def cur_type():
        return stack[-1]["type"] if stack and stack[-1]["kind"] == "type" else None

    def parent_type():
        for e in reversed(stack):
            if e["kind"] == "type":
                return e["type"]
        return None

    while i < n:
        c = text[i]

        # Comments (capture Javadoc as pending doc).
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            is_doc = i + 2 < n and text[i + 2] == "*"
            j = i + 2
            while j + 1 < n and not (text[j] == "*" and text[j + 1] == "/"):
                j += 1
            block = text[i:j + 2]
            if is_doc:
                pending_doc = _clean_doc(block)
            i = j + 2
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if c == '"' or c == "'":
            j = _skip_string(text, i)
            buf.append(" ")
            i = j
            continue

        if c == "{":
            head = _norm("".join(buf))
            buf = []
            mt = _TYPE_RE.search(" " + head)
            ct = cur_type()
            if mt:
                td = {
                    "name": mt.group(2), "kind": mt.group(1), "sig": head,
                    "doc": pending_doc or "", "members": [], "children": [],
                    "enum_done": False, "public": _is_public(head),
                }
                p = parent_type()
                if p is not None:
                    td["qual"] = p["qual"] + "." + td["name"]
                    p["children"].append(td)
                else:
                    td["qual"] = td["name"]
                    tops.append(td)
                stack.append({"kind": "type", "type": td})
                pending_doc = None
                i += 1
                continue
            # Field initializer with a brace (arrays, map literals): keep the
            # statement, skip the balanced braces, let the ';' record the field.
            if ct is not None and "=" in head:
                i = _skip_braces(text, i + 1)
                buf = list(head + " {…}")
                continue
            if ct is not None and _METHOD_NAME_RE.search(head):
                name_m = _METHOD_NAME_RE.search(head)
                mname = name_m.group(1)
                # Constructors (name == type) and control words are not methods.
                if mname != ct["name"] and mname not in _JAVA_KEYWORDS \
                        and head.startswith(("public", "protected")):
                    ct["members"].append(
                        {"kind": "method", "name": mname, "sig": head,
                         "doc": pending_doc or ""})
                stack.append({"kind": "block"})
                pending_doc = None
                i += 1
                continue
            stack.append({"kind": "block"})
            i += 1
            continue

        if c == "}":
            if stack:
                e = stack.pop()
                if e["kind"] == "type":
                    td = e["type"]
                    # Enum with no trailing ';' — constants sit in the buffer.
                    if td["kind"] == "enum" and not td["enum_done"]:
                        _record_enum_consts(td, _norm("".join(buf)))
            buf = []
            i += 1
            continue

        if c == ";":
            stmt = _norm("".join(buf))
            buf = []
            ct = cur_type()
            if ct is not None and stmt:
                if ct["kind"] == "enum" and not ct["enum_done"]:
                    _record_enum_consts(ct, stmt)
                else:
                    _maybe_field(ct, stmt, pending_doc)
            pending_doc = None
            i += 1
            continue

        buf.append(c)
        i += 1

    return tops


def _record_enum_consts(td: dict, stmt: str):
    td["enum_done"] = True
    consts = []
    for part in _split_top(stmt):
        part = part.strip()
        m = re.match(r"([A-Za-z_]\w*)\s*(\((.*)\))?", part)
        if m and m.group(1):
            consts.append((m.group(1), (m.group(3) or "").strip()))
    if consts:
        td["members"].append({"kind": "enum_consts", "consts": consts})


_SIMPLE_LIT_RE = re.compile(r"[-+]?[0-9][0-9A-Fa-fxXLl_]*$")


def _maybe_field(td: dict, stmt: str, doc):
    if not _is_public(stmt):
        return
    decl, _, init = stmt.partition("=")
    decl = decl.strip()
    m = re.search(r"([A-Za-z_]\w*)\s*$", decl)
    if not m:
        return
    # Keep only simple numeric literals in the rendered signature; drop
    # array / string / call initializers (they don't survive scanning cleanly).
    init = init.strip()
    sig = decl if not (init and _SIMPLE_LIT_RE.match(init)) else "%s = %s" % (decl, init)
    td["members"].append(
        {"kind": "field", "name": m.group(1), "sig": sig, "doc": doc or ""})


# ── domain assignment + flattening ─────────────────────────────────────────

def _domain_for(package: str, top_name: str) -> str:
    if top_name in DOMAIN_OF_TYPE:
        return DOMAIN_OF_TYPE[top_name]
    leaf = package.split(".")[-1]
    if package.endswith("sensors.vl53l8"):
        return "vl53l8"
    if package.endswith("sensors.bno086"):
        return "bno086"
    return leaf


def _flatten(td: dict, out: list):
    """Depth-first flatten a type and its nested types into one symbol list,
    skipping non-public types (and their subtrees)."""
    if not td.get("public", True):
        return
    out.append(td)
    for ch in td["children"]:
        _flatten(ch, out)


def collect():
    """``{domain: [type_dict, ...]}`` for every public type in the tree, and a
    ``{qual: type_dict}`` index."""
    by_domain: dict[str, list] = {}
    index: dict[str, dict] = {}
    for path in sorted(SRC.rglob("*.java")):
        text = path.read_text(encoding="utf-8")
        pkg_m = re.search(r"^package\s+([\w.]+);", text, re.M)
        package = pkg_m.group(1) if pkg_m else ""
        for top in parse_java(text):
            domain = _domain_for(package, top["name"])
            flat: list = []
            _flatten(top, flat)
            for t in flat:
                t["domain"] = domain
                index[t["qual"]] = t
            by_domain.setdefault(domain, []).extend(flat)
    return by_domain, index


# ── Markdown rendering ─────────────────────────────────────────────────────

def _anchor(text: str) -> str:
    a = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"\s+", "-", a)


def _sig_block(sig: str) -> list[str]:
    return ["```java", sig, "```", ""]


def _render_member(qual: str, m: dict) -> list[str]:
    if m["kind"] == "enum_consts":
        lines = ["#### %s — constants" % qual, ""]
        items = []
        for name, val in m["consts"]:
            items.append("`%s`%s" % (name, "(%s)" % val if val else ""))
        lines += [", ".join(items), ""]
        return lines
    tag = " *(field)*" if m["kind"] == "field" else ""
    lines = ["#### %s.%s%s" % (qual, m["name"], tag), ""]
    lines += _sig_block(m["sig"])
    if m["doc"]:
        lines += [m["doc"], ""]
    return lines


def _render_type(td: dict) -> list[str]:
    lines = ["### %s" % td["qual"], ""]
    lines += _sig_block(td["sig"])
    if td["doc"]:
        lines += [td["doc"], ""]
    for m in td["members"]:
        lines += _render_member(td["qual"], m)
    return lines


def _render_method_symbol(td: dict, method: dict) -> list[str]:
    """Render a single method as a top-level ``###`` symbol (CH picks)."""
    qual = "%s.%s" % (td["qual"], method["name"])
    lines = ["### %s" % qual, ""]
    lines += _sig_block(method["sig"])
    if method["doc"]:
        lines += [method["doc"], ""]
    return lines


def _contents(sections: list[tuple[str, list[dict]]]) -> list[str]:
    head = ["## Contents", ""]
    for title, syms in sections:
        links = ", ".join("[`%s`](#%s)" % (s["qual"], _anchor(s["qual"]))
                          for s in syms)
        head.append("- **%s**: %s" % (title, links))
    head.append("")
    return head


def _render_reference(header: list[str], sections: list[tuple[str, list[dict]]]) -> str:
    out = list(header) + _contents(sections)
    for title, syms in sections:
        out += ["## %s" % title, ""]
        for s in syms:
            out += _render_type(s)
    return "\n".join(out).rstrip() + "\n"


def _resolve_picks(index: dict, picks: list[str]):
    """Resolve CH pick strings into rendered symbol entries + index rows."""
    syms = []          # {"qual","render"} for bodies
    rows = []          # type dicts (or shims) for the Contents index
    for p in picks:
        if p in index:                       # a whole type
            td = index[p]
            syms.append({"qual": td["qual"], "render": _render_type(td)})
            rows.append(td)
        else:                                # a Type.method member
            tname, mname = p.rsplit(".", 1)
            td = index.get(tname)
            method = None
            if td:
                method = next((m for m in td["members"]
                               if m.get("kind") == "method" and m["name"] == mname), None)
            if method:
                qual = p
                syms.append({"qual": qual, "render": _render_method_symbol(td, method)})
                rows.append({"qual": qual})
    return syms, rows


def main() -> None:
    by_domain, index = collect()
    ordered = DOMAIN_ORDER + [d for d in by_domain if d not in DOMAIN_ORDER]
    sections = [(DOMAIN_TITLE.get(d, d.title()), by_domain[d])
                for d in ordered if by_domain.get(d)]

    DOCS.mkdir(parents=True, exist_ok=True)

    root_header = [
        "# API reference",
        "",
        "Auto-generated from the public Java source under",
        "`src/main/java/ai/depz/sensor/` by `scripts/gen_api_md.py` — run",
        "`python3 scripts/gen_api_md.py` to regenerate. Edit the Javadoc in the",
        "source, not this file.",
        "",
        "This SDK is decode-layer only: pure, host-verifiable codecs. The live",
        "ULD init / register-bridge driver and the VL53L8CH CNH histogram decode",
        "are extension points, surfaced here as documented stubs.",
        "",
        "Each sensor also has a focused reference with just its own symbols:",
        "[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · "
        "[VL53L8CX](vl53l8cx/api.md) · "
        "[VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).",
        "",
    ]
    (DOCS / "api.md").write_text(_render_reference(root_header, sections))
    n_syms = sum(len(s) for _, s in sections)
    print("wrote %s (%d symbols, %d domains)"
          % (DOCS / "api.md", n_syms, len(sections)))

    # Per-sensor references. The ToF package is split into the CX base and the
    # CH-only additions, mirroring the Python generator's class split.
    _write_sensor("sr04", "SR04", by_domain.get("sr04", []), [])
    _write_sensor("vl53l4cd", "VL53L4CD (ToF)", by_domain.get("vl53l4", []), [])
    _write_sensor("vl53l8cx", "VL53L8CX (ToF)", by_domain.get("vl53l8", []), [])

    ch_syms, ch_rows = _resolve_picks(index, VL53L8CH_PICKS)
    ch_note = [
        "`VL53L8CH` is a decode-superset of the VL53L8CX: it shares the entire",
        "results-frame decode and advanced-DCI surface, so only the CH-specific",
        "additions are listed here. For the frame decoder, the reassembler and",
        "every advanced-DCI codec, see the",
        "[VL53L8CX API reference](../vl53l8cx/api.md).",
        "",
        "CNH (compact-histogram) decode is a CH-only **extension point** and is",
        "not implemented in this decode-layer SDK; the shared decoder surfaces",
        "the raw CNH block bytes on `Vl53l8Uld.Results.cnhRaw` but does not",
        "interpret them.",
        "",
    ]
    _write_sensor_custom("vl53l8ch", "VL53L8CH (ToF + CNH)", ch_syms, ch_rows, ch_note)

    _write_sensor("bno086", "BNO086 (IMU)", by_domain.get("bno086", []), [])


def _sensor_header(title: str, note: list[str]) -> list[str]:
    return [
        "# %s — API reference" % title,
        "",
        "The public API for the %s sensor. Transport, USB identity, the common" % title,
        "protocol codecs and other cross-sensor symbols shared by every sensor",
        "live in the [top-level API reference](../api.md).",
        "",
        *note,
        "Auto-generated by `scripts/gen_api_md.py` — edit the Javadoc in the",
        "source, not this file.",
        "",
    ]


def _write_sensor(folder: str, title: str, syms: list[dict], note: list[str]):
    if not syms:
        return
    out = DOCS / folder / "api.md"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(_render_reference(_sensor_header(title, note), [(title, syms)]))
    print("wrote %s (%d symbols)" % (out, len(syms)))


def _write_sensor_custom(folder: str, title: str, syms: list[dict],
                         rows: list[dict], note: list[str]):
    """Per-sensor page whose symbols are hand-picked members (CH split)."""
    out = DOCS / folder / "api.md"
    out.parent.mkdir(parents=True, exist_ok=True)
    header = _sensor_header(title, note)
    links = ", ".join("[`%s`](#%s)" % (r["qual"], _anchor(r["qual"])) for r in rows)
    body = list(header) + ["## Contents", "", "- **%s**: %s" % (title, links), "",
                           "## %s" % title, ""]
    for s in syms:
        body += s["render"]
    out.write_text("\n".join(body).rstrip() + "\n")
    print("wrote %s (%d symbols)" % (out, len(syms)))


if __name__ == "__main__":
    main()
