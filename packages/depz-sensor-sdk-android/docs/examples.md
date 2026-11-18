# Android SDK — examples

The package's example is one app: `sample/`, the attached DEPZ boards as a
list of cards, each connected, configured, shown live and recorded through the
SDK's Kotlin API. Its sources are in the package's folder of the public
repository ([Install](install.md#from-the-sources)). This page builds and starts
it, says where in its sources
each thing an app does with a board is, and lists the guide's short snippets by
task.

## The sample app

One screen, a card per attached board:

- the board's name, its USB id and — once the permission is granted — its
  serial number;
- **Connect / Disconnect**: the USB permission dialog when the app does not
  hold the permission yet, then the board opened as the class of its sensor
  family;
- the family's settings (an HC-SR04's period, a VL53L4CD's timing budget, a
  VL53L8's zones and frame rate, …), applied while the board streams;
- the live view: a distance as a number, the zones of a multizone ToF sensor as
  a heatmap, an IMU's orientation and acceleration as text;
- **Record**: the board's measurements into a dataset file (`.depzdata`, the
  format of the desktop SDKs and the web viewer) in the app's files directory;
- the errors as the user meets them: a refused permission, an unplugged board.

Several boards run at once, and the list follows the boards plugged in and
unplugged while the app runs.

### Build and install it

The app is built from the package's sources — no built APK is published —
with the tools of the package's build
([Install](install.md#from-the-sources) names them). From the
package's folder, `packages/depz-sensor-sdk-android`, with the device on
`adb`:

<!-- doc-check: skip — a build of this repository's sample app and adb on the user's device, not an install command of a published package -->
```sh
gradle :sample:assembleDebug                                     # the debug APK
adb install -r sample/build/outputs/apk/debug/sample-debug.apk
adb shell am start -n ai.depz.sensor.android.sample/.MainActivity
```

At launch every attached board is connected: answer the permission dialogs —
one per board, one after another. A phone has one USB port, taken by the
boards: `adb` then goes over Wi-Fi.

### Run it from adb

The launch takes extras, for runs without a hand on the screen, and the log
(tag `DEPZ`) gets a `RATE <board> | <state> | n=<items> | <items>/s | <detail>`
line per board every 5 s:

<!-- doc-check: skip — adb commands on the sample app's own launch extras -->
```sh
adb shell am start -n ai.depz.sensor.android.sample/.MainActivity --ei zones 16 --ei hz 30 --ez record true
adb logcat -s DEPZ
```

| Extra | What it does |
|---|---|
| `--ez connect false` | Leave the boards to their Connect buttons. |
| `--ei zones 16` / `64` | VL53L5CX / L7 / L8: 4×4 or 8×8 zones (default 64). |
| `--ei hz N` | VL53L5CX / L7 / L8: frames per second (default 15). |
| `--ez reset true` | VL53L4CD and the 1D ToF family: pulse XSHUT before the init. |
| `--ez nostop true` | Close the boards without stopping their sensors. |
| `--ez record true` | Record every board from the moment it streams (the log tells the files). |
| `--ez finish true` | Close the running app. |

A recording comes off the device, from a debug build, with

<!-- doc-check: skip — adb on the user's device; <name> is the file the app showed -->
```sh
adb exec-out run-as ai.depz.sensor.android.sample cat files/<name> > <name>
```

and opens in the web viewer ([sensor-viewer.depz.ai](https://sensor-viewer.depz.ai))
and with the `DatasetReader` of every DEPZ SDK.

### Where to look in its sources

The sources are `sample/src/main/kotlin/ai/depz/sensor/android/sample/`. All of
it is the SDK's Kotlin API called from the main thread.

| What an app does | File | The SDK calls |
|---|---|---|
| List the boards, follow hot-plug | `MainActivity.kt` (`rescan`, `onCreate`) | `DepzAndroid.discover`, `DepzAndroid.devices` |
| Connect a board: permission, open, the errors by type | `MainActivity.kt` (`connect`, `session`) | `DepzAndroid.hasPermission`, `DepzAndroid.open`; `UsbPermissionDeniedException`, `DeviceLostException` |
| Each family between `open` and its first item; settings changed while it streams | `Live.kt` (`liveOf` and a class per family) | `init`, `configure`, `start`, `stop`, `enable`, `io`; the `Flow` of the family |
| Turn an item into what the screen shows | `Readings.kt`, `HeatmapView.kt` | the Java SDK's measurement types |
| Record into a file, name it, close it | `MainActivity.kt` (`record`, `save`), `RecordingFile.kt` | `DepzSensor.record`, `DepzRecording.closeAndWait` |
| Close a board, and reopen it only once it is released | `MainActivity.kt` (`close`), `Connections.kt` | `stop`, `closeAndWait` |
| Ship it: R8, the one rule the app adds for itself | `sample/build.gradle.kts`, `sample/proguard-rules.pro` | — |

## Snippets by task

The short programs of the other pages, each compiled with the SDK's tests:

| Task | Where |
|---|---|
| First program: one board, its distance in the log — Kotlin and Java | [Introduction, "Quick start"](intro.md#quick-start) |
| List the boards; find one by its serial number | [Guide, "Discovery"](guide.md#discovery) |
| Ask for the USB permission ahead of time, for every board, with a time limit | [Guide, "The USB permission"](guide.md#the-usb-permission) |
| Open a board and branch on its sensor family | [Guide, "Opening a board"](guide.md#opening-a-board) |
| A ranging sensor, an IMU, a blocking Java SDK call | [Guide, "Sensor families"](guide.md#sensor-families) |
| An unplugged board; the list of boards as it changes | [Guide, "An unplugged board, and hot-plug"](guide.md#an-unplugged-board-and-hot-plug) |
| Callbacks and listeners instead of coroutines | [Guide, "From Java"](guide.md#from-java) |
| Record a dataset; capture the link for a bug report | [Guide, "Recording"](guide.md#recording) |
