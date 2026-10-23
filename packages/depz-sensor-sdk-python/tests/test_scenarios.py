"""Contract 15 device scenarios: the Python runner.

Each contracts/scenarios/<id>.json names a committed real-hardware capture
and the API calls that produced it. The runner opens the capture as its
`probe` field says (legacy: the old probe, then the class attached in place;
canonical: open_on_link) over a causal, strict_tx ReplayLink, so every byte
the SDK writes must equal the recorded tx stream, then executes the steps (§2), canonicalises each result (§3) and matches it
against `expect`. TS, C and C++ run the same files.
"""

import dataclasses
import enum
import functools
import json
import queue
import time
from pathlib import Path

import numpy as np
import pytest

from depz_sensor_sdk.bno055 import Bno055, Units
from depz_sensor_sdk.bno086 import Bno086
from depz_sensor_sdk.errors import DepzError, DepzTimeoutError, StatusError
from depz_sensor_sdk.protocol.identity import parse_software_name
from depz_sensor_sdk.sr04 import Sr04
from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.vl53l4 import Vl53l4cd
from depz_sensor_sdk.vl53l7 import Vl53l5cx, Vl53l7ch, Vl53l7cx
from depz_sensor_sdk.vl53l8 import CnhConfig, CnhMixin, Vl53l8ch, Vl53l8cx
from depz_sensor_sdk.vl53lx import (
    Vl53Error,
    Vl53l0x,
    Vl53l1cb,
    Vl53l1cx,
    Vl53l3cx,
    Vl53l4cx,
    Vl53lx,
)
from scenario_open import open_scenario_link

CONTRACTS = Path(__file__).resolve().parents[3] / "contracts"
SCENARIOS = CONTRACTS / "scenarios"
RECORDINGS = CONTRACTS / "vectors" / "recordings"
FILES = sorted(p.name for p in SCENARIOS.glob("*.json"))

TIMEOUT_S = 2.0  # request timeout, as the replay tests always used
TAKE_TIMEOUT_S = 5.0  # per streamed item; replay serves them instantly

# Contract 07 §1 layer 3: canonical class → Python class.
CLASSES = {
    "sr04": Sr04,
    "vl53l4cd": Vl53l4cd,
    "vl53l8cx": Vl53l8cx,
    "vl53l8ch": Vl53l8ch,
    "vl53l5cx": Vl53l5cx,
    "vl53l7cx": Vl53l7cx,
    "vl53l7ch": Vl53l7ch,
    "vl53l0x": Vl53l0x,
    "vl53l1cx": Vl53l1cx,
    "vl53l1cb": Vl53l1cb,
    "vl53l3cx": Vl53l3cx,
    "vl53l4cx": Vl53l4cx,
    "vl53lx": Vl53lx,
    "bno086": Bno086,
    "bno055": Bno055,
}

# The alias table (§2): canonical names Python spells differently, per class.
# A string is the Python attribute; a callable adapts the call (dev first).
ALIASES = {
    (Sr04, "measurements"): "stream",
    # product is keyword-only in Python, positional in the facade.
    (Vl53lx, "init"): lambda dev, driver=None, product=None: dev.init(driver, product=product),
    # No device-level accessor in Python: the bound driver carries them.
    (Vl53lx, "budget_range"): lambda dev: dev.driver.BUDGET_MS,
    (Vl53lx, "driver_reach_mm"): lambda dev: dev.driver.reach_mm(),
}


def _cnh_config(rec: dict) -> CnhConfig:
    cfg = CnhConfig()
    cfg.init_config(rec["start_bin"], rec["num_bins"], rec["sub_sample"])
    cfg.create_agg_map(rec["resolution"], *rec["agg_map"])
    return cfg


# Record arguments (§2): (class, call, position) → builder of the Python type.
RECORD_ARGS = {
    (Bno055, "configure", 1): lambda rec: Units(**rec),
    (CnhMixin, "configure_cnh", 0): _cnh_config,
}


def error_kind(exc: BaseException) -> str | None:
    """§4: the canonical kind of a Python exception, None for anything else."""
    if isinstance(exc, DepzTimeoutError):
        return "timeout"
    if isinstance(exc, StatusError):
        return "not_supported" if exc.status == 0x07 else "device_error"  # ERR_CMD_NOT_SUPPORTED
    if isinstance(exc, ValueError):
        return "invalid_argument"
    # The VL53Lx drivers refuse a mode they lack with Vl53Error, the same type
    # as a sensor that misbehaved; only the message tells them apart.
    if isinstance(exc, Vl53Error) and str(exc).startswith("no such mode"):
        return "invalid_argument"
    if isinstance(exc, DepzError) and " does not support " in str(exc):
        return "not_supported"
    return None


