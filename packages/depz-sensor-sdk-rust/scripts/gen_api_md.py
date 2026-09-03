#!/usr/bin/env python3
"""Generate the API reference (Markdown) from the crate's public surface.

This walks ``src/**/*.rs`` and emits the same Markdown shape as the Python
SDK's ``docs/api.md``: a title, a Contents index, one ``## Domain`` section per
module group, and for every public symbol a ``### name`` heading with a fenced
```rust``` signature block and its ``///`` doc-comment.

Why a source parser (not ``rustdoc``): ``cargo +nightly rustdoc
--output-format json`` would give a richer model, but nightly is not always
available and the JSON format is unstable. The crate is small and rustfmt-
clean, so a line-oriented parser over the ``pub`` items + ``///`` doc-comments
is the safer, dependency-free default (Python stdlib only).

The doc-comments in the source are the single source of truth, so the
reference never drifts from the code.

Regenerate (from the crate root, ``packages/depz-sensor-sdk-rust``)::

    python3 scripts/gen_api_md.py

Outputs:

- ``docs/api.md`` — the whole public surface;
- ``docs/{sr04,vl53l4cd,vl53l8cx,vl53l8ch,bno086}/api.md`` — one focused
  reference per sensor (its own symbols only; the shared
  transport/protocol/discovery surface stays in the root). The single
  ``vl53l8`` module is TWO sensors — the CX base and the CH superset (CNH) —
  so it is split by symbol: the CH-only extension points land in ``vl53l8ch``,
  everything else in ``vl53l8cx``. The ``vl53l4`` module is one sensor and
  maps 1:1 onto ``vl53l4cd``.
"""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
DOCS = ROOT / "docs"

# Domain (module group) → section title, in reading order — mirrors the Python
# SDK's DOMAIN_ORDER / DOMAIN_TITLE.
DOMAIN_ORDER = [
    "discovery", "sr04", "vl53l4", "vl53l8", "bno086",
    "fwdepz", "dataset", "transport", "protocol", "json",
]
DOMAIN_TITLE = {
    "discovery": "Discovery",
    "sr04": "SR04",
    "vl53l4": "VL53L4CD (ToF)",
    "vl53l8": "VL53L8 (ToF)",
    "bno086": "BNO086 (IMU)",
    "fwdepz": "Bootloader / firmware update",
    "dataset": "Datasets (record & replay)",
    "transport": "Transport",
    "protocol": "Protocol codecs",
    "json": "JSON",
}

# CH-only symbols in the shared vl53l8 module. Everything else in the domain is
# the CX base; these are the VL53L8CH-specific additions/extension points.
VL53L8CH_SYMBOLS = {"CNH_DATA_IDX"}


def domain_for(path: Path) -> str | None:
    """Group key for a source file, or None for files with nothing to document
    (``lib.rs`` and the ``mod.rs`` re-export shims)."""
    p = str(path.relative_to(SRC)).replace("\\", "/")
    if p.startswith("vl53l4/"):
        return "vl53l4"
    if p.startswith("vl53l8/"):
        return "vl53l8"
    if p.startswith("bno086/"):
        return "bno086"
    if p == "protocol/sr04.rs":
        return "sr04"
    if p == "protocol/identity.rs":
        return "discovery"
    if p == "protocol/common.rs":
        return "protocol"
    if p == "usb_ids.rs":
        return "discovery"
    if p in ("framing.rs", "crc.rs"):
        return "transport"
    if p == "fwdepz.rs":
        return "fwdepz"
    if p == "dataset.rs":
        return "dataset"
    if p == "json.rs":
        return "json"
    return None


ITEM_RE = re.compile(r"^pub\s+(fn|struct|enum|const|static|type)\s+([A-Za-z_][A-Za-z0-9_]*)")
METHOD_RE = re.compile(r"^\s{4}pub\s+fn\s+([A-Za-z_][A-Za-z0-9_]*)")
IMPL_RE = re.compile(r"^impl(?:<[^>]*>)?\s+(.*?)\s*\{")


