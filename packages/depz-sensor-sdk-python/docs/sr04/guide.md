# SR04 — user guide

Hands-on guide to the `Sr04` device class. For what the sensor is and its
concepts, read the [introduction](introduction.md); for exact signatures see
the [API reference](../api.md).

## Contents

- [Open the device](#open-the-device)
- [Hello-world: live distance](#hello-world-live-distance)
- [Configuration](#configuration)
- [Single shot vs the loop](#single-shot-vs-the-loop)
- [Streaming: callbacks vs iterators](#streaming-callbacks-vs-iterators)
- [Temperature-compensated distance](#temperature-compensated-distance)
- [Gotchas](#gotchas)

## Open the device

`open_device()` probes the port and returns the right class:

```python
from depz_sensor_sdk import open_device, Sr04

dev = open_device("/dev/ttyACM0")   # returns an Sr04
assert isinstance(dev, Sr04)
```

`open_device()` with no argument picks the DEPZ port with the smallest USB
serial; pass an index or `serial=` to disambiguate two of the same model. See
the [common guide](../guide.md#discovery) for discovery details.

## Hello-world: live distance

```python
from depz_sensor_sdk import open_device, Sr04

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Sr04)
    dev.set_sample_period_us(50_000)     # 20 Hz
    dev.start()                          # start the measurement loop
    for m in dev.stream():               # blocks; Ctrl+C to stop
        if m.valid:
            print(f"{m.distance_mm:7.1f} mm  ({m.source})")
        else:
            print("no echo")
```

## Configuration

All config is a plain get/set pair; values are microseconds.

```python
dev.get_sample_period_us()        # → 50000 (stored, not the effective rate)
dev.set_sample_period_us(20_000)  # 50 Hz ceiling

# echo decay is clamped to 4000–65000 µs on the device; set_* re-reads and
# returns the value actually in effect
eff = dev.set_echo_decay_us(3_000)   # → 4000 (clamped up to the minimum)
dev.get_echo_decay_us()              # → 4000
```

`set_sample_period_us` returns nothing; a read-back gives the stored value. The
device throttles the **effective** rate by the echo window, so at long range
you will see fewer samples per second than the configured ceiling.

## Single shot vs the loop

```python
# one-shot: blocks until the echo completes (or times out at ~65.5 ms)
m = dev.measure_once(timeout=1.0)
print(m.echo_time_us, m.distance_mm, m.source)   # source == "once"

# free-running loop: samples arrive on the stream / callbacks
dev.start()      # idempotent
# ... consume dev.stream() ...
dev.stop()       # idempotent
```

`measure_once()` raises `BusyError` while the loop is running — stop the loop
first, or just consume loop samples. Both paths deliver `Sr04Measurement`.

## Streaming: callbacks vs iterators

```python
# callback — fires on the reader thread; keep it quick, hand off heavy work
unsub = dev.on_measurement(lambda m: q.put(m))
unsub()                                   # stop

# iterator — bounded, drop-oldest; subscribes immediately
it = dev.stream(maxsize=256)
m = next(it)
print("dropped so far:", it.dropped_count)
print("per-stream drops:", dev.stream_dropped_counts)
```

Every subscriber (callback or iterator) receives **every** measurement — loop
samples *and* unsolicited SYNC_IN single shots. Bounded iterators drop the
oldest item when full and count the drops.

## Temperature-compensated distance

The default distance assumes 343 m/s. For better accuracy across temperature,
pass the air temperature (°C):

```python
m = dev.measure_once()
m.distance_mm            # at 343 m/s
m.distance_mm_at(30.0)   # c = 331.3 + 0.606·T ≈ 349.5 m/s at 30 °C
```

Both return `None` for a no-echo timeout.

## Gotchas

- **Always check `m.valid`.** A no-echo timeout sets `echo_time_us == 0xFFFF`,
  `valid == False`, `distance_mm == None`.
- **`measure_once()` during the loop raises `BusyError`.** One in-flight
  measurement path at a time.
- **Configured period is a ceiling, not the realised rate** — the echo window
  throttles it; read-back returns the stored value.
- **`set_echo_decay_us` clamps to 4000–65000 µs** and returns the effective
  value. Values above 65535 can't be sent (the field is a u16).
- **Callbacks run on the reader thread** — don't block; don't call blocking
  device methods from inside one.
