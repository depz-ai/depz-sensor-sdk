#!/usr/bin/env python3
"""Generate the C# SDK API reference (Markdown) from the public sources.

Parses the public C# under ``src/Depz.Sensor/`` — types (class / record /
enum / static class, including nested types), their public members
(methods, properties, constants, fields, enum members) and the ``///``
XML-doc ``<summary>`` — and writes, in the SAME markdown shape as the
Python SDK's ``docs/api.md`` (title, Contents index, ``## Domain``
sections, ``### Name`` + fenced ```csharp signature``` + doc):

- ``docs/api.md`` — the full reference (every public symbol), and
- ``docs/{sr04,vl53l4cd,vl53l8cx,vl53l8ch,bno086}/api.md`` — one focused
  reference per sensor with just that sensor's own symbols. The shared
  foundation (transport, protocol, firmware, dataset, USB) stays only in
  the root.

The single VL53L8 ToF namespace is TWO sensors — the CX base and the CH
superset — so it is split by symbol: the CH-only ``Vl53l8Cnh`` extension
point lands in ``vl53l8ch``, everything else (shared by CX and CH) in
``vl53l8cx``, mirroring the Python split.

The ``///`` summaries in the source are the single source of truth, so the
reference never drifts from the code. Regenerate with:

    export PATH="$HOME/.dotnet:$PATH"        # not needed by this script,
    python3 scripts/gen_api_md.py            # but keep the toolchain env

Stdlib only — no Roslyn / doc-tool dependency.
"""

from __future__ import annotations

import re
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "src" / "Depz.Sensor"
DOCS = Path(__file__).resolve().parents[1] / "docs"

# Domain (documentation group) → section title, in reading order. Mirrors the
# Python api.md grouping: the four sensors first, then the shared foundation.
DOMAIN_ORDER = [
    "usb", "sr04", "vl53l4", "vl53l8", "bno086",
    "firmware", "dataset", "transport", "protocol",
]
DOMAIN_TITLE = {
    "usb": "USB identity",
    "sr04": "SR04",
    "vl53l4": "VL53L4CD (ToF)",
    "vl53l8": "VL53L8 (ToF)",
    "bno086": "BNO086 (IMU)",
    "firmware": "Firmware update",
    "dataset": "Datasets (record & replay)",
    "transport": "Transport",
    "protocol": "Protocol codecs",
}

# Nicer-than-alphabetical file ordering within a domain (headline codecs first).
FILE_PRIORITY = [
    "Sr04.cs",
    "Vl53l4Wire.cs", "Vl53l4Uld.cs",
    "FrameDecoder.cs", "Vl53l8Stream.cs", "Advanced.cs", "Cnh.cs", "Vl53l8Uld.cs",
    "Reports.cs", "Sh2Control.cs", "Shtp.cs",
    "FwDepz.cs",
    "Dataset.cs",
    "Framing.cs", "PacketParser.cs", "Events.cs", "Crc.cs", "CrcType.cs",
    "Common.cs", "Identity.cs",
    "UsbIds.cs",
]

# The VL53L8 ToF domain is one namespace but two sensors. CH-specific top-level
# symbols go to the vl53l8ch reference; everything else is the shared CX base.
VL53L8CH_SYMBOLS = {"Vl53l8Cnh"}


# --------------------------------------------------------------------------
# Source sanitization: blank out string/char-literal contents and comments so
# brace/paren matching over the code is never confused by braces inside strings
# (e.g. interpolated ``$"...{x}..."``) or by ``///`` doc text. Returns the
# sanitized text (same length as the input) plus the raw text for slicing.

def _sanitize(text: str) -> str:
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = i
            while j < n and text[j] != "\n":
                out[j] = " "
                j += 1
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            j = i
            while j < n and not (text[j] == "*" and j + 1 < n and text[j + 1] == "/"):
                if text[j] != "\n":
                    out[j] = " "
                j += 1
            # blank the closing */
            for k in (j, j + 1):
                if k < n and text[k] != "\n":
                    out[k] = " "
            i = j + 2
        elif c == '"':
            verbatim = i > 0 and text[i - 1] == "@"
            out[i] = " "
            j = i + 1
            while j < n:
                if not verbatim and text[j] == "\\":
                    if text[j] != "\n":
                        out[j] = " "
                    if j + 1 < n and text[j + 1] != "\n":
                        out[j + 1] = " "
                    j += 2
                    continue
                if text[j] == '"':
                    if verbatim and j + 1 < n and text[j + 1] == '"':
                        out[j] = out[j + 1] = " "
                        j += 2
                        continue
                    out[j] = " "
                    j += 1
                    break
                if text[j] != "\n":
                    out[j] = " "
                j += 1
            i = j
        elif c == "'":
            out[i] = " "
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    out[j] = " "
                    if j + 1 < n:
                        out[j + 1] = " "
                    j += 2
                    continue
                if text[j] == "'":
                    out[j] = " "
                    j += 1
                    break
                out[j] = " "
                j += 1
            i = j
        else:
            i += 1
    return "".join(out)


