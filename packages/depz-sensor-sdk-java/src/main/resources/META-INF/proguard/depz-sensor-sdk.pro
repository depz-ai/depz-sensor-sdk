# R8 / ProGuard rules for apps that shrink this jar (an Android app: the Android Gradle Plugin
# applies META-INF/proguard/*.pro of a dependency jar). jSerialComm is the desktop serial
# transport only (SerialPortLink, SerialPorts) and is excluded from an Android runtime, where a
# USB Link replaces it; without this rule R8 fails the build on the missing classes
# (docs/plans/android-readiness.md).
-dontwarn com.fazecast.jSerialComm.**
# The JSpecify nullness annotations are for compilers (Java, Kotlin): nothing reads them at run time,
# and an app may leave the annotations-only jar out of its runtime classes.
-dontwarn org.jspecify.annotations.**
# Class names that are data. A BNO086 report's kind is its record's simple name
# (Reports.Report.type()): the `type` of a dataset record (contract 09) and of the vectors.
# Class.getSimpleName() reads the name of a nested class from the InnerClasses attribute, which R8
# leaves only on kept classes, so the attribute and the outer class are named too. Renamed, a
# recording made by a shrunk app would hold "b" for "Acceleration".
-keepattributes InnerClasses,EnclosingMethod
-keepnames class ai.depz.sensor.sensors.bno086.Reports
-keepnames class * implements ai.depz.sensor.sensors.bno086.Reports$Report
# Vl53lxDevice.identifyProduct() reports the driver's class as `driver_class`.
-keepnames class * extends ai.depz.sensor.sensors.vl53lx.Vl53lxDriver
