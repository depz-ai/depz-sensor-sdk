"""Family-generic lifecycle adapter.

The four sensors expose deliberately different APIs — SR04 has
``start()``/``stop()``, VL53L8 has ``init()``/``start_ranging()``, BNO086 has
``enable()``/``disable()``. The *lifecycle contract* they must all honour is
the same, though, so this module maps each onto one small interface:

    prepare() -> start() -> read_one() -> stop() -> close()

That lets one lifecycle/reconnect test body run against every family instead of
four near-identical copies drifting apart. Anything genuinely sensor-specific
(CNH config, resolutions, quaternion norms) stays in the per-sensor test files
where it belongs.
"""

from __future__ import annotations

import time
from abc import ABC, abstractmethod
from typing import Any

from _support import (
    FAMILY_BNO086,
    FAMILY_SR04,
    FAMILY_VL53L8CH,
    FAMILY_VL53L8CX,
    SR04_MODULE_NO_ECHO_US,
    HwDevice,
    open_family,
)


class SensorDriver(ABC):
    """Uniform lifecycle facade over one open sensor object."""

    #: Wall-clock budget for the first sample after start(), per family.
    first_sample_timeout = 2.0
    #: Nominal streaming rate used to size "did it really stream?" waits.
    nominal_hz = 20.0

    def __init__(self, dev: Any, info: HwDevice):
        self.dev = dev
        self.info = info

    #: Request timeout to open this family with. The contract default is 200 ms
    #: (contract 07); a family overrides this only where the *device* has been
    #: measured unable to meet it — see `Bno086Driver`.
    open_timeout = 0.2

    @classmethod
    def open(cls, info: HwDevice, **kw) -> "SensorDriver":
        driver_cls = DRIVERS[info.family]
        kw.setdefault("timeout", driver_cls.open_timeout)
        return driver_cls(open_family(info, **kw), info)

    @property
    def family(self) -> str:
        return self.info.family

    def prepare(self) -> None:
        """Post-open, pre-start setup (VL53L8 firmware download)."""

    @abstractmethod
    def start(self) -> None: ...

    @abstractmethod
    def stop(self) -> None: ...

    @abstractmethod
    def read_one(self, timeout: float | None = None) -> Any:
        """Return the next sample or raise on timeout. Never blocks forever."""

    @abstractmethod
    def pending_read(self) -> Any:
        """Issue the family's canonical *long-blocking* SDK read.

        Used to assert that `close()` releases an in-flight reader. It must go
        through the SDK's real public path (`get_frame`, `StreamIterator`) —
        reaching into a private queue here would test this file instead of the
        SDK.
        """

    def drain_stragglers(self, quiet_for: float = 0.4, limit: float = 5.0) -> int:
        """Read until the stream goes quiet; return how many samples arrived.

        After `stop()` a few in-flight samples still land. A close-release test
        must drain them first, or a straggler — not `close()` — is what wakes
        the pending reader, and the test passes for the wrong reason.
        """
        end = time.monotonic() + limit
        n = 0
        while time.monotonic() < end:
            try:
                self.read_one(timeout=quiet_for)
                n += 1
            except Exception:
                return n
        return n

    @abstractmethod
    def sample_timestamp_us(self, sample: Any) -> int: ...

    @abstractmethod
    def sample_is_valid(self, sample: Any) -> bool:
        """Structural validity — decodable and in range. NOT physical accuracy."""

    def close(self) -> None:
        self.dev.close()

    @property
    def closed(self) -> bool:
        return self.dev.closed

    def drain(self, seconds: float) -> int:
        """Read as many samples as arrive in ``seconds``; return the count."""
        end = time.monotonic() + seconds
        n = 0
        while time.monotonic() < end:
            try:
                self.read_one(timeout=max(0.05, end - time.monotonic()))
                n += 1
            except Exception:
                break
        return n


class Sr04Driver(SensorDriver):
    first_sample_timeout = 1.5
    nominal_hz = 20.0

    def __init__(self, dev, info):
        super().__init__(dev, info)
        self._stream = None

    def start(self) -> None:
        # Subscribe *before* the device starts producing: StreamIterator
        # registers eagerly, so this cannot miss the first sample.
        if self._stream is None:
            self._stream = self.dev.stream()
        self.dev.start()

    def stop(self) -> None:
        # Stop the device but keep the subscription alive: reads after stop()
        # must time out, not raise "no stream". The iterator is released in
        # close().
        self.dev.stop()

    def close(self) -> None:
        if self._stream is not None:
            self._stream.close()
            self._stream = None
        self.dev.close()

    def read_one(self, timeout: float | None = None) -> Any:
        if self._stream is None:
            raise RuntimeError("read_one() before start()")
        t = self.first_sample_timeout if timeout is None else timeout
        try:
            return self._stream._queue.get(timeout=t)
        except Exception:
            raise TimeoutError(f"SR04: no measurement within {t}s") from None

    def pending_read(self) -> Any:
        if self._stream is None:
            raise RuntimeError("pending_read() before start()")
        return next(self._stream)  # blocks until data or close (StopIteration)

    def sample_timestamp_us(self, sample) -> int:
        return sample.timestamp_us

    def sample_is_valid(self, sample) -> bool:
        # A no-echo timeout is a legitimate reading (nothing in range), not a
        # protocol fault — only the decode has to be sane. That covers both
        # the firmware's 0xFFFF sentinel and the module-level ~58.3 ms pulse
        # (SR04_MODULE_NO_ECHO_US), which the firmware reports as data.
        if not sample.valid or sample.echo_time_us >= SR04_MODULE_NO_ECHO_US:
            return True
        d = sample.distance_mm
        return d is not None and 0 < d