# --------------------------------------------------------------------------
# XML-doc summary extraction.

_TAG = re.compile(r"<[^>]+>")


def _clean_doc(inner_lines: list[str]) -> str:
    """Turn a run of ``///`` inner texts into a one-paragraph summary."""
    text = "\n".join(inner_lines)
    m = re.search(r"<summary>(.*?)</summary>", text, re.DOTALL)
    if m:
        text = m.group(1)
    else:
        # No <summary> wrapper: drop everything from the first other tag on
        # (<param>, <returns>, <remarks>, …) — keep the leading prose.
        cut = re.search(r"<(param|returns|remarks|typeparam|exception|example)\b", text)
        if cut:
            text = text[:cut.start()]
    text = text.replace("<para>", "\n").replace("</para>", "\n")
    # <see cref="X.Y"/> → `Y`; <c>code</c> → `code`.
    text = re.sub(r'<see\s+cref="([^"]+)"\s*/>',
                  lambda m: "`" + m.group(1).split(".")[-1] + "`", text)
    text = re.sub(r"<c>(.*?)</c>", r"`\1`", text, flags=re.DOTALL)
    text = re.sub(r"<paramref\s+name=\"([^\"]+)\"\s*/>", r"`\1`", text)
    text = _TAG.sub("", text)
    text = text.replace("&lt;", "<").replace("&gt;", ">").replace("&amp;", "&")
    # Collapse internal whitespace to a single space per paragraph.
    paras = [re.sub(r"\s+", " ", p).strip() for p in re.split(r"\n\s*\n", text)]
    return "\n\n".join(p for p in paras if p).strip()


# --------------------------------------------------------------------------
# Parser: line-oriented, brace-matched over the sanitized source.

KIND_RE = re.compile(r"\b(enum|class|record|struct|interface)\b")


class Sym:
    def __init__(self, name, kind, signature, doc):
        self.name = name
        self.kind = kind            # 'enum' | 'class' | 'record' | 'struct' | 'method' | 'property' | 'const' | 'field' | 'enum-member'
        self.signature = signature  # csharp code line (types/members) or None
        self.doc = doc
        self.members: list[Sym] = []


