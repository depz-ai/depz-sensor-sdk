import java.util.zip.ZipFile

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "ai.depz.sensor.android.sample"
    compileSdk = 35
    defaultConfig {
        applicationId = "ai.depz.sensor.android.sample"
        minSdk = 31
        targetSdk = 35
        versionCode = 1
        versionName = "0.1"
    }
    buildTypes {
        release {
            // The build an app ships: R8 shrinks and obfuscates the app, the Android SDK and the
            // Java SDK under it. Signed with the debug key so that the APK installs on a bench.
            isMinifyEnabled = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            signingConfig = signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
    lint {
        // As in the SDK: an error or a warning fails the build, and a call above minSdk is an error.
        abortOnError = true
        warningsAsErrors = true
        error += "NewApi"
        // Not findings in the code: "a newer version exists" (compileSdk and targetSdk 35 are the
        // task's; see the SDK). And what a sample without resources leaves out on purpose: its
        // texts are English literals in the code (SetTextI18n), it has no icon of its own
        // (MissingApplicationIcon) and no backup rules — it stores only the recordings its user
        // makes (DataExtractionRules).
        disable += setOf(
            "GradleDependency", "NewerVersionAvailable", "OldTargetApi",
            "SetTextI18n", "MissingApplicationIcon", "DataExtractionRules",
        )
        explainIssues = false
    }
}

dependencies {
    implementation(project(":sdk"))
    testImplementation("junit:junit:4.13.2")
}

// The release APK is the app a user of the SDK ships: shrunk and renamed by R8 with only the rules
// the Java SDK's jar brings and the one rule of proguard-rules.pro. Checked in the APK itself,
// since no test runs the shrunk code:
// - every resource of the Java SDK's jar — the sensor firmware images — is in it, byte for byte,
//   under the name the SDK reads it by;
// - the BNO086 report classes are in the dex under their own names, with the InnerClass annotation
//   Android's Class.getSimpleName() reads: the name is the `type` of a dataset record;
// - no class of the desktop serial library, no native library.
tasks.register("checkReleaseApk") {
    val apk = layout.buildDirectory.file("outputs/apk/release/sample-release.apk")
    // the jars of the app's runtime classpath, as R8 gets them
    val runtimeJars = configurations.named("releaseRuntimeClasspath").map {
        it.incoming.artifactView {
            attributes.attribute(Attribute.of("artifactType", String::class.java), "android-classes-jar")
        }.artifacts
    }
    val dexdump = File(android.sdkDirectory, "build-tools/${android.buildToolsVersion}/dexdump")
    dependsOn("assembleRelease")
    inputs.files(runtimeJars.map { it.artifactFiles })
    doLast {
        val problems = mutableListOf<String>()
        val javaSdk = runtimeJars.get().artifacts.single { "io.github.depz-ai:depz-sensor-sdk:" in it.id.componentIdentifier.displayName }.file
        val dexFiles = mutableListOf<File>()
        var resources = 0
        ZipFile(apk.get().asFile).use { zip ->
            val entries = zip.entries().asSequence().associateBy { it.name }
            entries.keys.filter { it.startsWith("lib/") || it.startsWith("com/fazecast/") || Regex(""".*\.(so|dll|dylib|jnilib)$""").matches(it) }
                .forEach { problems += "the APK holds $it" }
            ZipFile(javaSdk).use { jar ->
                for (resource in jar.entries().asSequence().filterNot { it.isDirectory || it.name.endsWith(".class") || it.name.startsWith("META-INF/") }) {
                    resources++
                    val packed = entries[resource.name]
                    if (packed == null) {
                        problems += "the APK lacks the Java SDK's resource ${resource.name}"
                    } else if (!jar.getInputStream(resource).use { it.readBytes() }.contentEquals(zip.getInputStream(packed).use { it.readBytes() })) {
                        problems += "the APK's ${resource.name} differs from the Java SDK's"
                    }
                }
            }
            for (dex in entries.values.filter { it.name.matches(Regex("""classes\d*\.dex""")) }) {
                val file = File(temporaryDir, dex.name)
                file.writeBytes(zip.getInputStream(dex).use { it.readBytes() })
                dexFiles += file
            }
        }
        check(resources > 0) { "no resource found in $javaSdk" }
        // dexdump -a prints a class's annotations, then the class: `… Ldalvik/annotation/InnerClass; accessFlags=25
        // name="Acceleration"` before `Class descriptor  : 'Lai/depz/sensor/sensors/bno086/Reports$Acceleration;'`
        val reports = mutableListOf<String>()
        for (dex in dexFiles) {
            val process = ProcessBuilder(dexdump.path, "-a", dex.path).redirectErrorStream(true).start()
            var innerName: String? = null
            process.inputStream.bufferedReader().useLines { lines ->
                for (line in lines) {
                    if (line.startsWith("Class #") && line.endsWith("annotations:")) innerName = null
                    Regex("""Ldalvik/annotation/InnerClass; .*name="([^"]*)"""").find(line)?.let { innerName = it.groupValues[1] }
                    val type = Regex("""Class descriptor\s*: 'L([^;]*);'""").find(line)?.groupValues?.get(1) ?: continue
                    if (type.startsWith("com/fazecast/")) problems += "the APK's dex defines $type"
                    if (type.startsWith("ai/depz/sensor/sensors/bno086/Reports$")) {
                        val name = type.substringAfter('$')
                        reports += name
                        if (innerName != name) problems += "$type: Class.getSimpleName() gives ${innerName ?: type.substringAfterLast('/')}"
                    }
                    innerName = null
                }
            }
            check(process.waitFor() == 0) { "$dexdump failed on ${dex.name}" }
        }
        // A report class R8 renamed is no `Reports$…` in the dex and so is not in the list above:
        // R8's own map tells it (a class it kept under its name maps to itself, a removed one is absent).
        val mapping = layout.buildDirectory.file("outputs/mapping/release/mapping.txt").get().asFile
        check(mapping.isFile) { "no R8 map at $mapping" }
        // but for the interface `Report` itself: no report's class, so its name is no record's type
        val mapped = mapping.readLines().mapNotNull {
            Regex("""^(ai\.depz\.sensor\.sensors\.bno086\.Reports\$\S+) -> (\S+):$""").find(it)?.destructured
        }.filter { (from) -> from != "ai.depz.sensor.sensors.bno086.Reports\$Report" }
        for ((from, to) in mapped) {
            if (from != to) problems += "R8 renamed $from to $to"
        }
        if (mapped.size != reports.size) {
            problems += "R8's map names ${mapped.size} BNO086 report classes, the dex ${reports.size}"
        }
        // the reports the sample shows and records; R8 renamed every one of them before the Java SDK's rules
        for (name in listOf("Acceleration", "RotationVector")) {
            if (name !in reports) problems += "the APK's dex has no class Reports$$name (found: $reports)"
        }
        check(problems.isEmpty()) { "the release APK:\n" + problems.joinToString("\n") }
        logger.lifecycle("checkReleaseApk: $resources Java SDK resources intact, ${reports.size} BNO086 report classes under their names, no jSerialComm, no native code")
    }
}
