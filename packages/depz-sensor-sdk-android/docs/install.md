# Android SDK — install

What an app needs to use `depz-sensor-sdk-android`, the one line that adds it,
what that line brings into the app, and how to build it from the sources. A
first program is in the
[introduction](intro.md#quick-start).

## Requirements

- **Android 12 (API 31) or newer** on the device: `minSdk = 31`.
- **A USB host port**: a phone through an OTG adapter, a board computer's own
  ports. Several boards go on a hub with its own power supply — the guide's
  [In the field](guide.md#wiring-an-otg-adapter-and-a-powered-hub) has the
  wiring.
- **Android Gradle Plugin 8.1 or newer** (the Java SDK under this one uses
  Java records), in a standard Android Studio / Gradle project with
  `mavenCentral()` among its repositories.

## Add the dependency

Maven Central, `io.github.depz-ai:depz-sensor-sdk-android`:

```kotlin
// build.gradle.kts of the app module
android {
    defaultConfig { minSdk = 31 }   // Android 12, or newer
}

dependencies {
    implementation("io.github.depz-ai:depz-sensor-sdk-android:1.0.0")
}
```

The Kotlin and Java classes are in the package `ai.depz.sensor.android`; the
measurement types and the device classes are the Java SDK's, `ai.depz.sensor.*`.
(The Maven group `io.github.depz-ai` and the package names differ on purpose.)

## What comes with it

- **The Java SDK** (`io.github.depz-ai:depz-sensor-sdk`) and
  **`kotlinx-coroutines-core`**, as dependencies of the artifact — nothing to
  add by hand. The Java SDK's desktop serial library (jSerialComm) is left out.
- **One manifest entry**, merged into the app's:
  `<uses-feature android:name="android.hardware.usb.host" android:required="true">`
  — Google Play offers the app only to devices with a USB host port. No
  `<uses-permission>`: USB access is granted per board, by the user, in a
  system dialog ([Permissions](intro.md#permissions)).
- **Nothing to configure**: no core-library desugaring, no R8 / ProGuard rules
  for the SDK (the Java SDK's jar carries its own), no native code. The guide's
  [In an app](guide.md#in-an-app-r8-desugaring-native-code) says why.

## From the sources

The package's sources (the SDK and its sample app) are in the public
repository, [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk),
folder `packages/depz-sensor-sdk-android`. The folder builds the artifact from the sources,
into the machine's local Maven repository (`~/.m2`). The build needs a JDK 17
or newer, the Android SDK (platform 35, build-tools 35), Gradle 8.14 and Maven
(`ANDROID_HOME` set; `gradle` and `mvn` on `PATH`), and the Java SDK of the
same checkout installed into `~/.m2` first — the package's README has the
steps. An app then takes it with `mavenLocal()` among the repositories of its
`settings.gradle.kts`, beside `google()` and `mavenCentral()`, and the
dependency line above as it is.

## Check the install

Build the app: Gradle resolves the dependency and the manifest merges. Then
run the [quick start](intro.md#quick-start) with a board attached — the first
`DepzAndroid.open` shows the system's USB permission dialog, and the distances
appear in the log (`adb logcat -s DEPZ`).

No app at hand? The package's sample app is one:
[Examples](examples.md#the-sample-app) describes it.
