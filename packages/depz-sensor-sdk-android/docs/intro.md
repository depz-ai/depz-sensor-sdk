# Android SDK

`depz-sensor-sdk-android` brings the DEPZ USB sensor boards to **Android 12 and
newer** (API 31+), over the phone's or the board computer's USB host port. It is
a Kotlin library on top of the [Java SDK](../../depz-sensor-sdk-java/docs/intro.md):
the protocol, the drivers and the decoding are the Java SDK's — and so are the
measurement types — and this package adds what Android needs: the USB transport
(CDC-ACM over `UsbManager`), discovery, the USB permission, and an API for
Kotlin (`suspend` functions, `Flow`) and for Java (callbacks, listeners). No
JNI, no native code, no root.

## Install

Maven Central, `io.github.depz-ai:depz-sensor-sdk-android`
([Install](install.md) has the requirements). In a standard Android Studio / Gradle project
(Android Gradle Plugin 8.1 or newer, `mavenCentral()` among the repositories):

```kotlin
// build.gradle.kts of the app module
android {
    defaultConfig { minSdk = 31 }   // Android 12, or newer
}

dependencies {
    implementation("io.github.depz-ai:depz-sensor-sdk-android:1.0.0")
}
```

The Java SDK (`io.github.depz-ai:depz-sensor-sdk`, package `ai.depz.sensor.*`)
and `kotlinx-coroutines-core` come with it as its dependencies; the desktop
serial library of the Java SDK (jSerialComm) does not. No other setup: no
core-library desugaring, no R8 rules, no native code. [Install](install.md) has
the requirements, what the artifact brings into an app, and the build from the
sources until it is published.

## Quick start

Kotlin — the first attached board, opened and streaming:

```kotlin
suspend fun showDistance(context: Context) {
    val board = DepzAndroid.discover(context).firstOrNull() ?: return   // no DEPZ board attached
    try {
        DepzAndroid.open(context, board).use { sensor ->   // asks for the USB permission when needed
            if (sensor is Sr04Sensor) {
                sensor.start()
                sensor.measurements.collect { m -> Log.i("DEPZ", "${m.distanceMm()} mm") }
            }
        }   // collect returns when the sensor is closed
    } catch (e: UsbPermissionDeniedException) {
        Log.w("DEPZ", "the user refused the USB permission", e)
    } catch (e: DepzException) {
        Log.w("DEPZ", "the board failed or was unplugged", e)   // unplugged: DeviceLostException
    }
}
```

Java — no coroutines: a callback for the opened sensor, a listener for its
stream, both called on the main thread:

```java
ExecutorService background = Executors.newSingleThreadExecutor();

DepzUsbDevice board = DepzAndroid.discover(context).get(0);
DepzAndroid.open(context, board, new DepzCallback<DepzSensor>() {   // asks for the USB permission when needed
    @Override public void onResult(DepzSensor sensor) {             // main thread
        if (sensor instanceof Sr04Sensor sr04) {
            Cancellable subscription = sr04.measurements(new StreamListener<Sr04Measurement>() {
                @Override public void onItem(Sr04Measurement m) { show(m.distanceMm()); }    // main thread
                @Override public void onError(DeviceLostException e) { showUnplugged(); }   // the board is lost
                @Override public void onEnd() { }                                           // the sensor was closed
            });
            background.execute(() -> sr04.getDevice().start());     // a Java SDK call: it blocks
            // later: subscription.cancel(); sensor.close();
        }
    }

    @Override public void onError(Exception error) {                // UsbPermissionDeniedException, DepzException…
        showError(error);
    }
});
```

## Permissions

No `<uses-permission>` and no manifest entry of the app's: USB host access is
granted per device, by the user, in a system dialog. `DepzAndroid.open` shows it
when the app does not hold the permission yet; `requestPermission` asks ahead
of time:

```kotlin
val board = DepzAndroid.discover(context).first()
if (!DepzAndroid.hasPermission(context, board)) {
    val granted = DepzAndroid.requestPermission(context, board)   // the system's dialog
    if (!granted) return                                         // the user refused
}
val serial = DepzAndroid.device(context, board.usbDevice).serial   // readable from now on
```

A refusal is `false` here and a `UsbPermissionDeniedException` from `open`.
A board unplugged while its dialog is open ends the wait: `false` here, a
`DeviceLostException` ("…: device detached") from `open`.
The permission is for the attached device: a board plugged in again is asked
for again. Android hides the board's USB serial number until it is granted. The
SDK's manifest declares `<uses-feature android:name="android.hardware.usb.host"
android:required="true">`, merged into the app's: Google Play offers the app
only to devices with a USB host port.

## Supported boards

Every board of the DEPZ USB sensor line the Java SDK has a device class for —
`DepzAndroid.open` returns the class of the board's family:

| Board | Class | Stream |
|---|---|---|
| HC-SR04 (ultrasonic) | `Sr04Sensor` | `measurements`: `Sr04Measurement` |
| VL53L4CD (single-zone ToF) | `Vl53l4cdSensor` | `measurements`: `Vl53l4Measurement` |
| VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CX (1D ToF family) | `Vl53lxSensor` | `measurements`: `Vl53lxMeasurement` |
| VL53L5CX, VL53L7CX, VL53L7CH, VL53L8CX, VL53L8CH (8×8 ToF) | `Vl53l8Sensor` | `frames`: `Vl53l8Frame` |
| TMF8829 (multizone dToF) | `Tmf8829Sensor` | `measurements`: `MeasurementBuilder.Measurement` |
| BNO055 (9-axis IMU) | `Bno055Sensor` | `samples`: `Bno055Sample` |
| BNO085, BNO086 (9-axis IMU) | `Bno086Sensor` | `reports`: `Reports.Report` |

Any other DEPZ board opens as a `GenericSensor` (its `device` is the Java SDK's
`DepzDevice`). The measurement types, the configuration and the sensors'
behaviour are the Java SDK's: its [per-sensor pages](../../depz-sensor-sdk-java/docs/intro.md)
apply as they are.

## Where to next

- **[Install](install.md)** — requirements, the dependency, what comes with
  it, building it from the sources.
- **[Examples](examples.md)** — the sample app: build it, run it from `adb`,
  where in its sources each thing an app does with a board is; the short
  snippets by task.
- **[Guide](guide.md)** — discovery, opening, each family's setup, threads and
  cancellation, an unplugged board and hot-plug, the Java API, recording, R8;
  and [in the field](guide.md#in-the-field): an OTG adapter and a powered hub,
  Google Play Protect, the permission dialog as the user meets it, a board
  that is not listed.
- **API reference** — the KDoc of `DepzAndroid`, `DepzSensor` and the classes
  around them, in the IDE and as the artifact's `-javadoc.jar` (Dokka HTML).
- **[Java SDK](../../depz-sensor-sdk-java/docs/intro.md)** — the device classes
  under every `DepzSensor` (`sensor.device`) and each sensor's own pages.
- **Source & license** — the SDKs are open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk),
  the Android package in its folder `packages/depz-sensor-sdk-android`.
