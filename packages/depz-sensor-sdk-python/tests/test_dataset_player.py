"""DatasetPlayer (contract 09 §Player): pause / resume, seek, position, speed,
events — deterministic, on a fake clock (``threaded=False`` + ``tick``), plus a
few real-thread checks of the threaded driver."""

from __future__ import annotations

import math
import threading
from pathlib import Path

import pytest

from depz_sensor_sdk.dataset import (
    DATASET_SCHEMA,
    SCHEMA,
    DatasetPlayer,
    DatasetReader,
    DatasetWriter,
    PlayerState,
)
from depz_sensor_sdk.errors import InvalidStateError

REPO = Path(__file__).resolve().parents[3]
DUAL_SR04 = REPO / "contracts/vectors/recordings/dataset_dual_sr04.depzdata"

EPS = 1e-6  # seconds; a hair past / before a due time


class FakeClock:
    def __init__(self, t: float = 100.0):
        self.t = t

    def __call__(self) -> float:
        return self.t

    def advance(self, dt: float) -> None:
        self.t += dt


def _write(path: Path, events: list[tuple[str, int]]) -> Path:
    w = DatasetWriter(path, {
        "d0": {"sensor_type": "sr04", "time_sync": {"offset_us": 100, "rtt_us": 5}},
        "d1": {"sensor_type": "vl53l8", "time_sync": {"offset_us": -5000, "rtt_us": 7}},
    }, note="test")
    for i, (d, t) in enumerate(events):
        w.write(d, t, "sr04", {"n": i})
    w.close()
    return path


@pytest.fixture
def sample(tmp_path: Path) -> Path:
    """The TS test's dataset: interleaved out of order across devices."""
    return _write(tmp_path / "s.depzdata",
                  [("d0", 1000), ("d1", 900), ("d0", 51000), ("d1", 30900)])


def _player(path: Path, **kw):
    clock = FakeClock()
    p = DatasetPlayer(DatasetReader(path), clock=clock, threaded=False, **kw)
    seen: list[int] = []
    states: list[str] = []
    p.on_record(lambda r: seen.append(r.t_host_us))
    p.on_state(lambda s: states.append(s.value))
    return p, clock, seen, states


# ── reader additions ─────────────────────────────────────────────────────────


def test_schema_constant_and_reader_records(sample: Path):
    assert DATASET_SCHEMA == SCHEMA == "depz.dataset/2"
    r = DatasetReader(sample)
    assert r.header["schema"] == DATASET_SCHEMA
    assert [x.t_host_us for x in r.records] == [900, 1000, 30900, 51000]
    assert r.records is r.records  # cached
    assert r.duration_us == 50100


def test_duration_empty_and_single(tmp_path: Path):
    assert DatasetReader(_write(tmp_path / "e.depzdata", [])).duration_us == 0
    assert DatasetReader(_write(tmp_path / "o.depzdata", [("d0", 5)])).duration_us == 0


# ── the TS scenario ──────────────────────────────────────────────────────────


def test_plays_paced_pauses_resumes_seeks(sample: Path):
    """Mirrors the TS `dataset player` test, on the reference timing model."""
    p, clock, seen, states = _player(sample)
    assert p.state is PlayerState.IDLE and p.position_us == 0
    assert p.duration_us == 50100
    p.play()
    assert seen == []  # delivery happens in tick (or on the player thread)
    wait = p.tick()
    assert seen == [900]  # the first record goes out at once
    assert wait == pytest.approx(100e-6)  # 1000 is 100 µs later
    clock.advance(100e-6 + EPS)
    p.tick()
    assert seen == [900, 1000]
    clock.advance(0.0299 - 2 * EPS)
    p.tick()
    assert seen == [900, 1000]
    clock.advance(2 * EPS)
    p.tick()
    assert seen == [900, 1000, 30900]
    assert p.position_us == 30000

    p.pause()
    clock.advance(1.0)
    assert p.tick() is None
    assert len(seen) == 3

    p.play()  # resume: the next record waits its full gap (20.1 ms) again
    clock.advance(0.0201 - EPS)
    p.tick()
    assert len(seen) == 3
    clock.advance(2 * EPS)
    assert p.tick() is None
    assert seen == [900, 1000, 30900, 51000]
    assert p.state is PlayerState.DONE
    assert p.position_us == 50100
    assert states == ["playing", "paused", "playing", "done"]

    seen.clear()
    p.seek_us(30000)  # → 30900
    assert p.state is PlayerState.DONE  # seek never changes state (unless …)
    assert p.position_us == 100  # last record before the target
    p.speed = 0
    p.play()
    p.tick()
    assert seen == [30900, 51000]
    assert p.state is PlayerState.DONE


# ── timing model ─────────────────────────────────────────────────────────────


def _evenly(tmp_path: Path, n: int = 5, gap_us: int = 100_000) -> Path:
    return _write(tmp_path / "even.depzdata",
                  [("d0", 1_000_000 + i * gap_us) for i in range(n)])


