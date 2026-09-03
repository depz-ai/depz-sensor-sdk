"""Generate the API reference from the public surface: Markdown + JSON.

The SDK declares its public contract via ``__all__`` in the top-level
package *and* in each subpackage. This walks that union, grouped by
module (a symbol re-exported at the top level is documented once, under
"Top level"), and writes two files in one run so they can never drift
from each other:

- ``docs/api.md`` — human-readable reference (signature + docstring;
  for classes, public methods and properties too);
- ``docs/api.json`` — machine-readable model of the same surface, for
  parity checks when porting (diff two dumps to see what changed) and
  for tooling. On top of runtime introspection it adds a static (ast)
  pass over the class bodies, which captures what introspection cannot:
  PEP 224 attribute docstrings on dataclass fields, enum members and
  plain class attributes.

The docstrings in the source are the single source of truth, so the
reference never drifts from the code. Regenerate with ``make docs`` and
commit both results. Stdlib only — no doc-tool dependencies.
"""

from __future__ import annotations

import ast
import dataclasses
import enum
import functools
import importlib
import inspect
import json
import math
import re
import textwrap
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "docs" / "api.md"
JSON_OUT = Path(__file__).resolve().parents[1] / "docs" / "api.json"

# Modules to scan for ``__all__`` (top level + every subpackage). A symbol
# is grouped by where it's *defined* (its ``__module__``), not by which of
# these re-exports it — so e.g. every inference class lands in "Inference"
# even though several are also top-level re-exports.
SCAN_MODULES = [
    "depz_sensor_sdk",
    "depz_sensor_sdk.transport",
    "depz_sensor_sdk.protocol",
    "depz_sensor_sdk.vl53l4",
    "depz_sensor_sdk.vl53l8",
    "depz_sensor_sdk.bno086",
]

# Domain (subpackage / top-level module) → section title, in reading order.
DOMAIN_ORDER = [
    "discovery", "device", "sr04", "vl53l4", "vl53l8", "bno086",
    "bootloader", "dataset", "transport", "protocol", "errors",
]
DOMAIN_TITLE = {
    "discovery": "Discovery", "device": "Device core", "sr04": "SR04",
    "vl53l4": "VL53L4CD (ToF)", "vl53l8": "VL53L8 (ToF)", "bno086": "BNO086 (IMU)",
    "bootloader": "Bootloader / firmware update", "dataset": "Datasets (record & replay)",
    "transport": "Transport", "protocol": "Protocol codecs", "errors": "Errors",
    "depz_sensor_sdk": "Top level",
}

# Per-sensor docs/<sensor>/api.md files (just that sensor's own symbols) so each
# sensor has a full intro/guide/api set without sending the reader to an anchor
# in the big top-level reference. The shared surface (discovery/device/transport
# /…) stays only in the root api.md. The VL53L8 ToF domain is one Python module
# but TWO sensors — the CX base and the CH superset (CNH) — so it is split by
# class: CH-specific symbols land in vl53l8ch, everything else in vl53l8cx.
VL53L8CH_SYMBOLS = {"Vl53l8Ch", "CnhConfig"}


def _sensor_api_targets(groups):
    """``[(folder, title, members, note_lines), ...]`` — one entry per
    per-sensor ``api.md``. The single ``vl53l8`` domain is split by class into
    the CX base and the CH superset; the other sensors map one-to-one."""
    by = {domain: (title, members) for domain, title, members in groups}
    targets: list[tuple[str, str, list, list[str]]] = []
    if "sr04" in by:
        title, members = by["sr04"]
        targets.append(("sr04", title, members, []))
    if "vl53l4" in by:
        title, members = by["vl53l4"]
        targets.append(("vl53l4cd", title, members, []))
    if "vl53l8" in by:
        _title, members = by["vl53l8"]
        cx = [(n, o) for n, o in members if n not in VL53L8CH_SYMBOLS]
        ch = [(n, o) for n, o in members if n in VL53L8CH_SYMBOLS]
        targets.append(("vl53l8cx", "VL53L8CX (ToF)", cx, []))
        targets.append((
            "vl53l8ch", "VL53L8CH (ToF + CNH)", ch,
            ["`Vl53l8Ch` is a superset of the VL53L8CX and inherits its entire",
             "configuration and ranging surface — only the CH-specific additions",
             "(Compact-Network-Histogram output) are listed here. For init,",
             "resolution, frequency, the advanced ULD features and the frame,",
             "see the [VL53L8CX API reference](../vl53l8cx/api.md).",
             ""],
        ))
    if "bno086" in by:
        title, members = by["bno086"]
        targets.append(("bno086", title, members, []))
    return targets