def _indent(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def _strip_doc(line: str) -> str:
    s = line.lstrip()
    s = s[3:]  # drop '///'
    if s.startswith(" "):
        s = s[1:]
    return s.rstrip()


def _dedent(parts: list[str]) -> str:
    widths = [_indent(l) for l in parts if l.strip()]
    cut = min(widths) if widths else 0
    return "\n".join(l[cut:] if l.strip() else "" for l in parts).rstrip()


def _capture_braced(lines: list[str], i0: int) -> int:
    """Return the index of the closing line of a brace block that opened on
    (or after) line ``i0``, matched by indentation of the start line."""
    close = " " * _indent(lines[i0]) + "}"
    seen = "{" in lines[i0]
    j = i0 + 1
    while j < len(lines):
        seen = seen or "{" in lines[j]
        if seen and lines[j].rstrip() == close:
            return j
        j += 1
    return len(lines) - 1


def _capture_signature(lines: list[str], i0: int) -> tuple[list[str], int]:
    """Collect a fn signature (up to the body ``{`` or a ``;``)."""
    parts: list[str] = []
    j = i0
    while j < len(lines):
        line = lines[j].rstrip()
        if "{" in line:
            parts.append(line[: line.index("{")].rstrip())
            return parts, j
        if line.endswith(";"):
            parts.append(line.rstrip(";").rstrip())
            return parts, j
        parts.append(line)
        j += 1
    return parts, j - 1


def _capture_semicolon(lines: list[str], i0: int) -> tuple[list[str], int]:
    parts: list[str] = []
    j = i0
    while j < len(lines):
        parts.append(lines[j].rstrip())
        if lines[j].rstrip().endswith(";"):
            return parts, j
        j += 1
    return parts, j - 1


def parse_file(path: Path):
    """Return a list of top-level items: dicts with name/kind/signature/doc and
    (for types) a list of method dicts."""
    lines = (path.read_text().split("\n"))
    items: list[dict] = []
    by_name: dict[str, dict] = {}
    pending: list[str] = []
    impl_target: str | None = None  # None outside impl, "" for a skipped impl
    i = 0
    n = len(lines)
    while i < n:
        raw = lines[i]
        stripped = raw.strip()

        # End of an impl block (rustfmt: closing brace at column 0).
        if impl_target is not None and raw.rstrip() == "}":
            impl_target = None
            pending = []
            i += 1
            continue

        if stripped.startswith("///"):
            pending.append(_strip_doc(raw))
            i += 1
            continue
        if stripped.startswith("#["):
            i += 1  # attribute: keep pending doc
            continue
        if stripped == "" or stripped.startswith("//"):
            pending = []
            i += 1
            continue

        # Method inside an inherent impl of a public type.
        m_method = METHOD_RE.match(raw)
        if m_method and impl_target:
            parent = by_name.get(impl_target)
            sig_parts, end = _capture_signature(lines, i)
            if parent is not None:
                parent["methods"].append({
                    "name": m_method.group(1),
                    "signature": _dedent(sig_parts),
                    "doc": "\n".join(pending).strip(),
                })
            pending = []
            i = end + 1
            continue

        # Enter an impl block (column 0).
        if raw.startswith("impl"):
            m_impl = IMPL_RE.match(raw)
            header = m_impl.group(1) if m_impl else ""
            if " for " in header:
                impl_target = ""  # trait impl: skip its methods
            else:
                impl_target = header.split("<")[0].strip()
            pending = []
            i += 1
            continue

        # Top-level public item (column 0).
        m_item = ITEM_RE.match(raw)
        if m_item:
            kind, name = m_item.group(1), m_item.group(2)
            if kind in ("struct", "enum"):
                if "{" in raw:
                    end = _capture_braced(lines, i)
                    sig = _dedent(lines[i:end + 1])
                else:  # tuple / unit struct, one line ending ';'
                    parts, end = _capture_semicolon(lines, i)
                    sig = _dedent(parts)
            elif kind == "fn":
                parts, end = _capture_signature(lines, i)
                sig = _dedent(parts)
            else:  # const / static / type
                parts, end = _capture_semicolon(lines, i)
                sig = _dedent(parts)
            item = {
                "name": name,
                "kind": kind,
                "signature": sig,
                "doc": "\n".join(pending).strip(),
                "methods": [],
            }
            items.append(item)
            if kind in ("struct", "enum"):
                by_name[name] = item
            pending = []
            i = end + 1
            continue

        pending = []
        i += 1
    return items


def collect():
    """Return {domain: [items...]} across the whole crate, files sorted."""
    by_domain: dict[str, list[dict]] = {}
    for path in sorted(SRC.rglob("*.rs")):
        domain = domain_for(path)
        if domain is None:
            continue
        for item in parse_file(path):
            by_domain.setdefault(domain, []).append(item)
    return by_domain


def _anchor(text: str) -> str:
    a = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"\s+", "-", a)