def canon(v):
    """§3: a result as canonical JSON (records keyed by Python field names)."""
    if v is None or isinstance(v, (bool, str)):
        return v
    if isinstance(v, enum.Enum):
        return canon(v.value)
    if isinstance(v, (int, float)):
        return v
    if isinstance(v, np.generic):
        return v.item()
    if isinstance(v, np.ndarray):
        return v.tolist()
    if isinstance(v, (bytes, bytearray, memoryview)):
        return bytes(v).hex()
    if dataclasses.is_dataclass(v):
        return {f.name: canon(getattr(v, f.name)) for f in dataclasses.fields(v)}
    if isinstance(v, tuple) and hasattr(v, "_fields"):  # namedtuple
        return {k: canon(x) for k, x in zip(v._fields, v)}
    if isinstance(v, (list, tuple)):
        return [canon(x) for x in v]
    if isinstance(v, dict):
        return {str(k): canon(x) for k, x in v.items()}
    if isinstance(v, (set, frozenset)):
        return sorted(canon(x) for x in v)
    if hasattr(v, "__dict__"):  # plain record class (e.g. HistogramBinData)
        return {k: canon(x) for k, x in vars(v).items() if not k.startswith("_")}
    raise TypeError(f"no canonical form for {type(v).__name__}")


def mismatch(got, want, path: str = "") -> str | None:
    """§3 matching; None when `got` matches `want`, else where and how not."""
    at = path or "result"
    if isinstance(want, dict):
        if not isinstance(got, dict):
            return f"{at}: expected an object, got {got!r}"
        for k, w in want.items():
            if k not in got:
                return f"{at}.{k}: missing (fields: {', '.join(got)})"
            m = mismatch(got[k], w, f"{path}.{k}")
            if m:
                return m
        return None
    if isinstance(want, list):
        if not isinstance(got, list):
            return f"{at}: expected an array, got {got!r}"
        if len(got) != len(want):
            return f"{at}: expected {len(want)} elements, got {len(got)}"
        for i, (g, w) in enumerate(zip(got, want)):
            m = mismatch(g, w, f"{path}[{i}]")
            if m:
                return m
        return None
    if isinstance(want, float):
        if isinstance(got, (int, float)) and not isinstance(got, bool):
            if abs(got - want) <= max(1e-9 * abs(want), 1e-12):
                return None
        return f"{at}: expected {want!r}, got {got!r}"
    # integers, strings, booleans, null: exact, bool never equal to an int
    if type(got) is not type(want) or got != want:
        return f"{at}: expected {want!r}, got {got!r}"
    return None


def resolve(dev, name: str):
    """The Python attribute behind a canonical name (alias table first)."""
    for (cls, canonical), target in ALIASES.items():
        if canonical == name and isinstance(dev, cls):
            return getattr(dev, target) if isinstance(target, str) else functools.partial(target, dev)
    if not hasattr(dev, name):
        pytest.fail(f"{type(dev).__name__} has no {name!r}: add it to the alias table")
    return getattr(dev, name)


def record_args(dev, name: str, args: list) -> list:
    out = []
    for i, a in enumerate(args):
        if isinstance(a, dict):
            build = next((b for (cls, n, pos), b in RECORD_ARGS.items()
                          if n == name and pos == i and isinstance(dev, cls)), None)
            if build is None:
                pytest.fail(f"{name} argument {i} is a record with no builder in RECORD_ARGS")
            a = build(a)
        out.append(a)
    return out


def open_scenario(sc: dict, link: ReplayLink):
    """Open the capture as its `probe` says (15_SCENARIOS.md §1): a legacy
    capture re-issues the old in-place probe, then resolve_sensor_class and
    attach in place; a canonical one goes through open_on_link. `usb_model`
    stands in for the USB enumeration a replay has none of."""
    dev, software = open_scenario_link(sc, link, timeout=TIMEOUT_S)
    if "identity" in sc:
        if software is None:
            dev.close()
            pytest.fail("`identity` is for legacy captures only")
        if m := mismatch(canon(parse_software_name(software)), sc["identity"], "identity"):
            dev.close()
            pytest.fail(m)
    return dev