def _sig(obj) -> str:
    # eval_str=True resolves string / forward-ref annotations to real
    # objects so they render unquoted; fall back if evaluation fails.
    # Evaluating annotations runs arbitrary expressions, so anything can
    # come out — never let one symbol kill the whole generator.
    for eval_str in (True, False):
        try:
            return str(inspect.signature(obj, eval_str=eval_str))
        except Exception:
            continue
    return ""


# Cython's ``embedsignature`` prepends the call signature as the first
# line of every function/method docstring (e.g. ``foo(self) -> int``).
# We already render the signature ourselves, so strip that leading line.
_EMBEDDED_SIG = re.compile(r"^[\w.]+\(.*\)(\s*->.*)?$")


def _doc(obj) -> str:
    d = inspect.cleandoc(getattr(obj, "__doc__", "") or "")
    lines = d.split("\n")
    if lines and _EMBEDDED_SIG.match(lines[0].strip()):
        lines = lines[1:]
        if lines and not lines[0].strip():
            lines = lines[1:]
    return "\n".join(lines).strip()


def _anchor(text: str) -> str:
    """GitHub heading-anchor: lowercase, drop punctuation, spaces → '-'."""
    a = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"\s+", "-", a)


def _public_members(cls):
    """Public methods + properties across the MRO (excluding ``object``),
    in MRO + definition order, de-duped by name. Yields
    ``(name, member, owner)`` where ``owner`` is the defining class."""
    seen: set[str] = set()
    out = []
    for klass in cls.__mro__:
        if klass is object:
            continue
        for name, member in vars(klass).items():
            if name.startswith("_") or name in seen:
                continue
            if isinstance(member, (property, staticmethod, classmethod)) or callable(member):
                seen.add(name)
                out.append((name, member, klass))
    return out


def _enum_value(entry) -> str:
    """An enum_values entry is either a scalar or a ``(value, label)`` pair."""
    if isinstance(entry, (tuple, list)) and entry:
        return str(entry[0])
    return str(entry)


def _render_controls(cls) -> list[str]:
    """Table of the class's runtime controls (``_CONTROL_SPECS``). These are
    typed-descriptor class attributes — read via the attribute, written
    via the owner's setter (``set_inference_control`` / ``set_filter_control``
    / ``set_pipeline_control`` / ``set_calibration_control``). They're the
    main tunable surface and don't show up as ordinary methods."""
    specs = getattr(cls, "_CONTROL_SPECS", ()) or ()
    if not specs:
        return []
    lines = [
        "**Runtime controls** — read via the attribute, set via the owner's "
        "`set_*` API:",
        "",
        "| control | type | default | range / values | unit |",
        "|---|---|---|---|---|",
    ]
    for k in specs:
        enum = getattr(k, "enum_values", None)
        mn, mx, step = getattr(k, "min", None), getattr(k, "max", None), getattr(k, "step", None)
        if enum:
            rng = ", ".join(f"`{_enum_value(e)}`" for e in enum)
        elif mn is not None and mx is not None:
            rng = f"{mn}…{mx}" + (f" step {step}" if step else "")
        else:
            rng = "—"
        unit = getattr(k, "unit", None) or "—"
        lines.append(
            f"| `{k.name}` | {getattr(k, 'value_type', '') or ''} "
            f"| `{getattr(k, 'default', '')}` | {rng} | {unit} |"
        )
    lines.append("")
    return lines


