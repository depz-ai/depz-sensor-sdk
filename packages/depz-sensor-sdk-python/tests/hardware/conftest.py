"""Fixtures for the real-hardware QA suite.

Collection is cheap and side-effect free; the *probe* happens once per session
and only when a hardware test is actually selected, so `pytest -m "not
hardware"` never touches a device.

Selection contract
------------------
A hardware test declares the family it needs (``sr04_dev``, ``cx_dev``,
``ch_dev``, ``bno_dev``) and is **skipped only when that unit is physically
absent**. A device that is present but fails to answer is a FAILURE, never a
skip — masking a broken device as "skipped" is exactly how hardware rot goes
unnoticed.
"""

from __future__ import annotations

import os
import random
import time
from typing import Iterator

import pytest

from _support import (  # noqa: E402  (tests dir is on sys.path via rootdir conftest)
    ALL_FAMILIES,
    FAMILY_BNO086,
    FAMILY_SR04,
    FAMILY_VL53L4CD,
    FAMILY_VL53L8CH,
    FAMILY_VL53L8CX,
    HwDevice,
    discover_hw,
    enable_faulthandler,
    install_byte_trace,
    open_family,
    port_lease,
    sample_resources,
    toolchain_info,
    wait_for_threads_to_die,
    write_report,
)

if os.environ.get("DEPZ_QA_BYTE_TRACE") == "1":
    install_byte_trace()

# ── profiles ─────────────────────────────────────────────────────────────────
#
# Cycle counts are the knob that turns a 30 s smoke run into an overnight soak.
# Defaults stay at "smoke" so an unsuspecting `pytest -m hardware` cannot burn
# an hour; CI/QA runs pass --profile explicitly.

PROFILES = {
    "smoke": {"cycles": 10, "duration_s": 20},
    "regression": {"cycles": 50, "duration_s": 120},
    "stress": {"cycles": 200, "duration_s": 600},
    "extended": {"cycles": 1000, "duration_s": 1800},
    "soak": {"cycles": 200, "duration_s": 1800},
}


def pytest_addoption(parser: pytest.Parser) -> None:
    g = parser.getgroup("depz-hardware")
    g.addoption("--profile", default="smoke", choices=sorted(PROFILES), help="hardware test profile")
    g.addoption("--cycles", type=int, default=None, help="override the profile cycle count")
    g.addoption("--duration", type=float, default=None, help="override the profile soak duration (s)")
    g.addoption("--seed", type=int, default=None, help="RNG seed for jitter (default: time-based, printed)")
    g.addoption("--sensor", default=None, help="restrict hardware tests to one family (sr04|vl53l4cd|vl53l8cx|vl53l8ch|bno086)")
    g.addoption("--port", default=None, help="restrict hardware tests to one device path")


def _resolve_profile(config: pytest.Config) -> dict:
    name = config.getoption("--profile")
    prof = dict(PROFILES[name], name=name)
    if (c := config.getoption("--cycles")) is not None:
        prof["cycles"] = c
    if (d := config.getoption("--duration")) is not None:
        prof["duration_s"] = d
    return prof


@pytest.fixture(scope="session")
def profile(pytestconfig: pytest.Config) -> dict:
    return _resolve_profile(pytestconfig)


def pytest_collection_modifyitems(config: pytest.Config, items: list) -> None:
    """Give duration-driven soak tests their real budget.

    pytest-timeout gives a *marker* precedence over the CLI value, so the
    multi-sensor module's 600 s ceiling would kill any soak longer than
    ~10 min — including the extended profile's 1800 s and an overnight run —
    while the CLI --timeout can never raise it. Stamp the soak test with the
    resolved duration plus teardown margin: still bounded, never mark-capped.
    """
    duration = float(_resolve_profile(config)["duration_s"])
    for item in items:
        if item.name.startswith("test_multi_sensor_soak"):
            item.add_marker(pytest.mark.timeout(duration + 600))


@pytest.fixture(scope="session")
def seed(pytestconfig: pytest.Config) -> int:
    """A recorded seed — a race found with jitter must stay reproducible."""
    s = pytestconfig.getoption("--seed")
    if s is None:
        s = int(os.environ.get("DEPZ_QA_SEED", time.time_ns() % (2**31)))
    print(f"\n[depz-qa] random seed = {s}  (re-run with --seed={s} to reproduce)")
    return s


@pytest.fixture
def rng(seed: int, request: pytest.FixtureRequest) -> random.Random:
    """Per-test RNG derived from the session seed + test name.

    Deriving from the test name keeps each test's jitter independent of
    execution order, so a single test can be re-run in isolation and still
    replay the exact sequence that failed.
    """
    return random.Random(f"{seed}:{request.node.nodeid}")


