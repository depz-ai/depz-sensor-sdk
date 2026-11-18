# depz-sensor-sdk-android — guide

The guide to the Android SDK for the DEPZ USB sensor line: how an app finds the
boards, gets the user's permission, opens a board as the class of its sensor
family, reads its stream, and what happens when a board is unplugged. Install
and a first program are in the [introduction](intro.md). What a sensor measures
and how it is configured is the Java SDK's — the
[Java guide](../../depz-sensor-sdk-java/docs/guide.md) and its per-sensor pages
apply to `sensor.device` as they are.

## Contents

- [What it is](#what-it-is)
- [Discovery](#discovery)
- [The USB permission](#the-usb-permission)
- [Opening a board](#opening-a-board)
- [Sensor families](#sensor-families)
- [Threads and cancellation](#threads-and-cancellation)
- [An unplugged board, and hot-plug](#an-unplugged-board-and-hot-plug)
- [From Java](#from-java)
- [Recording](#recording)
- [In an app: R8, desugaring, native code](#in-an-app-r8-desugaring-native-code)
- [In the field](#in-the-field): [wiring](#wiring-an-otg-adapter-and-a-powered-hub), [Google Play Protect](#installing-a-build-by-hand-google-play-protect), [the permission dialog](#the-permission-dialog-as-the-user-meets-it), [Samsung](#samsung-one-ui), [a board is not listed](#a-board-is-not-listed)

## What it is

Each DEPZ sensor board is a USB CDC-ACM device speaking one framed protocol.
On desktop the Java SDK reaches it through a serial port; Android has no serial
ports for apps, so this SDK implements the Java SDK's `Link` over Android's USB
host API and opens the Java SDK's device classes on it. Everything above the
link — probing, drivers, decoding, time stamps — is the same code as on
desktop, and a `DepzSensor` is a thin Kotlin layer over the Java SDK's device
of the family (`sensor.device`).

Requirements: Android 12 (API 31) or newer, a device with a USB host port (a
phone with an OTG adapter, a board computer), Android Gradle Plugin 8.1 or
newer.

## Discovery

`DepzAndroid.discover` lists the attached DEPZ boards — the USB devices with a
DEPZ id; any other USB device is ignored. Nothing is opened and no permission
is asked. One board is also reached by its USB serial number, or from an
Android `UsbDevice`:

```kotlin
for (board in DepzAndroid.discover(context)) {
    Log.i("DEPZ", "${board.modelHint} at ${board.deviceName}, serial ${board.serial}")
}
val byUsbDevice = DepzAndroid.device(context, DepzAndroid.discover(context).first().usbDevice)
val bySerial = DepzAndroid.find(context, "3275395A3233")   // null: not attached, or no permission yet
```

Several boards of one model differ by `serial`, which Android hides (`null`)
until the app holds the permission for the board; `deviceName` is Android's
name of the device, unique among the attached ones and new after every re-plug.

## The USB permission

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

Android shows one such dialog at a time; concurrent requests take turns.

## Opening a board

`DepzAndroid.open` asks for the permission when it is missing, probes the board
and returns the `DepzSensor` of its family; the caller closes it (`use { }`
works). On any failure — and when the coroutine is cancelled — nothing stays
open.

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

Errors are the Java SDK's exceptions (`DepzException` and its subclasses:
`DepzTimeoutException`, `StatusException`, `DeviceLostException`…) and
`UsbPermissionDeniedException`, thrown by the `suspend` call that met them.

## Sensor families

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

Every stream is a cold `Flow`: a collection gets the items from its start on,
several may run at once, and each has a buffer of 64 items (the oldest is
dropped when the collector is slower than the sensor). The setup differs by
family. A ranging sensor is initialised, configured and started:

```kotlin
sensor.init()
sensor.configure(timingBudgetMs = 50)
sensor.start()
sensor.measurements.collect { m -> if (m.valid()) Log.i("DEPZ", "${m.distanceMm()} mm") }
```

A BNO086 has no start: each report is enabled at its own rate —

```kotlin
sensor.enable(Reports.ROTATION_VECTOR, hz = 100.0)
sensor.reports.collect { report ->
    if (report is Reports.RotationVector) Log.i("DEPZ", "accuracy ${report.accuracy()} at ${report.timestampUs()} us")
}
```

What `DepzSensor` has no function for is a call of the Java SDK's device, which
blocks; `io` runs it off the main thread:

```kotlin
val periodUs = sensor.io { device.samplePeriodUs }   // a blocking Java SDK call, off the main thread
```

## Threads and cancellation

Nothing blocks the main thread: every call that waits is a `suspend` function
that works on `Dispatchers.IO`, and the flows are collected in the collector's
own context. Cancelling a `suspend` call does not interrupt the request under
way — a board is never left with half a command — so the call returns once that
request ended, within its timeouts. Cancelling a collection closes its
subscription at once; the sensor goes on measuring until its `stop()`. `close()`
returns at once; `closeAndWait()` also waits until the USB interface is
released.

## An unplugged board, and hot-plug

A stream ends in one of two ways: the program closes the sensor — the flow
completes — or the board is lost (unplugged, a USB error) — the flow fails with
the Java SDK's `DeviceLostException`, whose cause names the USB device:

```kotlin
try {
    sensor.measurements.collect { m -> /* … */ }
    // here the program itself closed the sensor
} catch (e: DeviceLostException) {
    Log.w("DEPZ", "unplugged: ${e.cause?.message}")   // "/dev/bus/usb/001/005: device detached"
} finally {
    sensor.close()   // a lost sensor is closed too
}
```

The other open boards are not touched, and nothing reconnects by itself. The
list of attached boards as it changes is a `Flow` — a new list whenever a DEPZ
board is plugged in or unplugged; a re-plugged board is a new device with the
same serial, to be opened again:

```kotlin
DepzAndroid.devices(context).collect { boards -> /* show them; open the new one */ }
```

## From Java

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
executor; a third sets the buffer between the sensor and the executor (64 items
by default, drop-oldest):

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

## Recording

A **dataset** (`.depzdata`) holds the decoded measurements on the host's
timeline, for the web viewer and the `DatasetReader` / `DatasetPlayer` of every
DEPZ SDK. It goes into an `OutputStream` the app opens — a file of its storage,
or `contentResolver.openOutputStream(uri)` for a document the user picked:

```kotlin
val recording = sensor.record(File(context.filesDir, "run.depzdata").outputStream())
// … the sensor streams: every item is a record of the file, beside the app's own flows
val records = recording.closeAndWait()   // the file is complete
```

`record` synchronises the board's clock with the phone's and hooks the sensor's
stream; it does not configure, start or stop the sensor. The recording ends by
`close()` / `closeAndWait()`, and by itself when the sensor is closed or lost.
`DepzRecording.start(output, sensors)` records several sensors into one file
on one timeline.

A **capture of the link** (`.depzrec`) holds every byte exchanged with the
board, which any DEPZ SDK replays as if the board were attached — what to
attach to a bug report. It starts with the first probe, so it is asked for when
the board is opened:

```kotlin
val sensor = DepzAndroid.open(context, board, capture = File(context.filesDir, "run.depzrec").outputStream())
// … closing the sensor (or losing it) closes the capture
```

## In an app: R8, desugaring, native code

- **No core-library desugaring.** Neither SDK calls a Java API that Android 12
  lacks.
- **R8 (`isMinifyEnabled = true`) needs no rules for the SDK.** The Java SDK's
  jar brings its own, which the Android Gradle Plugin applies; a rule is the
  app's to add only for class names the app itself reads.
- **No native code, no desktop serial library.** The AAR holds Kotlin classes
  only; the sensor firmware images the Java SDK uploads (VL53L5/L7/L8, TMF8829)
  are Java resources of its jar and are packed into the APK as they are.

## In the field

What two devices with 10 boards on them showed on 2026-11-11 — an Orange Pi 5B
board computer (Android 12) and a Samsung Galaxy A35 phone (Android 16) — and
what the SDK does about it. On each, the SDK's tests on a device with the
boards attached ended with 21 passed, 7 skipped,
0 failed, and the test that unplugs a board and plugs it back passed by hand.

### Wiring: an OTG adapter and a powered hub

- **A phone needs an OTG adapter.** Its USB-C port is a host port only through
  one (USB-C to USB-A, "OTG" or "host"); a board computer has host ports of its
  own. Without a host port Android lists no USB device, and
  `DepzAndroid.discover` returns an empty list.
- **Several boards need a hub with its own power supply.** A board takes its
  power from the port it is plugged into, and a phone's port gives little
  current. Both devices above ran their 10 boards from one powered hub — on the
  phone, behind the OTG adapter. How many boards a phone feeds without one was
  not measured.
- **The hub is not in the way.** Android lists a hub as one more USB device;
  `discover` ignores every device without a DEPZ id, and `devices` gives no new
  list when such a device comes or goes. Every board behind the hub is a USB
  device of its own, with its own permission, opened and closed apart from the
  others.
- **A phone has one port.** With the boards on it, `adb` reaches the phone over
  Wi-Fi (Android's wireless debugging): `adb connect <host:port>`, then
  `adb -s <host:port> …` names that device.

### Installing a build by hand: Google Play Protect

On the phone, Google Play Protect blocked the install of an APK as "built for
an older version of Android". The APK was the SDK's own test APK, which
targeted API 31: the test APK of a library module targets the module's `minSdk`
unless the module says otherwise. With `targetSdk = 35` it installed, and so did
the sample app, which targets 35 from the start. What decides is the APK's
`targetSdk`, not its `minSdk`: an app keeps `minSdk = 31` and targets a recent
API level.

<!-- doc-check: skip — fragments of an app's and a library's build.gradle.kts, no dependency in them; the values are the sample's (sample/build.gradle.kts) and the SDK's (sdk/build.gradle.kts) -->
```kotlin
// build.gradle.kts of an app module
android {
    defaultConfig {
        minSdk = 31      // Android 12: the oldest the SDK runs on
        targetSdk = 35   // a recent level: what Google Play Protect looks at
    }
}

// build.gradle.kts of a library module with tests for a device (src/androidTest)
android {
    testOptions { targetSdk = 35 }   // the test APK's; without it, the module's minSdk
}
```

### The permission dialog, as the user meets it

- **One dialog per board.** The permission is for one USB device, so an app
  with 10 boards attached shows 10 dialogs. Android shows one at a time and the
  SDK queues the requests: the next dialog comes up when the one before it is
  answered. Asking for all of them ahead of the first `open`:

```kotlin
for (board in DepzAndroid.discover(context)) {   // one dialog per board, one after another
    if (!DepzAndroid.requestPermission(context, board)) Log.w("DEPZ", "no USB permission for ${board.deviceName}")
}
val boards = DepzAndroid.discover(context)   // read again: the serials are readable now
```

- **How long a grant lasts.** Until the board is unplugged, and until the app
  is uninstalled. It outlives the app's process: an app started again finds its
  boards granted (`hasPermission`) and `open` shows no dialog. A board plugged
  in again is a new USB device and is asked for again.
- **No "use by default" checkbox.** Android offers to remember the choice only
  to an app whose manifest names the device in a `USB_DEVICE_ATTACHED` intent
  filter. The SDK's manifest declares the USB host feature and nothing else, and
  the sample app has no such filter: their dialog has two buttons — the tests
  for a device answer it with the keys TAB, TAB, ENTER — and nothing is
  remembered. An app that adds such a filter of its own is outside what this
  repository tests.
- **The app is stopped while its dialog is open.** The dialog is the system's
  and stays on the screen. The user's "OK" then grants the board, but the
  answer goes to the process that asked, which is gone — the next process of
  the app would wait for an answer that never comes, and every other board
  behind it in the queue. So a request that waits also reads `hasPermission`
  every 500 ms and ends with `true` as soon as the system holds the grant. The
  answer of a dialog an earlier process left behind is never taken for the
  answer of a new request: each request listens under a name of its own.
- **A wait the app wants to bound** is a cancelled coroutine: the request stops
  listening, and the next one in the queue gets its turn. The dialog itself
  stays until the user dismisses it.

```kotlin
val granted = withTimeoutOrNull(60_000) { DepzAndroid.requestPermission(context, board) }   // null: no answer
```

### Samsung (One UI)

Nothing in the SDK depends on the manufacturer. What the Galaxy A35 (Android
16) showed that the Orange Pi did not:

- Google Play Protect and the `targetSdk` of a sideloaded APK,
  [above](#installing-a-build-by-hand-google-play-protect).
- The phone stopped the app two seconds after its first start, with the first
  permission dialog on the screen; why is not known. What follows is the case
  "the app is stopped while its dialog is open"
  [above](#the-permission-dialog-as-the-user-meets-it), which the SDK gets
  through by itself.

### A board is not listed

`DepzAndroid.discover` returns the USB devices Android lists whose id is a DEPZ
one. So the first question is whether Android sees the board at all:

```kotlin
val usb = context.getSystemService(UsbManager::class.java)
for (device in usb.deviceList.values) {   // every USB device Android sees, DEPZ or not
    Log.i("DEPZ", "%04X:%04X at %s".format(device.vendorId, device.productId, device.deviceName))
}
Log.i("DEPZ", "DEPZ boards: ${DepzAndroid.discover(context)}")
```

1. **Android's list is empty, or lacks the board** — the cause is below the
   SDK: the OTG adapter (a phone), the cable, the hub's power supply.
2. **The board is in Android's list and not in `discover`** — its USB id is not
   a DEPZ one. The DEPZ ids are vendor `1BCF` with a product id from `EC78` to
   `FFFF`, and `0483:56DC`, a board that was not programmed with its own id
   yet.
3. **`discover` lists the board and `find(serial)` returns `null`** — the app
   holds no permission for it yet, and Android hides the serial until then:
   `requestPermission`, then `discover` or `find` again.
4. **The board is listed with another `deviceName` than a moment ago** — it was
   unplugged and plugged in again: a new USB device, to be asked for and
   opened again. A `DepzUsbDevice` kept from
   before is a snapshot of a device that is gone: `requestPermission` answers
   `false` for it without a dialog.
5. **The list does not follow the boards** — `discover` is a snapshot;
   `DepzAndroid.devices(context)` gives a new list after every board attached
   or detached.
6. **The board is listed and does not open** — `open` throws a
   `DeviceLostException` whose message says what failed. "cannot open" and
   "cannot claim the CDC interfaces": Android did not give the device or its
   interfaces — nothing else may hold the board (the bench's script stops the
   sample app before its tests). "no CDC control interface" and its like: the
   USB device is not a DEPZ sensor board.