def _render_class(name: str, cls) -> list[str]:
    lines = [f"### {name}", "", "```python", f"class {name}{_sig(cls)}", "```", ""]
    if (d := _doc(cls)):
        lines += [d, ""]
    lines += _render_controls(cls)
    for mname, member, _owner in _public_members(cls):
        if isinstance(member, property):
            lines += [f"#### {name}.{mname} *(property)*", ""]
            target = member.fget or member
        else:
            target = member.__func__ if isinstance(member, (staticmethod, classmethod)) else member
            kind = " *(classmethod)*" if isinstance(member, classmethod) else ""
            lines += [f"#### {name}.{mname}{kind}", "", "```python", f"{mname}{_sig(target)}", "```", ""]
        if (md := _doc(target)):
            lines += [md, ""]
    return lines


def _render_callable(name: str, obj) -> list[str]:
    lines = [f"### {name}", "", "```python", f"{name}{_sig(obj)}", "```", ""]
    if (d := _doc(obj)):
        lines += [d, ""]
    return lines


def _render_value(name: str, obj) -> list[str]:
    lines = [f"### {name}", "", f"`{type(obj).__name__}` constant.", ""]
    if (d := _doc(obj)):
        lines += [d, ""]
    return lines


def _domain(obj, found_in: str) -> str:
    """Group key: the subpackage / top-level module where the symbol is
    *defined* (its ``__module__``). Data constants without ``__module__``
    (e.g. a bare dict) fall back to the module that re-exported them."""
    dm = getattr(obj, "__module__", None) or ""
    parts = dm.split(".")
    if len(parts) >= 2 and parts[0] == "depz_sensor_sdk":
        return parts[1]
    fparts = found_in.split(".")
    return fparts[1] if len(fparts) >= 2 else found_in


def _collect():
    """``(groups, exports)`` where groups is
    ``[(section_title, [(name, obj), ...]), ...]`` — union of every
    ``__all__``, grouped by *defining* domain, each object documented
    once — and exports maps ``id(obj)`` to every public import path
    (``module.name``) that re-exports it."""
    seen: set[int] = set()
    by_domain: dict[str, list] = {}
    exports: dict[int, list[str]] = {}
    for mod_name in SCAN_MODULES:
        mod = importlib.import_module(mod_name)
        for name in dict.fromkeys(getattr(mod, "__all__", [])):
            obj = getattr(mod, name)
            if inspect.ismodule(obj):
                continue
            exports.setdefault(id(obj), []).append(f"{mod_name}.{name}")
            if id(obj) in seen:
                continue
            seen.add(id(obj))
            by_domain.setdefault(_domain(obj, mod_name), []).append((name, obj))
    ordered = DOMAIN_ORDER + [d for d in by_domain if d not in DOMAIN_ORDER]
    groups = [(d, DOMAIN_TITLE.get(d, d.title()), by_domain[d])
              for d in ordered if by_domain.get(d)]
    return groups, exports


# --------------------------------------------------------------------------
# JSON model (docs/api.json)
#
# Runtime introspection (same objects the Markdown pass uses) plus a
# static ast pass over class bodies for what introspection can't see:
# PEP 224 attribute docstrings ("""...""" right under a field/attribute
# assignment) on dataclass fields, enum members and class attributes.

# Object reprs may embed memory addresses; strip them or the dump (and
# the CI drift check) would churn on every run.
_ADDR = re.compile(r" at 0x[0-9a-fA-F]+")


def _jsonval(v):
    """A JSON-native scalar as-is; anything else as a sanitized repr.
    Non-finite floats become their repr — ``json.dumps`` would emit
    ``NaN``/``Infinity`` tokens, which are not valid strict JSON."""
    if isinstance(v, float) and not math.isfinite(v):
        return repr(v)
    if v is None or isinstance(v, (bool, int, float, str)):
        return v
    return _ADDR.sub("", repr(v))


def _ann(a) -> str | None:
    """Annotation as a string; ``None`` only for *absent* annotations.
    An explicit ``-> None`` / ``: None`` is meaningful and renders as
    ``"None"`` — don't conflate it with "not annotated"."""
    if a is inspect.Parameter.empty:
        return None
    return a if isinstance(a, str) else inspect.formatannotation(a)