class Parser:
    def __init__(self, text: str):
        self.raw = text
        self.san = _sanitize(text)
        self.rlines = text.split("\n")
        self.slines = self.san.split("\n")

    # -- doc gathering ------------------------------------------------------
    def _doc_above(self, idx: int) -> str:
        inner: list[str] = []
        i = idx - 1
        while i >= 0:
            s = self.rlines[i].strip()
            if s.startswith("///"):
                inner.append(s[3:].lstrip())
                i -= 1
                continue
            if s == "" and inner:
                break
            if s.startswith("[") and inner:   # attribute between doc and decl
                i -= 1
                continue
            break
        inner.reverse()
        return _clean_doc(inner) if inner else ""

    # -- head collection ----------------------------------------------------
    def _collect_head(self, i: int):
        """From line i, gather a declaration head. Returns
        (raw_head, term, brace_line, brace_col, next_line). term ∈ {'{',';','=>'}."""
        depth = 0
        assigned = False
        raw_parts: list[str] = []
        j = i
        while j < len(self.slines):
            s = self.slines[j]
            r = self.rlines[j]
            raw_parts.append(r)
            k = 0
            while k < len(s):
                c = s[k]
                if c in "([":
                    depth += 1
                elif c in ")]":
                    depth -= 1
                elif c == "{":
                    if depth == 0 and not assigned:
                        return ("\n".join(raw_parts), "{", j, k, j)
                    depth += 1
                elif c == "}":
                    depth -= 1
                elif c == ";" and depth == 0:
                    return ("\n".join(raw_parts), ";", None, None, j)
                elif c == "=" and depth == 0:
                    prev = s[k - 1] if k > 0 else " "
                    nxt = s[k + 1] if k + 1 < len(s) else " "
                    if nxt == ">":
                        return ("\n".join(raw_parts), "=>", None, None, j)
                    if prev not in "=<>!" and nxt != "=":
                        assigned = True
                k += 1
            j += 1
        return ("\n".join(raw_parts), ";", None, None, len(self.slines) - 1)

    def _match_brace(self, line: int, col: int):
        """Given ``{`` at (line,col) in sanitized text, return the (line) index
        of the line holding the matching ``}`` and the char index after it."""
        depth = 0
        j, k = line, col
        while j < len(self.slines):
            s = self.slines[j]
            while k < len(s):
                if s[k] == "{":
                    depth += 1
                elif s[k] == "}":
                    depth -= 1
                    if depth == 0:
                        return j, k
                k += 1
            j += 1
            k = 0
        return len(self.slines) - 1, 0

    @staticmethod
    def _tidy(sig: str) -> str:
        sig = re.sub(r"\s+", " ", sig).strip()
        sig = sig.replace("( ", "(").replace(" )", ")").replace(" ,", ",")
        return sig

    @staticmethod
    def _norm(head: str, term: str) -> str:
        """One-line normalized signature: everything up to the body/terminator."""
        san = _sanitize(head)
        cut = len(head)
        depth = 0
        for k, c in enumerate(san):
            if c in "([":
                depth += 1
            elif c in ")]":
                depth -= 1
            elif depth == 0 and c == "{":
                cut = k
                break
            elif depth == 0 and c == "=" and san[k:k + 2] == "=>":
                cut = k
                break
            elif depth == 0 and c == ";":
                cut = k
                break
        return Parser._tidy(head[:cut])

    @staticmethod
    def _sig_name(head: str) -> str:
        m = re.search(r"\b(enum|class|record|struct|interface)\s+([A-Za-z_]\w*)", head)
        if m:
            return m.group(2)
        # member: last identifier before '(' or before '{'/'=>'/end
        h = re.split(r"[({=]", head, maxsplit=1)[0]
        ids = re.findall(r"[A-Za-z_]\w*", h)
        return ids[-1] if ids else "?"

    # -- scope parsing ------------------------------------------------------
    def parse(self) -> list[Sym]:
        start = 0
        for idx, s in enumerate(self.slines):
            if s.strip().startswith("namespace"):
                start = idx + 1
                break
        syms, _ = self._parse_scope(start, len(self.slines))
        return syms

    def _parse_scope(self, start: int, end: int):
        """Parse the members declared directly in [start, end). Returns
        (symbols, index_after_closing_brace)."""
        out: list[Sym] = []
        i = start
        while i < end:
            s = self.slines[i].strip()
            r = self.rlines[i].strip()
            if s == "" or r.startswith("//") or r.startswith("["):
                i += 1
                continue
            if s.startswith("}"):
                return out, i + 1

            head, term, bl, bc, nxt = self._collect_head(i)
            hsan = _sanitize(head)
            is_public = bool(re.search(r"\bpublic\b", hsan.split("(")[0]))
            kmatch = re.search(r"\b(enum|class|record|struct|interface)\s+([A-Za-z_]\w*)", hsan)
            doc = self._doc_above(i)
            name = self._sig_name(head)

            if kmatch:
                kind = kmatch.group(1)
                sym = Sym(name, kind, self._norm(head, term), doc)
                if term == "{":
                    bend_line, _ = self._match_brace(bl, bc)
                    if kind == "enum":
                        sym.members = self._parse_enum(bl, bend_line)
                    else:
                        sym.members, _ = self._parse_scope(bl + 1, bend_line)
                    i = bend_line + 1
                else:
                    i = nxt + 1
                if is_public:
                    out.append(sym)
                continue

            # member — advance past its body/terminator first
            if term == "{":
                bend_line, _ = self._match_brace(bl, bc)
                i = bend_line + 1
            elif term == "=>":
                i = self._skip_to_semicolon(nxt)
            else:
                i = nxt + 1
            if not is_public:
                continue

            declarator = re.split(r"(?<![=<>!])=(?!=)|=>|\{", hsan, maxsplit=1)[0]
            has_paren = "(" in declarator
            if has_paren:
                kind = "method"
            elif re.search(r"\bconst\b", declarator):
                kind = "const"
            elif term in ("{", "=>"):
                kind = "property"
            else:
                kind = "field"
            out.append(Sym(name, kind, self._member_sig(head, term, kind), doc))
        return out, i

    def _skip_to_semicolon(self, i: int) -> int:
        j = i
        while j < len(self.slines):
            if ";" in self.slines[j]:
                return j + 1
            j += 1
        return j

    def _member_sig(self, head: str, term: str, kind: str) -> str:
        san = _sanitize(head)
        if kind == "property":
            cut = len(head)
            for tok in ("{", "=>"):
                p = san.find(tok)
                if p != -1:
                    cut = min(cut, p)
            return Parser._tidy(head[:cut])
        if kind in ("const", "field"):
            # declarator up to the '=' (if any); append a short one-line value,
            # cutting at the sanitized ';' so trailing '// comments' never leak.
            semi = san.find(";")
            body = head[:semi] if semi != -1 else head
            m = re.search(r"(?<![=<>!])=(?!=)", _sanitize(body))
            if m is None:
                return Parser._tidy(body)
            declr = Parser._tidy(body[:m.start()])
            val = body[m.start() + 1:].strip()
            one_line = "\n" not in val and len(val) <= 60
            return f"{declr} = {Parser._tidy(val)}" if one_line else f"{declr} = …"
        return self._norm(head, term)

    def _parse_enum(self, start_line: int, end_line: int) -> list[Sym]:
        out: list[Sym] = []
        i = start_line
        # move past the line containing '{'
        while i <= end_line and "{" not in self.slines[i]:
            i += 1
        i += 1
        while i < end_line:
            r = self.rlines[i].strip()
            s = self.slines[i].strip()
            if s == "" or r.startswith("///") or r.startswith("//"):
                i += 1
                continue
            if s.startswith("}"):
                break
            m = re.match(r"([A-Za-z_]\w*)\s*(=\s*[^,]+)?,?", s)
            if m:
                name = m.group(1)
                val = (m.group(2) or "").strip().lstrip("=").strip()
                doc = self._doc_above(i)
                sig = f"{name} = {val}" if val else name
                out.append(Sym(name, "enum-member", sig, doc))
            i += 1
        return out