def _render_item(item: dict) -> list[str]:
    out = [f"### {item['name']}", "", "```rust", item["signature"], "```", ""]
    if item["doc"]:
        out += [item["doc"], ""]
    for m in item["methods"]:
        out += [f"#### {item['name']}::{m['name']}", "", "```rust", m["signature"], "```", ""]
        if m["doc"]:
            out += [m["doc"], ""]
    return out


def _render_reference(groups: list[tuple[str, str, list[dict]]], header: list[str]) -> str:
    head = list(header) + ["## Contents", ""]
    for _domain, title, members in groups:
        links = ", ".join(f"[`{it['name']}`](#{_anchor(it['name'])})" for it in members)
        head.append(f"- **{title}**: {links}")
    head.append("")

    body: list[str] = []
    for _domain, title, members in groups:
        body += [f"## {title}", ""]
        for it in members:
            body += _render_item(it)
    return "\n".join(head + body).rstrip() + "\n"


def _sensor_targets(by_domain: dict[str, list[dict]]):
    """[(folder, title, members, note_lines), ...] — one per per-sensor api.md.
    The vl53l8 module is split by symbol into the CX base and CH superset."""
    targets = []
    if by_domain.get("sr04"):
        targets.append(("sr04", "SR04", by_domain["sr04"], []))
    if by_domain.get("vl53l4"):
        targets.append(("vl53l4cd", "VL53L4CD (ToF, single-zone)", by_domain["vl53l4"], []))
    if by_domain.get("vl53l8"):
        members = by_domain["vl53l8"]
        cx = [it for it in members if it["name"] not in VL53L8CH_SYMBOLS]
        ch = [it for it in members if it["name"] in VL53L8CH_SYMBOLS]
        targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx, []))
        targets.append((
            "vl53l8ch", "VL53L8CH (ToF + CNH)", ch,
            ["`Vl53l8Ch` is the VL53L8CX superset: it shares one results-frame",
             "decoder, framing and advanced-DCI codec set with the CX base — only",
             "the Compact-Network-Histogram (CNH) additions are listed here. For",
             "the frame decoder, reassembler, `Variant`, resolution and the",
             "advanced ULD codecs, see the [VL53L8CX API reference](../vl53l8cx/api.md).",
             ""],
        ))
    if by_domain.get("bno086"):
        targets.append(("bno086", "BNO086 (IMU)", by_domain["bno086"], []))
    return targets


def main() -> None:
    by_domain = collect()
    ordered = DOMAIN_ORDER + [d for d in by_domain if d not in DOMAIN_ORDER]
    groups = [(d, DOMAIN_TITLE.get(d, d.title()), by_domain[d])
              for d in ordered if by_domain.get(d)]

    root_head = [
        "# API reference",
        "",
        "Auto-generated from the crate's public surface (the `pub` items and",
        "their `///` doc-comments across `src/**`) by `scripts/gen_api_md.py`.",
        "Regenerate with `python3 scripts/gen_api_md.py` from the crate root.",
        "Edit the doc-comments in the source, not this file.",
        "",
        "Each sensor also has a focused reference with just its own symbols:",
        "[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · "
        "[VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · "
        "[BNO086](bno086/api.md).",
        "",
    ]
    DOCS.mkdir(parents=True, exist_ok=True)
    (DOCS / "api.md").write_text(_render_reference(groups, root_head))
    n = sum(len(m) for _, _, m in groups)
    print(f"wrote {DOCS / 'api.md'} ({n} public symbols across {len(groups)} groups)")

    for folder, title, members, note in _sensor_targets(by_domain):
        sub_head = [
            f"# {title} — API reference",
            "",
            f"The public API for the {title} sensor. Transport, framing,",
            "identity/discovery, firmware-container and dataset symbols shared",
            "across sensors live in the [top-level API reference](../api.md).",
            "",
            *note,
            "Auto-generated by `scripts/gen_api_md.py` — edit the doc-comments in",
            "the source, not this file. Regenerate with `python3 scripts/gen_api_md.py`.",
            "",
        ]
        out = DOCS / folder / "api.md"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(_render_reference([(folder, title, members)], sub_head))
        print(f"wrote {out} ({len(members)} symbols)")


if __name__ == "__main__":
    main()
