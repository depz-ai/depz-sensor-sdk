# DEPZ Sensor SDK — Android

The DEPZ USB sensor boards on Android 12+ (API 31), over USB host. Kotlin, on top
of the [Java SDK](../depz-sensor-sdk-java): the protocol, the drivers and the
decoding are the Java SDK's, and so are the measurement types; this package adds
the USB transport (`AndroidUsbLink`, CDC-ACM over `UsbManager`), discovery, the
USB permission and an API for Kotlin (coroutines, `Flow`) and for Java
(callbacks, listeners). No JNI, no root.

- `sdk/` — the library, package `ai.depz.sensor.android`.
- `sample/` — an app that lists the attached boards, connects them, shows
  their measurements live and records them into a file.
- [docs/](docs/intro.md) — the introduction, [install](docs/install.md), the
  [guide](docs/guide.md) and the [examples](docs/examples.md).

## Install

Maven Central, `io.github.depz-ai:depz-sensor-sdk-android` (or build the
artifact from this repository: [Build from the sources](#build-from-the-sources)).
In a standard Android Studio / Gradle project
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
core-library desugaring, no R8 rules, no native code.

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
behaviour are the Java SDK's: its [per-sensor pages](../depz-sensor-sdk-java/docs/intro.md)
apply as they are.

## Kotlin

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

Each family has its own setup and its own typed stream — a `when` over the
sensor classes is exhaustive:

```kotlin
val board = DepzAndroid.discover(context).first()
DepzAndroid.open(context, board).use { sensor ->   // asks for the USB permission when needed
    when (sensor) {
        is Vl53l8Sensor -> {
            sensor.init()
            sensor.configure(resolution = 64, rangingFrequencyHz = 15)
            sensor.start()
            sensor.frames.collect { frame -> /* … */ }   // ends when the sensor is closed or lost
        }
        else -> { /* Sr04Sensor, Vl53l4cdSensor, Vl53lxSensor, Bno055Sensor, Bno086Sensor, Tmf8829Sensor, GenericSensor */ }
    }
}
```

A board unplugged while it is open ends as a loss: its flows fail with the Java
SDK's `DeviceLostException` (a listener gets `onError`), whose cause names the
USB device, and its USB connection is released; the other boards go on. Nothing
reconnects by itself. `DepzAndroid.devices(context)` is the list of attached
boards as a `Flow` that gives a new list whenever a board is plugged in or
unplugged:

```kotlin
DepzAndroid.devices(context).collect { boards -> /* show them; open the new one */ }
```

Every call that waits is a `suspend` function that works off the main thread;
every stream is a cold `Flow`. The KDoc of `DepzAndroid` and `DepzSensor` has
the rules: threading, cancellation, how a stream ends, errors, cleanup.

## Java

No coroutines: `requestPermission` and `open` answer a `DepzCallback`, a stream
is subscribed with a `StreamListener`, and each of these calls returns a
`Cancellable`. The callbacks run on the `Executor` the call names — the main
thread when it names none. The commands go to the Java SDK's device
(`sensor.getDevice()`), whose methods block: call them on a background thread.

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

A lambda is a listener of the items alone, and a second argument names another
executor:

```java
Cancellable items = sr04.measurements(m -> show(m.distanceMm()));                 // main thread
Cancellable elsewhere = sr04.measurements(m -> show(m.distanceMm()), background); // on another executor
```

`sensor.close()` returns at once, before the USB interface is released;
`sensor.close(callback)` answers once it is — wait for that before the same
board is opened again (`isClosed()` is true from the moment the closing began):

```java
sensor.close(new DepzCallback<Void>() {
    @Override public void onResult(Void released) { reopen(); }     // main thread: the board is free
    @Override public void onError(Exception error) { showError(error); }   // closed all the same
});
```

A third argument sets the buffer between the sensor and the executor (64 items
by default, the oldest dropped when the executor is slower than the sensor).

## Recording

Two files, the ones the desktop SDKs write, each into an `OutputStream` the app
opens — a file of its storage, or `contentResolver.openOutputStream(uri)` for a
document the user picked (the Storage Access Framework). The SDK closes the
stream when the recording ends, and when it could not start.

**A dataset** (`.depzdata`, the dataset format, contract 09):
the decoded measurements on the host's timeline, for the web viewer and the
`DatasetReader` / `DatasetPlayer` of every SDK. Written by the Java SDK's
`SessionRecorder`:

```kotlin
val recording = sensor.record(File(context.filesDir, "run.depzdata").outputStream())
// … the sensor streams: every item is a record of the file, beside the app's own flows
val records = recording.closeAndWait()   // the file is complete
```

`record` synchronises the board's clock with the phone's and hooks the sensor's
stream; it does not configure, start or stop the sensor. The recording ends by
`close()` / `closeAndWait()`, and by itself when the sensor is closed or lost;
a `record` call that is cancelled records nothing. Options: `note` (the header's
free text), `vl53l8Layers` (signal, ambient, sigma and reflectance per zone),
`temperatureHz` (`temperature` records per second, 1 by default, `null` for
none). For a `.depzdata.gz` pass a `GZIPOutputStream`. Several sensors into one
file, on one timeline: `DepzRecording.start(output, listOf(tof, imu))`.

From Java: `sensor.record(output, callback)` answers a
`DepzCallback<DepzRecording>`, `recording.close(callback)` answers the number of
records once the file is closed.

**A capture of the link** (`.depzrec`, the recording format, contract 08):
every byte exchanged with the board, which any SDK replays as if the board were
attached (`ReplayLink`) — what to attach to a bug report. It starts with the
first probe, so it is asked for when the board is opened:

```kotlin
val sensor = DepzAndroid.open(context, board, capture = File(context.filesDir, "run.depzrec").outputStream())
// … closing the sensor (or losing it) closes the capture
```

The sample app's Record button writes a dataset into the app's files directory
and shows its path; `adb shell am start … --ez record true` records every board
from the moment it streams. From a debug build the file comes off the device
with `adb exec-out run-as ai.depz.sensor.android.sample cat files/<name> > <name>`.

## In an app

Android 12 (`minSdk` 31) or newer, Android Gradle Plugin 8.1 or newer (the Java
SDK's records). Nothing else to configure:

- **No core-library desugaring.** Neither SDK calls a Java API that Android 12
  lacks.
- **R8 (`isMinifyEnabled = true`) needs no rules for the SDK.** The Java SDK's
  jar brings its own (`META-INF/proguard`), which the Android Gradle Plugin
  applies: they keep the class names that are data — a BNO086 report's `type`
  in a dataset is its class's name — and the Android SDK needs none. A rule is
  the app's to add only for names the app itself reads, as the sample does to
  show `sensor.device.javaClass.simpleName`
  ([sample/proguard-rules.pro](sample/proguard-rules.pro)).
- **No native code, no desktop serial library.** The AAR holds Kotlin classes
  only; jSerialComm, the Java SDK's desktop transport, is excluded from its
  dependencies. The sensor firmware images the Java SDK uploads (VL53L5/L7/L8,
  TMF8829) come as Java resources of its jar and are packed into the APK as
  they are.

## In the field

From two devices with 10 boards on them (an Orange Pi 5B, Android 12; a Samsung
Galaxy A35, Android 16; 2026-11-11). The guide's
[In the field](docs/guide.md#in-the-field) has each point in full.

- **OTG adapter, powered hub.** A phone's USB-C port is a host port only
  through an OTG adapter, and it gives little current: several boards go on a
  hub with its own power supply — both devices ran their 10 boards from one.
  `discover` ignores the hub. A phone has one port, so `adb` goes over Wi-Fi.
- **Google Play Protect** blocked a sideloaded APK that targeted API 31 as
  "built for an older version of Android"; with `targetSdk = 35` it installed.
  Keep `minSdk = 31` and target a recent API level; a library module's test APK
  targets `minSdk` unless `testOptions { targetSdk = … }` says otherwise.
- **The permission dialog.** One per board, one at a time, in a queue the SDK
  keeps. A grant lasts until the board is unplugged or the app uninstalled, and
  outlives the app's process. There is no "use by default" checkbox: the SDK
  and the sample declare no `USB_DEVICE_ATTACHED` filter. An app stopped while
  its dialog is open loses the dialog's answer, not the grant: a request that
  waits reads `hasPermission` every 500 ms.
- **A board is not listed.** Look at Android's own list first
  (`UsbManager.deviceList`): a board missing there is a matter of the adapter,
  the cable or the hub's power; a board there and not in `discover` has no DEPZ
  USB id; a board in `discover` that `find(serial)` does not return has no
  permission yet. The guide's
  [checklist](docs/guide.md#a-board-is-not-listed) goes on from there.

## Build from the sources

<!-- doc-check: skip — a build of this repository's sources, not an install command of a published package -->
```sh
# in packages/depz-sensor-sdk-java — only while the Java SDK's version is not on Maven Central
mvn -Dgpg.skip=true install                         # the Java SDK into the local Maven repository (~/.m2)
# in packages/depz-sensor-sdk-android
gradle :sdk:publishCentralPublicationToMavenLocal   # the SDK into the local Maven repository (~/.m2)
gradle :sample:assembleDebug                         # the sample app: sample/build/outputs/apk/debug/sample-debug.apk
```

The build needs a JDK 17 or newer, the Android SDK (platform 35, build-tools
35; `ANDROID_HOME` names it), Gradle 8.14 (`gradle` on `PATH` — the repository
keeps no Gradle wrapper) and, for the first step, Maven. The Java SDK it
depends on comes from Maven Central, or from `~/.m2` when Maven Central does
not have that version: the first step puts it there (`-Dgpg.skip=true`: the
build signs the jar for Maven Central, and that needs a key). Between releases
the dependency is a `-SNAPSHOT` version (`api("io.github.depz-ai:depz-sensor-sdk:…")`
in `sdk/build.gradle.kts`): write that version into the first `<version>` of
the Java SDK's `pom.xml` before the first step. An app takes the SDK built
this way with `mavenLocal()` among the repositories of its `settings.gradle.kts`.

`gradle :sdk:checkReleaseAar` and
`gradle :sample:checkReleaseApk` look into what was built: the AAR holds this
package's classes and nothing else; the release APK, shrunk by R8, has the
sensor firmware images and the class names that are data, no desktop serial
library and no native code.