# --------------------------------------------------------------------------
# Markdown rendering.

def _anchor(text: str) -> str:
    a = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"\s+", "-", a)


def _render_member(parent: str, m: Sym, depth: int) -> list[str]:
    h = "#" * depth
    lines: list[str] = []
    if m.kind == "property":
        lines += [f"{h} {parent}.{m.name} *(property)*", ""]
        if m.signature:
            lines += ["```csharp", m.signature, "```", ""]
    elif m.kind in ("const", "field"):
        tag = "constant" if m.kind == "const" else "field"
        lines += [f"{h} {parent}.{m.name} *({tag})*", "", "```csharp", m.signature, "```", ""]
    elif m.kind == "enum-member":
        # rendered inside the enum body block, handled by caller
        return []
    elif m.kind in ("enum", "class", "record", "struct", "interface"):
        # nested type
        lines += [f"{h} {parent}.{m.name}", "", "```csharp", m.signature, "```", ""]
        if m.doc:
            lines += [m.doc, ""]
        lines += _render_type_body(f"{parent}.{m.name}", m, depth + 1)
        return lines
    else:  # method
        tag = " *(constructor)*" if m.name == parent.split(".")[-1] else ""
        lines += [f"{h} {parent}.{m.name}{tag}", "", "```csharp", m.signature, "```", ""]
    if m.doc:
        lines += [m.doc, ""]
    return lines


def _render_type_body(name: str, sym: Sym, member_depth: int) -> list[str]:
    lines: list[str] = []
    if sym.kind == "enum":
        vals = [m for m in sym.members if m.kind == "enum-member"]
        if vals:
            block = ["```csharp", f"{sym.signature}", "{"]
            for v in vals:
                block.append(f"    {v.signature},")
            block += ["}", "```", ""]
            lines += block
        return lines
    for m in sym.members:
        if m.kind == "enum-member":
            continue
        lines += _render_member(name, m, member_depth)
    return lines


def _render_type(sym: Sym) -> list[str]:
    lines = [f"### {sym.name}", ""]
    if sym.kind == "enum":
        lines += _render_type_body(sym.name, sym, 4)
        if sym.doc:
            # doc after the enum block for readability
            lines += [sym.doc, ""]
        # If no enum values were captured, still show the declaration.
        if not any(m.kind == "enum-member" for m in sym.members):
            lines += ["```csharp", sym.signature, "```", ""]
        return lines
    lines += ["```csharp", sym.signature, "```", ""]
    if sym.doc:
        lines += [sym.doc, ""]
    lines += _render_type_body(sym.name, sym, 4)
    return lines