def _params_json(obj) -> dict:
    """``{"parameters": [...], "returns": ...}`` from the signature, or
    ``{}`` when no signature is recoverable (builtins, descriptors)."""
    sig = None
    # Same catch-all rationale as _sig: annotation evaluation can raise
    # anything; degrade to the unevaluated form, then to no signature.
    for eval_str in (True, False):
        try:
            sig = inspect.signature(obj, eval_str=eval_str)
            break
        except Exception:
            continue
    if sig is None:
        return {}
    params = []
    for p in sig.parameters.values():
        entry = {"name": p.name, "kind": p.kind.name.lower()}
        if (a := _ann(p.annotation)) is not None:
            entry["annotation"] = a
        if p.default is not inspect.Parameter.empty:
            entry["default"] = _jsonval(p.default)
        params.append(entry)
    out: dict = {"parameters": params}
    if (r := _ann(sig.return_annotation)) is not None:
        out["returns"] = r
    return out


def _attr_docs(cls) -> dict[str, str]:
    """PEP 224 attribute docstrings for ``cls`` including inherited ones:
    base-class docs first, subclass overrides win — mirroring how
    ``dataclasses.fields`` accumulates fields across the MRO."""
    docs: dict[str, str] = {}
    for klass in reversed(cls.__mro__):
        if klass is not object:
            docs.update(_own_attr_docs(klass))
    return docs


@functools.cache
def _own_attr_docs(cls) -> dict[str, str]:
    """PEP 224 attribute docstrings from one class body — a string
    literal immediately following an assignment. Invisible at runtime;
    this is the static-analysis half of the JSON dump."""
    try:
        tree = ast.parse(textwrap.dedent(inspect.getsource(cls)))
    except (OSError, TypeError, SyntaxError, IndentationError):
        return {}
    cdef = next((n for n in tree.body if isinstance(n, ast.ClassDef)), None)
    if cdef is None:
        return {}
    docs: dict[str, str] = {}
    pending: str | None = None
    for node in cdef.body:
        if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            pending = node.target.id
        elif (isinstance(node, ast.Assign) and len(node.targets) == 1
              and isinstance(node.targets[0], ast.Name)):
            pending = node.targets[0].id
        elif (isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant)
              and isinstance(node.value.value, str) and pending):
            docs[pending] = inspect.cleandoc(node.value.value)
            pending = None
        else:
            pending = None
    return docs


def _source_json(obj) -> dict | None:
    """``{"file": ..., "line": ...}`` relative to the package root."""
    try:
        file, (_, line) = inspect.getsourcefile(obj), inspect.getsourcelines(obj)
    except (OSError, TypeError):
        return None
    if not file:
        return None
    parts = Path(file).parts
    if "depz_sensor_sdk" in parts:
        rel = str(Path(*parts[parts.index("depz_sensor_sdk"):]))
    else:
        rel = Path(file).name
    return {"file": rel, "line": line}


def _fields_json(cls) -> list[dict] | None:
    """Dataclass fields with types, defaults and attribute docstrings."""
    if not dataclasses.is_dataclass(cls):
        return None
    docs = _attr_docs(cls)
    out = []
    for f in dataclasses.fields(cls):
        if f.name.startswith("_"):
            continue
        entry: dict = {"name": f.name}
        if (t := _ann(f.type)) is not None:
            entry["type"] = t
        if f.default is not dataclasses.MISSING:
            entry["default"] = _jsonval(f.default)
        elif f.default_factory is not dataclasses.MISSING:  # type: ignore[misc]
            factory = f.default_factory  # type: ignore[misc]
            entry["default_factory"] = getattr(factory, "__name__", None) or _jsonval(factory)
        if (d := docs.get(f.name)):
            entry["doc"] = d
        out.append(entry)
    return out


def _enum_members_json(cls) -> list[dict] | None:
    if not (isinstance(cls, type) and issubclass(cls, enum.Enum)):
        return None
    docs = _attr_docs(cls)
    return [
        {"name": m.name, "value": _jsonval(m.value),
         **({"doc": d} if (d := docs.get(m.name)) else {})}
        for m in cls
    ]