def test_speed_scales_gaps(tmp_path: Path):
    p, clock, seen, _ = _player(_evenly(tmp_path), speed=2.0)
    p.play()
    assert p.tick() == pytest.approx(0.05)
    clock.advance(0.05)
    assert p.tick() == pytest.approx(0.05)
    assert len(seen) == 2


def test_speed_change_mid_wait_rescales_the_remainder(tmp_path: Path):
    p, clock, seen, _ = _player(_evenly(tmp_path))
    p.play()
    p.tick()
    clock.advance(0.05)  # half of the 100 ms gap played at 1×
    p.tick()
    assert len(seen) == 1
    p.speed = 0.5  # remaining 50 ms of media → 100 ms of wall time
    assert p.tick() == pytest.approx(0.1)
    clock.advance(0.1 - EPS)
    p.tick()
    assert len(seen) == 1
    clock.advance(2 * EPS)
    p.tick()
    assert len(seen) == 2
    p.speed = 4.0  # next gaps are 25 ms
    assert p.tick() == pytest.approx(0.025, abs=1e-5)


def test_speed_zero_delivers_everything_at_once_then_speed_back(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path), speed=0)
    p.play()
    assert p.tick() is None
    assert len(seen) == 5 and states == ["playing", "done"]

    # from 0 back to paced mid-play: the next record waits its gap from the
    # last delivered one
    p2, clock2, seen2, _ = _player(_evenly(tmp_path))
    p2.play()
    p2.tick()
    p2.speed = 0
    p2.speed = 1.0
    assert p2.tick() == pytest.approx(0.1)


def test_slow_consumer_catches_up_without_drift(tmp_path: Path):
    p, clock, seen, _ = _player(_evenly(tmp_path))
    p.play()
    p.tick()
    clock.advance(0.35)  # consumer was busy: records 1..3 are overdue
    w = p.tick()
    assert len(seen) == 4
    assert w == pytest.approx(0.05)  # record 4 stays on its original schedule


def test_speed_validation(sample: Path):
    p, *_ = _player(sample)
    for bad in (-1, math.nan, math.inf, "2"):
        with pytest.raises(ValueError):
            p.speed = bad  # type: ignore[assignment]
    with pytest.raises(ValueError):
        DatasetPlayer(DatasetReader(sample), speed=-0.5, threaded=False)
    assert p.speed == 1.0


# ── seek ─────────────────────────────────────────────────────────────────────


def test_seek_while_paused_skips_and_waits_gap(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path))
    p.play()
    p.tick()
    p.pause()
    p.seek_us(250_000)  # between records 2 (200 ms) and 3 (300 ms)
    assert p.position_us == 200_000
    assert p.state is PlayerState.PAUSED
    p.play()
    assert p.tick() == pytest.approx(0.1)  # gap from record 2 to record 3
    clock.advance(0.1)
    p.tick()
    assert seen == [1_000_000, 1_300_000]  # 1 and 2 were skipped
    assert states == ["playing", "paused", "playing"]


def test_seek_while_playing_continues_without_state_events(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path))
    p.play()
    p.tick()
    clock.advance(0.03)
    p.seek_us(300_000)  # exactly a record: it is the next one delivered
    assert p.state is PlayerState.PLAYING
    assert p.tick() == pytest.approx(0.1)
    clock.advance(0.1)
    p.tick()
    assert seen == [1_000_000, 1_300_000]
    assert states == ["playing"]


def test_seek_backwards_and_negative(tmp_path: Path):
    p, clock, seen, _ = _player(_evenly(tmp_path), speed=0)
    p.play()
    p.tick()
    p.seek_us(-5)
    assert p.position_us == 0
    p.play()
    p.tick()
    assert len(seen) == 10


def test_seek_past_end(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path))
    p.play()
    p.tick()
    p.seek_us(10**9)  # while playing → done
    assert p.state is PlayerState.DONE
    assert p.position_us == 400_000
    assert states == ["playing", "done"]
    assert p.tick() is None and seen == [1_000_000]

    p2, clock2, seen2, _ = _player(_evenly(tmp_path), speed=0)
    p2.seek_us(10**9)  # while idle: play() restarts from the top
    assert p2.state is PlayerState.IDLE
    p2.play()
    p2.tick()
    assert len(seen2) == 5


def test_seek_errors_and_empty(tmp_path: Path):
    p, *_ = _player(_evenly(tmp_path))
    for bad in (math.nan, "5", None):
        with pytest.raises(ValueError):
            p.seek_us(bad)  # type: ignore[arg-type]
    e, _, seen, states = _player(_write(tmp_path / "e.depzdata", []))
    e.seek_us(100)  # no-op
    assert e.position_us == 0 and e.duration_us == 0


# ── states, events, edges ────────────────────────────────────────────────────


def test_empty_dataset_goes_straight_to_done(tmp_path: Path):
    p, clock, seen, states = _player(_write(tmp_path / "e.depzdata", []))
    p.play()
    assert states == ["playing", "done"]
    assert p.tick() is None and seen == []