class Vl53l8Driver(SensorDriver):
    first_sample_timeout = 3.0
    nominal_hz = 10.0

    def prepare(self) -> None:
        self.dev.init()

    def start(self) -> None:
        self.dev.start_ranging()

    def stop(self) -> None:
        self.dev.stop_ranging()

    def read_one(self, timeout: float | None = None) -> Any:
        return self.dev.get_frame(timeout=self.first_sample_timeout if timeout is None else timeout)

    def pending_read(self) -> Any:
        return self.dev.get_frame(timeout=30.0)

    def sample_timestamp_us(self, sample) -> int:
        return sample.timestamp_us

    def sample_is_valid(self, sample) -> bool:
        return (
            sample.distance_mm.shape == (sample.resolution,)
            and sample.target_status.shape == (sample.resolution,)
        )


class Bno086Driver(SensorDriver):
    first_sample_timeout = 2.0
    nominal_hz = 50.0

    #: The BNO086 takes a flat ~119 ms to service its FIRST command after a CDC
    #: reopen (ERRATA E12; every later command is ~0.5 ms, and no other sensor
    #: does this). The contract's 200 ms default leaves only ~80 ms of margin,
    #: so a reconnect-heavy test using it fails intermittently on a device that
    #: is working correctly. 500 ms is ~4x the honest first-ack cost. Raising
    #: it further buys nothing: the extended campaign proved the only thing
    #: beyond ~0.41 s is the F-24 dropped ack, which never arrives at any
    #: timeout (2 s failed identically; byte trace showed the command executed
    #: and the ack absent) — that case is handled by the bounded retry in
    #: start(), not by waiting longer.
    open_timeout = 0.5

    #: Rotation vector is the report every BNO086 board supports and the one
    #: the viewer actually renders, so it is the lifecycle canary.
    SENSOR = 0x05  # SensorId.ROTATION_VECTOR

    def __init__(self, dev, info):
        super().__init__(dev, info)
        self._reports = None
        #: F-24 dropped-ack retries, surfaced per cycle in the JSON report.
        self.retried_enables = 0

    def start(self) -> None:
        if self._reports is None:
            self._reports = self.dev.reports(sensors=self.SENSOR)
        from depz_sensor_sdk.errors import DepzTimeoutError

        try:
            self.dev.enable(self.SENSOR, hz=self.nominal_hz)
        except DepzTimeoutError:
            # F-24: ~1/300 reopens the device EXECUTES the first Set Feature
            # but its RPT_STATUS ack never reaches the host (byte trace:
            # streaming resumes at the requested rate, no 0x80 ack on the
            # wire). Set Feature is idempotent, so one retry distinguishes a
            # dropped ack from a dead device — which still fails right here.
            self.retried_enables += 1
            self.dev.enable(self.SENSOR, hz=self.nominal_hz)

    def stop(self) -> None:
        # Keep the subscription: reads after stop() must time out rather than
        # raise "no stream". Released in close().
        self.dev.disable(self.SENSOR)

    def close(self) -> None:
        if self._reports is not None:
            self._reports.close()
            self._reports = None
        self.dev.close()

    def read_one(self, timeout: float | None = None) -> Any:
        if self._reports is None:
            raise RuntimeError("read_one() before start()")
        t = self.first_sample_timeout if timeout is None else timeout
        try:
            return self._reports._queue.get(timeout=t)
        except Exception:
            raise TimeoutError(f"BNO086: no report within {t}s") from None

    def pending_read(self) -> Any:
        if self._reports is None:
            raise RuntimeError("pending_read() before start()")
        return next(self._reports)  # blocks until data or close (StopIteration)

    def sample_timestamp_us(self, sample) -> int:
        return sample.timestamp_us

    def sample_is_valid(self, sample) -> bool:
        import math

        norm = math.sqrt(sample.i**2 + sample.j**2 + sample.k**2 + sample.real**2)
        finite = all(
            math.isfinite(v) for v in (sample.i, sample.j, sample.k, sample.real)
        )
        # Q14 quantisation alone allows ~1e-4 of slack; 2% is generous but
        # still catches a genuinely broken/garbage quaternion.
        return finite and abs(norm - 1.0) < 0.02


DRIVERS: dict[str, type[SensorDriver]] = {
    FAMILY_SR04: Sr04Driver,
    FAMILY_VL53L8CX: Vl53l8Driver,
    FAMILY_VL53L8CH: Vl53l8Driver,
    FAMILY_BNO086: Bno086Driver,
}