def describe(step: dict) -> str:
    if "call" in step:
        return f"{step['call']}({', '.join(json.dumps(a) for a in step.get('args', []))})"
    if "subscribe" in step:
        return f"subscribe {step['subscribe']} as {step['as']}"
    if "take" in step:
        return f"take {step['count']} from {step['take']}"
    return f"get {step['get']}"


def run_step(dev, step: dict, subs: dict, sizes: dict) -> None:
    if "call" in step:
        fn = resolve(dev, step["call"])
        args = record_args(dev, step["call"], step.get("args", []))
        if "error" in step:
            try:
                result = fn(*args)
            except Exception as exc:  # noqa: BLE001 — classified below
                kind = error_kind(exc)
                assert kind == step["error"], (
                    f"expected error {step['error']}, got {kind or 'unmapped'} "
                    f"({type(exc).__name__}: {exc})")
                return
            pytest.fail(f"expected error {step['error']}, succeeded with {result!r}")
        result = fn(*args)
        if "expect" in step and (m := mismatch(canon(result), step["expect"])):
            pytest.fail(m)
    elif "subscribe" in step:
        subs[step["as"]] = resolve(dev, step["subscribe"])(maxsize=sizes[step["as"]])
    elif "take" in step:
        sub = subs[step["take"]]
        want = step["expect"]
        assert len(want) == step["count"], "count and expect disagree"
        for i, w in enumerate(want):
            try:
                item = sub._queue.get(timeout=TAKE_TIMEOUT_S)
            except queue.Empty:
                pytest.fail(f"got {i} of {step['count']} items, then nothing for {TAKE_TIMEOUT_S} s")
            if m := mismatch(canon(item), w, f"item[{i}]"):
                pytest.fail(m)
    elif "get" in step:
        value = resolve(dev, step["get"])
        if callable(value):
            value = value()
        if m := mismatch(canon(value), step["expect"], step["get"]):
            pytest.fail(m)
    else:
        pytest.fail(f"unknown step {step}")


def wait_exhausted(link: ReplayLink, timeout: float = 2.0) -> str | None:
    """`end: exhausted`: every recorded tx byte written, every rx delivered.
    ReplayLink exposes only the rx side; the tx count is read from its state."""
    deadline = time.monotonic() + timeout
    while not link.exhausted and time.monotonic() < deadline:
        time.sleep(0.01)
    tx_left = len(link._tx_stream) - link._tx_written
    if tx_left or not link.exhausted:
        return (f"capture not exhausted: {tx_left} tx bytes never written, "
                f"{len(link._rx) - link._rx_idx} rx chunks never delivered")
    return None


@pytest.mark.parametrize("name", FILES)
def test_scenario(name, seen):
    sc = json.loads((SCENARIOS / name).read_text(encoding="utf-8"))
    assert sc["schema"] == "depz.scenario/1"
    assert name == f"{sc['id']}.json"
    # No drops: each queue holds everything its scenario takes, with room
    # for what streams on past the last take (drop-oldest would eat items).
    sizes: dict[str, int] = {}
    for step in sc["steps"]:
        if "take" in step:
            sizes[step["take"]] = sizes.get(step["take"], 0) + step["count"]
    sizes = {k: v + 4096 for k, v in sizes.items()}

    link = ReplayLink(RECORDINGS / sc["recording"], strict_tx=True)
    dev = open_scenario(sc, link)
    subs: dict = {}
    try:
        assert type(dev) is CLASSES[sc["class"]], f"resolved {type(dev).__name__}, not {sc['class']}"
        for i, step in enumerate(sc["steps"]):
            try:
                run_step(dev, step, subs, sizes)
            except (AssertionError, pytest.fail.Exception) as exc:
                pytest.fail(f"{sc['id']} step {i} {describe(step)}: {exc}", pytrace=False)
            except Exception as exc:  # noqa: BLE001
                pytest.fail(f"{sc['id']} step {i} {describe(step)}: raised "
                            f"{type(exc).__name__}: {exc}", pytrace=False)
        for sub_name, sub in subs.items():
            assert sub.dropped_count == 0, f"{sub_name}: {sub.dropped_count} items dropped"
        if sc.get("end", "open") == "exhausted" and (m := wait_exhausted(link)):
            pytest.fail(f"{sc['id']}: {m}", pytrace=False)
    finally:
        dev.close()
    seen(f"scenarios/{sc['id']}")
    seen(f"recordings/{sc['recording']}")