def _attrs_json(cls, control_names: set[str]) -> list[dict]:
    """Plain public class attributes: annotated names + non-callable
    class-body values (constants, class-level defaults), accumulated
    across the MRO like methods are — but only from bases in the same
    root package as the class, so foreign bases (abc, numpy, ...) can't
    flood the dump. Dataclass fields, enum members and runtime controls
    are reported elsewhere."""
    docs = _attr_docs(cls)
    anns: dict = {}
    raw: dict = {}
    root = getattr(cls, "__module__", "").split(".")[0]
    for klass in reversed(cls.__mro__):
        if klass is not cls and getattr(
                klass, "__module__", "").split(".")[0] != root:
            continue
        try:
            anns.update(inspect.get_annotations(klass))
        except Exception:
            pass
        raw.update(vars(klass))
    field_names = ({f.name for f in dataclasses.fields(cls)}
                   if dataclasses.is_dataclass(cls) else set())
    names = list(anns) + [
        n for n, v in raw.items()
        if not (callable(v) or isinstance(v, (property, staticmethod, classmethod)))
    ]
    out = []
    for name in dict.fromkeys(names):
        if (name.startswith("_") or name in field_names or name in control_names):
            continue
        entry: dict = {"name": name}
        if name in anns:
            entry["type"] = _ann(anns[name])
        if name in raw:
            val = raw[name]
            if hasattr(type(val), "__get__"):
                # A descriptor (slot, getset on compiled classes, custom
                # typed attribute): the class-body value is machinery,
                # not a constant. Keep the name only if the annotation
                # or an attribute docstring says it's part of the API.
                if name not in anns and name not in docs:
                    continue
            else:
                entry["value"] = _jsonval(val)
        if (d := docs.get(name)):
            entry["doc"] = d
        out.append(entry)
    return out


def _controls_json(cls) -> list[dict]:
    out = []
    for k in getattr(cls, "_CONTROL_SPECS", ()) or ():
        entry: dict = {"name": k.name}
        for attr in ("value_type", "default", "min", "max", "step", "unit"):
            if (v := getattr(k, attr, None)) is not None:
                entry[attr] = _jsonval(v)
        if (ev := getattr(k, "enum_values", None)):
            entry["enum_values"] = [_enum_value(e) for e in ev]
        for attr in ("doc", "description", "help"):
            if (d := getattr(k, attr, None)):
                entry["doc"] = str(d)
                break
        out.append(entry)
    return out


def _member_json(name: str, member, owner, cls) -> dict:
    if isinstance(member, property):
        kind, target = "property", (member.fget or member)
    elif isinstance(member, classmethod):
        kind, target = "classmethod", member.__func__
    elif isinstance(member, staticmethod):
        kind, target = "staticmethod", member.__func__
    else:
        kind, target = "method", member
    entry: dict = {"name": name, "kind": kind}
    if kind == "property":
        # No call signature, but the getter's return annotation is the
        # property's type — worth keeping for parity checks.
        if (r := _params_json(target).get("returns")) is not None:
            entry["returns"] = r
    else:
        entry.update(_params_json(target))
    if (d := _doc(target)):
        entry["doc"] = d
    if owner is not cls:
        entry["defined_in"] = f"{owner.__module__}.{owner.__qualname__}"
    return entry


def _symbol_json(name: str, obj, exports: dict[int, list[str]]) -> dict:
    entry: dict = {"name": name}
    if inspect.isclass(obj):
        entry["kind"] = "class"
        entry["bases"] = [f"{b.__module__}.{b.__qualname__}"
                          for b in obj.__bases__ if b is not object]
        entry.update(_params_json(obj))
    elif callable(obj):
        entry["kind"] = "function"
        entry.update(_params_json(obj))
    else:
        entry["kind"] = "data"
        entry["type"] = type(obj).__name__
        entry["value"] = _jsonval(obj)
    if (m := getattr(obj, "__module__", None)):
        entry["module"] = m
    if (src := _source_json(obj)):
        entry["source"] = src
    entry["exported_as"] = exports.get(id(obj), [])
    if (d := _doc(obj)):
        entry["doc"] = d
    if inspect.isclass(obj):
        controls = _controls_json(obj)
        if (em := _enum_members_json(obj)) is not None:
            # Enum members double as class attributes — report them only
            # as enum_members, not again under "attributes".
            entry["enum_members"] = em
        else:
            if (df := _fields_json(obj)) is not None:
                entry["dataclass_fields"] = df
            if (attrs := _attrs_json(obj, {c["name"] for c in controls})):
                entry["attributes"] = attrs
        if controls:
            entry["controls"] = controls
        entry["members"] = [_member_json(mn, mm, own, obj)
                            for mn, mm, own in _public_members(obj)]
    return entry


