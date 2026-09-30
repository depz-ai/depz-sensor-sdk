# VL53L5CX — user guide

`Vl53l5cx` is the [VL53L7CX](../vl53l7cx/guide.md) class with a different
expected module type; everything in the VL53L7CX guide — board commands, pin
control, the L5/L7 limits — and in the [VL53L8CX guide](../vl53l8cx/guide.md)
— configuration, frames, streaming, advanced features — applies unchanged.

## Open and range

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l7 import MODULE_TYPE_MZ, RESOLUTION_8X8, Vl53l5cx

with open_device("/dev/ttyACM0") as dev:   # returns a Vl53l5cx for the L5CX board
    assert isinstance(dev, Vl53l5cx)
    dev.init()                             # same L5/L7 blob as the VL53L7CX
    assert dev.module_type == MODULE_TYPE_MZ
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(10)
    dev.start_ranging()
    frame = dev.get_frame(timeout=2.0)
    print(frame.grid())
    dev.stop_ranging()
```

## Gotchas

- **The class is chosen from the board's USB id / device name.** A board
  stamped as an L7 but carrying an L5 (or the reverse) still ranges — the blob
  is shared — and `init()` warns about the module-type mismatch.
- **No deep sleep, no threshold auto-stop** (ULD 2.0.1), as on the VL53L7CX.
- **Peel the protective film off the lens** before measuring — with the film
  on, weak-signal zones read 0.