def test_play_and_pause_are_idempotent(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path))
    p.pause()  # idle: no-op
    assert p.state is PlayerState.IDLE
    p.play()
    p.play()
    p.pause()
    p.pause()
    assert states == ["playing", "paused"]


def test_done_then_play_restarts(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path, n=2), speed=0)
    p.play()
    p.tick()
    p.play()
    p.tick()
    assert seen == [1_000_000, 1_100_000] * 2
    assert states == ["playing", "done", "playing", "done"]


def test_unsubscribe_is_idempotent(tmp_path: Path):
    p = DatasetPlayer(_evenly(tmp_path), clock=FakeClock(), threaded=False, speed=0)
    got: list[int] = []
    unsub = p.on_record(lambda r: got.append(1))
    unsub_s = p.on_state(lambda s: got.append(2))
    unsub()
    unsub()
    unsub_s()
    unsub_s()
    p.play()
    p.tick()
    assert got == []


def test_callback_may_control_the_player(tmp_path: Path):
    """Callbacks run under the player's (reentrant) lock and may call it."""
    p, clock, seen, states = _player(_evenly(tmp_path), speed=0)
    p.on_record(lambda r: p.pause() if r.t_host_us == 1_100_000 else None)
    p.play()
    assert p.tick() is None
    assert seen == [1_000_000, 1_100_000]
    assert states == ["playing", "paused"]


def test_record_callback_error_pauses_and_propagates(tmp_path: Path):
    p, clock, seen, states = _player(_evenly(tmp_path), speed=0)

    def boom(r):
        if r.t_host_us == 1_200_000:
            raise RuntimeError("consumer failed")

    p.on_record(boom)
    p.play()
    with pytest.raises(RuntimeError, match="consumer failed"):
        p.tick()
    assert p.state is PlayerState.PAUSED
    assert p.position_us == 200_000  # the failing record counts as delivered
    p.play()
    p.tick()
    assert seen == [1_000_000 + i * 100_000 for i in range(5)]
    assert states == ["playing", "paused", "playing", "done"]


def test_close_and_path_constructor(tmp_path: Path):
    path = _evenly(tmp_path)
    with DatasetPlayer(path, clock=FakeClock(), threaded=False) as p:
        assert p.duration_us == 400_000
        p.play()
        with pytest.raises(InvalidStateError):
            p.wait(0)  # no player thread
    assert p.state is PlayerState.PAUSED
    p.close()  # idempotent
    with pytest.raises(InvalidStateError):
        p.play()
    with pytest.raises(InvalidStateError):
        p.seek_us(0)


def test_cross_language_fixture_plays_in_reader_order():
    reader = DatasetReader(DUAL_SR04)
    p = DatasetPlayer(reader, clock=FakeClock(), threaded=False, speed=0)
    got = []
    p.on_record(got.append)
    p.play()
    p.tick()
    assert got == list(reader) and len(got) == 10
    assert p.position_us == p.duration_us == reader.duration_us > 0


# ── threaded driver (real time, loose bounds) ────────────────────────────────


def test_threaded_delivers_on_player_thread(tmp_path: Path):
    with DatasetPlayer(_evenly(tmp_path, n=4, gap_us=20_000), speed=1.0) as p:
        threads: set[str] = set()
        seen: list[int] = []

        def on_rec(r):
            threads.add(threading.current_thread().name)
            seen.append(r.t_host_us)

        p.on_record(on_rec)
        p.play()
        assert p.wait(5.0)
        assert p.state is PlayerState.DONE
        assert len(seen) == 4
        assert threads == {"depz-dataset-player"}
        # replay after done reuses the thread
        seen.clear()
        p.speed = 0
        p.play()
        assert p.wait(5.0) and len(seen) == 4


def test_threaded_pause_from_another_thread_stops_delivery(tmp_path: Path):
    with DatasetPlayer(_evenly(tmp_path, n=50, gap_us=100_000)) as p:
        first = threading.Event()
        seen: list[int] = []
        p.on_record(lambda r: (seen.append(r.t_host_us), first.set()))
        p.play()
        assert first.wait(5.0)
        p.pause()
        n = len(seen)
        assert p.wait(0)  # not playing
        assert p.state is PlayerState.PAUSED
        p.seek_us(4_000_000)  # paused: nothing delivered by seeking
        assert len(seen) == n
        p.speed = 0
        p.play()
        assert p.wait(5.0)
        assert seen[n:] == [1_000_000 + i * 100_000 for i in range(40, 50)]


def test_threaded_callback_error_goes_to_excepthook(tmp_path: Path, monkeypatch):
    caught: list[BaseException] = []
    monkeypatch.setattr(threading, "excepthook", lambda a: caught.append(a.exc_value))
    with DatasetPlayer(_evenly(tmp_path), speed=0) as p:
        def boom(r):
            raise KeyError("x")

        p.on_record(boom)
        p.play()
        assert p.wait(5.0)
        assert p.state is PlayerState.PAUSED
        assert len(caught) == 1 and isinstance(caught[0], KeyError)