def _write_json(groups, exports) -> int:
    model = {
        "note": "Auto-generated from the package's public surface by "
                "scripts/gen_api_md.py (alongside docs/api.md) — run "
                "`make docs` to regenerate. Edit the source, not this file.",
        "package": "depz_sensor_sdk",
        "groups": [
            {"title": title,
             "symbols": [_symbol_json(n, o, exports) for n, o in members]}
            for _domain, title, members in groups
        ],
    }
    # allow_nan=False turns any non-finite float that slipped past
    # _jsonval into a hard error instead of silently emitting NaN /
    # Infinity tokens (not valid strict JSON).
    JSON_OUT.write_text(
        json.dumps(model, indent=2, ensure_ascii=False, sort_keys=False,
                   allow_nan=False) + "\n")
    return sum(len(g["symbols"]) for g in model["groups"])


def _render_reference(groups, header_lines: list[str]) -> str:
    """Full Markdown reference (intro + contents index + per-group bodies)
    for the given ``[(domain, title, members), ...]``. Used for both the
    top-level api.md (all groups) and each per-sensor api.md (one group)."""
    head = list(header_lines) + ["## Contents", ""]
    for _domain, title, members in groups:
        links = ", ".join(f"[`{n}`](#{_anchor(n)})" for n, _ in members)
        head.append(f"- **{title}**: {links}")
    head.append("")

    body: list[str] = []
    for _domain, title, members in groups:
        body += [f"## {title}", ""]
        for name, obj in members:
            if inspect.isclass(obj):
                body += _render_class(name, obj)
            elif callable(obj):
                body += _render_callable(name, obj)
            else:
                body += _render_value(name, obj)
    return "\n".join(head + body).rstrip() + "\n"


def main() -> None:
    groups, exports = _collect()

    root_head = [
        "# API reference",
        "",
        "Auto-generated from the package's public surface (the union of",
        "`__all__` across `depz_sensor_sdk` and its subpackages) by",
        "`scripts/gen_api_md.py` — run `make docs` to regenerate. Edit the",
        "docstrings in the source, not this file.",
        "",
        "Each sensor also has a focused reference with just its own symbols:",
        "[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · "
        "[VL53L8CX](vl53l8cx/api.md) · "
        "[VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).",
        "",
    ]
    n_syms = sum(len(m) for _, _, m in groups)
    OUT.write_text(_render_reference(groups, root_head))
    print(f"wrote {OUT} ({n_syms} public symbols across {len(groups)} groups)")

    # Per-sensor references: one sensor each, common surface stays in the root.
    # The ToF module is split by class into the CX base and the CH superset.
    for folder, title, members, note_lines in _sensor_api_targets(groups):
        if not members:
            continue
        sub_head = [
            f"# {title} — API reference",
            "",
            f"The public API for the {title} sensor. Discovery, device-base,",
            "transport and other cross-sensor symbols shared by every sensor",
            "live in the [top-level API reference](../api.md).",
            "",
            *note_lines,
            "Auto-generated by `scripts/gen_api_md.py` — edit the docstrings in",
            "the source, not this file.",
            "",
        ]
        out = OUT.parent / folder / "api.md"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(_render_reference([(folder, title, members)], sub_head))
        print(f"wrote {out} ({len(members)} symbols)")

    n_json = _write_json(groups, exports)
    print(f"wrote {JSON_OUT} ({n_json} symbols)")


if __name__ == "__main__":
    main()