def _render_reference(groups, header_lines: list[str]) -> str:
    head = list(header_lines) + ["## Contents", ""]
    for _domain, title, syms in groups:
        links = ", ".join(f"[`{s.name}`](#{_anchor(s.name)})" for s in syms)
        head.append(f"- **{title}**: {links}")
    head.append("")
    body: list[str] = []
    for _domain, title, syms in groups:
        body += [f"## {title}", ""]
        for sym in syms:
            body += _render_type(sym)
    return "\n".join(head + body).rstrip() + "\n"


# --------------------------------------------------------------------------
# Collection + main.

def _collect():
    by_domain: dict[str, list[Sym]] = {}
    files = sorted(SRC.rglob("*.cs"))

    def prio(p: Path):
        b = p.name
        return (FILE_PRIORITY.index(b) if b in FILE_PRIORITY else len(FILE_PRIORITY), str(p))

    for path in sorted(files, key=prio):
        folder = path.parent.name
        base = path.name
        if folder == "Usb":
            domain = "usb"
        elif folder == "Transport":
            domain = "transport"
        elif folder == "Dataset":
            domain = "dataset"
        elif folder == "Vl53l4":
            domain = "vl53l4"
        elif folder == "Vl53l8":
            domain = "vl53l8"
        elif folder == "Bno086":
            domain = "bno086"
        elif folder == "Protocol":
            domain = {"Sr04.cs": "sr04", "FwDepz.cs": "firmware"}.get(base, "protocol")
        else:
            continue
        syms = Parser(path.read_text()).parse()
        by_domain.setdefault(domain, []).extend(syms)

    ordered = DOMAIN_ORDER + [d for d in by_domain if d not in DOMAIN_ORDER]
    return [(d, DOMAIN_TITLE.get(d, d.title()), by_domain[d])
            for d in ordered if by_domain.get(d)]


def _sensor_targets(groups):
    by = {d: (t, syms) for d, t, syms in groups}
    targets = []
    if "sr04" in by:
        t, syms = by["sr04"]
        targets.append(("sr04", "SR04", syms, []))
    if "vl53l4" in by:
        _t, syms = by["vl53l4"]
        targets.append(("vl53l4cd", "VL53L4CD (ToF, single-zone)", syms, []))
    if "vl53l8" in by:
        _t, syms = by["vl53l8"]
        cx = [s for s in syms if s.name not in VL53L8CH_SYMBOLS]
        ch = [s for s in syms if s.name in VL53L8CH_SYMBOLS]
        targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx, []))
        targets.append((
            "vl53l8ch", "VL53L8CH (ToF + CNH)", ch,
            ["`Vl53l8Cnh` is the VL53L8CH-specific extension over the shared CX/CH",
             "decode path — the CX base surface (results-frame decode, chunk",
             "reassembly and the advanced DCI codecs) serves both variants and is",
             "documented in the [VL53L8CX API reference](../vl53l8cx/api.md).",
             ""],
        ))
    if "bno086" in by:
        t, syms = by["bno086"]
        targets.append(("bno086", "BNO086 (IMU)", syms, []))
    return targets


def main() -> None:
    groups = _collect()

    root_head = [
        "# API reference",
        "",
        "Auto-generated from the public C# surface under `src/Depz.Sensor/` by",
        "`scripts/gen_api_md.py` — run it to regenerate (see below). Edit the",
        "`///` XML-doc summaries in the source, not this file.",
        "",
        "Each sensor also has a focused reference with just its own symbols:",
        "[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · "
        "[VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · "
        "[BNO086](bno086/api.md).",
        "",
        "Regenerate: `python3 scripts/gen_api_md.py` (from the package root).",
        "",
    ]
    DOCS.mkdir(parents=True, exist_ok=True)
    n = sum(len(s) for _, _, s in groups)
    (DOCS / "api.md").write_text(_render_reference(groups, root_head))
    print(f"wrote {DOCS / 'api.md'} ({n} public types across {len(groups)} groups)")

    for folder, title, syms, note in _sensor_targets(groups):
        sub_head = [
            f"# {title} — API reference",
            "",
            f"The public API for the {title} sensor. The shared foundation —",
            "transport framing, common protocol codecs, firmware image parse,",
            "dataset reader and USB identity — lives in the",
            "[top-level API reference](../api.md).",
            "",
            *note,
            "Auto-generated by `scripts/gen_api_md.py` — edit the `///` summaries",
            "in the source, not this file.",
            "",
        ]
        out = DOCS / folder / "api.md"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(_render_reference([(folder, title, syms)], sub_head))
        print(f"wrote {out} ({len(syms)} symbols)")


if __name__ == "__main__":
    main()