# ── inventory ────────────────────────────────────────────────────────────────


@pytest.fixture(scope="session")
def hw_inventory(pytestconfig: pytest.Config) -> list[HwDevice]:
    enable_faulthandler(float(os.environ.get("DEPZ_QA_FAULTHANDLER_S", 900)))
    all_devices = discover_hw()
    devices = all_devices
    only_family = pytestconfig.getoption("--sensor")
    only_port = pytestconfig.getoption("--port")
    if only_family:
        devices = [d for d in devices if d.family == only_family]
    if only_port:
        devices = [d for d in devices if d.port == only_port]
    write_report(
        "hardware_inventory.json",
        {
            "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "toolchain": toolchain_info(),
            # This is a bench inventory, not a record of the current pytest
            # selection.  A targeted `--sensor` run must not erase the other
            # physically attached devices from the shared artifact.
            "devices": [d.__dict__ for d in all_devices],
        },
    )
    print(f"\n[depz-qa] hardware inventory: {len(devices)} device(s)")
    for d in devices:
        print(f"  {d.stable_id:26} {d.port:14} fw={d.fw_version:5} {d.software_name}")
    return devices


def _pick(inventory: list[HwDevice], family: str) -> HwDevice:
    matches = [d for d in inventory if d.family == family]
    if not matches:
        pytest.skip(f"no {family} device physically attached")
    return matches[0]


@pytest.fixture(scope="session")
def sr04_info(hw_inventory) -> HwDevice:
    return _pick(hw_inventory, FAMILY_SR04)


@pytest.fixture(scope="session")
def vl53l4_info(hw_inventory) -> HwDevice:
    return _pick(hw_inventory, FAMILY_VL53L4CD)


@pytest.fixture(scope="session")
def cx_info(hw_inventory) -> HwDevice:
    return _pick(hw_inventory, FAMILY_VL53L8CX)


@pytest.fixture(scope="session")
def ch_info(hw_inventory) -> HwDevice:
    return _pick(hw_inventory, FAMILY_VL53L8CH)


@pytest.fixture(scope="session")
def bno_info(hw_inventory) -> HwDevice:
    return _pick(hw_inventory, FAMILY_BNO086)


@pytest.fixture(params=ALL_FAMILIES)
def any_device_info(request: pytest.FixtureRequest, hw_inventory) -> HwDevice:
    """Parametrised over every family, skipping the ones not plugged in."""
    return _pick(hw_inventory, request.param)


@pytest.fixture(scope="session")
def any_info(hw_inventory) -> HwDevice:
    """The first attached device of any family — for tests that need *a* DEPZ
    unit but don't care which. Skips (not IndexErrors) on an empty bench."""
    if not hw_inventory:
        pytest.skip("no DEPZ device physically attached")
    return hw_inventory[0]


# ── leases + leak assertions ─────────────────────────────────────────────────


@pytest.fixture
def lease():
    """Factory for exclusive port leases inside a test."""
    return port_lease


@pytest.fixture
def leased_device(request: pytest.FixtureRequest):
    """Open one device under an exclusive lease and guarantee cleanup.

    Usage::

        dev = leased_device(sr04_info)

    The device is closed and its reader thread verified dead at teardown even
    when the test body raises — a leaked reader would otherwise poison every
    later test in the session.
    """
    import contextlib

    stack = contextlib.ExitStack()
    opened: list = []

    def _open(info: HwDevice, **kw):
        stack.enter_context(port_lease(info.resolve_port()))
        dev = open_family(info, **kw)
        opened.append(dev)
        return dev

    yield _open

    for dev in opened:
        with contextlib.suppress(Exception):
            dev.close()
    stack.close()
    leaked = wait_for_threads_to_die(timeout=5.0)
    assert not leaked, f"reader threads still alive after close(): {leaked}"


@pytest.fixture
def resource_baseline():
    """Snapshot resources around a test and fail on an obvious leak.

    Thresholds are intentionally loose: this guards against *unbounded* growth
    (a handle per cycle), not against normal allocator noise. Precise
    per-cycle trend checks live in the reconnect suite, which has the sample
    count to make them meaningful.
    """
    before = sample_resources()
    yield before
    after = sample_resources()
    assert after.tty_fd_count <= before.tty_fd_count, (
        f"leaked serial fd(s): {before.tty_fd_count} -> {after.tty_fd_count}"
    )
    assert after.depz_reader_threads <= before.depz_reader_threads, (
        f"leaked reader thread(s): {before.depz_reader_threads} -> {after.depz_reader_threads}"
    )
