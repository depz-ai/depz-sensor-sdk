import java.util.zip.ZipFile
import java.util.zip.ZipInputStream

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.dokka")
    `maven-publish`
}

// The Maven coordinate: group and version are the package's
// (gradle.properties), the artifact is named here.
val artifact = "depz-sensor-sdk-android"

android {
    namespace = "ai.depz.sensor.android"
    compileSdk = 35
    defaultConfig {
        minSdk = 31
        // The tests on a device with boards attached (the development repository has them).
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }
    // The test APK targets what the sample app targets: without it a library's test APK targets
    // minSdk, and Google Play Protect on a phone blocks its install as "built for an older version of
    // Android" (Samsung A35, Android 16, 2026-11-11).
    testOptions {
        targetSdk = 35
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
        // Interface methods with a body are Java default methods: a Java lambda is a StreamListener.
        freeCompilerArgs += "-Xjvm-default=all"
    }
    lint {
        // Lint runs with warnings as errors: any error or warning fails the build, with no
        // baseline file to hide one behind. A call to an API above minSdk without a version check
        // (NewApi) crashes on Android 12, so it stays an error whatever the lint defaults become.
        abortOnError = true
        warningsAsErrors = true
        error += "NewApi"
        // Not findings in the code: "a newer version exists" of compileSdk and of the
        // dependencies — they would fail the build on the day of somebody else's release.
        disable += setOf("GradleDependency", "NewerVersionAvailable")
        explainIssues = false   // the finding and its place, not the essay
    }
    publishing {
        // The release AAR and its sources. The javadoc jar is javadocJar below, not withJavadocJar():
        // the Dokka inside the Android Gradle Plugin cannot read the Java SDK's records
        // ("Record requires ASM8") and fails.
        singleVariant("release") { withSourcesJar() }
    }
}

// The unit tests of the development repository read its recorded sessions by path: an input of
// theirs, or a changed recording would find the tests up to date without a run. The public
// repository has neither the tests nor the recordings.
val recordings = rootDir.resolve("../../contracts/vectors/recordings")
if (recordings.isDirectory) {
    tasks.withType<Test>().configureEach {
        inputs.dir(recordings).withPropertyName("recordings").withPathSensitivity(PathSensitivity.RELATIVE)
    }
}

// The API reference Maven Central asks for beside every artifact: the KDoc as Dokka's HTML.
val javadocJar = tasks.register<Jar>("javadocJar") {
    archiveClassifier = "javadoc"
    from(tasks.named("dokkaGeneratePublicationHtml"))
}

// The publication, with the POM metadata of the Java SDK's pom.xml. `publishCentralPublicationTo
// CentralBundleRepository` writes it as a Maven repository layout under build/central-bundle —
// unsigned (the release signs, zips and uploads it), and checkCentralBundle (at the end of this
// file) reads it.
publishing {
    publications {
        register<MavenPublication>("central") {
            artifactId = artifact
            afterEvaluate { from(components["release"]) }
            artifact(javadocJar)
            pom {
                name = artifact
                description = "Android SDK (Kotlin, Android 12+) for the DEPZ USB sensor line — USB host " +
                    "transport, discovery, permission, coroutines / Flow and Java callbacks on top of the DEPZ Java SDK."
                url = "https://github.com/depz-ai/depz-sensor-sdk"
                licenses {
                    license {
                        name = "MIT License"
                        url = "https://opensource.org/licenses/MIT"
                        distribution = "repo"
                    }
                }
                developers {
                    developer {
                        name = "DEPZ"
                        organization = "DEPZ"
                        organizationUrl = "https://depz.ai"
                    }
                }
                scm {
                    connection = "scm:git:https://github.com/depz-ai/depz-sensor-sdk.git"
                    developerConnection = "scm:git:ssh://git@github.com/depz-ai/depz-sensor-sdk.git"
                    url = "https://github.com/depz-ai/depz-sensor-sdk"
                }
            }
        }
    }
    repositories {
        maven {
            name = "centralBundle"
            url = uri(layout.buildDirectory.dir("central-bundle"))
        }
    }
}

dependencies {
    // The Java SDK's next release, built from this repo into ~/.m2 until it is published:
    // `mvn install` of packages/depz-sensor-sdk-java under the version written here puts it there.
    api("io.github.depz-ai:depz-sensor-sdk:1.0.0") {
        // Desktop serial ports; Android talks to the board through UsbManager.
        exclude(group = "com.fazecast", module = "jSerialComm")
    }
    // The public API suspends (DepzAndroid.requestPermission, DepzAndroid.open).
    api("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.9.0")
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.9.0")
    androidTestImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test:runner:1.6.2")
}

// What an app gets with the SDK: the AAR holds this package's classes and nothing else — no native
// library, no embedded jar, no rules of its own (the SDK uses no reflection; the Java SDK's R8
// rules come in its jar, META-INF/proguard) — and nothing it depends on at run time is the desktop
// serial library or carries native code.
val nativeCode = Regex(""".*\.(so|dll|dylib|jnilib)$""")
tasks.register("checkReleaseAar") {
    val aar = layout.buildDirectory.file("outputs/aar/sdk-release.aar")
    // the jars an app's runtime classpath gets with the SDK
    val runtimeJars = configurations.named("releaseRuntimeClasspath").map {
        it.incoming.artifactView {
            attributes.attribute(Attribute.of("artifactType", String::class.java), "android-classes-jar")
        }.artifacts
    }
    dependsOn("bundleReleaseAar")
    inputs.files(runtimeJars.map { it.artifactFiles })
    doLast {
        val problems = mutableListOf<String>()
        ZipFile(aar.get().asFile).use { zip ->
            val entries = zip.entries().asSequence().filterNot { it.isDirectory }.toList()
            entries.map { it.name }.filter { it.startsWith("jni/") || it.startsWith("libs/") || nativeCode.matches(it) }
                .forEach { problems += "the AAR holds $it" }
            val classes = entries.single { it.name == "classes.jar" }
            ZipInputStream(zip.getInputStream(classes)).use { jar ->
                generateSequence { jar.nextEntry }.filterNot { it.isDirectory }.map { it.name }
                    .filterNot { it.startsWith("ai/depz/sensor/android/") || it.startsWith("META-INF/") }
                    .forEach { problems += "the AAR's classes.jar holds $it" }
            }
        }
        val artifacts = runtimeJars.get().artifacts
        check(artifacts.any { "depz-sensor-sdk" in it.id.componentIdentifier.displayName }) {
            "the Java SDK is not on the AAR's runtime classpath: ${artifacts.map { it.id.componentIdentifier.displayName }}"
        }
        for (artifact in artifacts) {
            val name = artifact.id.componentIdentifier.displayName
            if ("fazecast" in name.lowercase() || "jserialcomm" in name.lowercase()) problems += "runtime dependency $name"
            ZipFile(artifact.file).use { jar ->
                jar.entries().asSequence().filterNot { it.isDirectory }.map { it.name }
                    .filter { nativeCode.matches(it) || it.startsWith("com/fazecast/") }
                    .forEach { problems += "runtime dependency $name holds $it" }
            }
        }
        check(problems.isEmpty()) { "desktop or native code reaches an app through the AAR:\n" + problems.joinToString("\n") }
        logger.lifecycle("checkReleaseAar: ${aar.get().asFile.name} and its ${artifacts.size} runtime dependencies: no jSerialComm, no native code")
    }
}

// The bundle directory holds one build's files: a snapshot's are named by their time stamp, so an
// earlier run's would stay beside them.
val centralBundle = layout.buildDirectory.dir("central-bundle")
tasks.withType<PublishToMavenRepository>().configureEach {
    if (name == "publishCentralPublicationToCentralBundleRepository") {
        val dir = centralBundle
        doFirst { dir.get().asFile.deleteRecursively() }
    }
}

// What Maven Central requires of a release (https://central.sonatype.org/publish/requirements/),
// read from the published files: the AAR, the sources jar and the javadoc jar with their content,
// the checksums, the POM's coordinate, name, description, url, licence, developer and scm, and the
// POM's dependencies — the Java SDK without jSerialComm. The signatures are the release's.
tasks.register("checkCentralBundle") {
    dependsOn("publishCentralPublicationToCentralBundleRepository")
    val dir = centralBundle
    val group = project.group.toString()
    val version = project.version.toString()
    val artifactId = artifact
    doLast {
        val home = dir.get().asFile.resolve("${group.replace('.', '/')}/$artifactId/$version")
        val files = home.listFiles().orEmpty().filter { it.isFile }
        val problems = mutableListOf<String>()
        // <artifact>-<version or a snapshot's time stamp><suffix>
        fun published(suffix: String): File? {
            val found = files.filter { it.name.startsWith("$artifactId-") && it.name.endsWith(suffix) &&
                it.name.removePrefix("$artifactId-").removeSuffix(suffix).none { c -> c.isLetter() } }
            if (found.size != 1) problems += "expected one *$suffix in $home, found ${found.map { it.name }}"
            return found.singleOrNull()
        }
        fun entries(file: File?) = if (file == null) emptyList() else ZipFile(file).use { zip ->
            zip.entries().asSequence().filterNot { it.isDirectory }.map { it.name }.toList()
        }
        val aar = published(".aar")
        val pom = published(".pom")
        val sources = published("-sources.jar")
        val javadoc = published("-javadoc.jar")
        for (file in listOfNotNull(aar, pom, sources, javadoc, published(".module"))) {
            for (sum in listOf("md5", "sha1")) {
                if (!File(file.path + ".$sum").isFile) problems += "${file.name} has no .$sum"
            }
        }
        if ("classes.jar" !in entries(aar)) problems += "the AAR has no classes.jar"
        if ("ai/depz/sensor/android/DepzAndroid.kt" !in entries(sources)) problems += "the sources jar has no DepzAndroid.kt"
        val pages = entries(javadoc)
        if ("index.html" !in pages || pages.none { it.endsWith("/-depz-android/index.html") }) {
            problems += "the javadoc jar has no page of DepzAndroid (${pages.size} files)"
        }
        if (pom != null) {
            val text = pom.readText()
            fun element(path: String): String? {
                var scope = text
                for (tag in path.split("/")) {
                    scope = Regex("<$tag>(.*?)</$tag>", RegexOption.DOT_MATCHES_ALL).find(scope)?.groupValues?.get(1) ?: return null
                }
                return scope.trim()
            }
            // the project's own elements, not a dependency's
            val head = text.replace(Regex("<dependencies>.*?</dependencies>", RegexOption.DOT_MATCHES_ALL), "")
            fun own(tag: String) = Regex("<$tag>(.*?)</$tag>").find(head)?.groupValues?.get(1)
            val expected = mapOf("groupId" to group, "artifactId" to artifactId, "version" to version, "packaging" to "aar")
            for ((tag, value) in expected) {
                if (own(tag) != value) problems += "the POM's <$tag> is ${own(tag)}, not $value"
            }
            for (path in listOf("name", "description", "url", "licenses/license/name", "licenses/license/url",
                "developers/developer/name", "scm/connection", "scm/developerConnection", "scm/url")) {
                if (element(path).isNullOrEmpty()) problems += "the POM has no <$path>"
            }
            val dependencies = Regex("<dependency>(.*?)</dependency>", RegexOption.DOT_MATCHES_ALL).findAll(text)
                .map { it.groupValues[1] }.toList()
            val javaSdk = dependencies.singleOrNull { "<artifactId>depz-sensor-sdk</artifactId>" in it }
            when {
                javaSdk == null -> problems += "the POM does not depend on the Java SDK"
                "<scope>compile</scope>" !in javaSdk -> problems += "the Java SDK is not a compile-scope dependency of the POM"
                !Regex("<exclusion>\\s*<groupId>com.fazecast</groupId>\\s*<artifactId>jSerialComm</artifactId>\\s*</exclusion>")
                    .containsMatchIn(javaSdk) -> problems += "the POM's Java SDK dependency does not exclude jSerialComm"
            }
            if (dependencies.none { "<artifactId>kotlinx-coroutines-core</artifactId>" in it }) {
                problems += "the POM does not depend on kotlinx-coroutines-core"
            }
            dependencies.filter { "fazecast" in it.substringBefore("<exclusions>") }.forEach { problems += "the POM depends on jSerialComm" }
        }
        check(problems.isEmpty()) { "the publication is not what Maven Central takes:\n" + problems.joinToString("\n") }
        logger.lifecycle("checkCentralBundle: $group:$artifactId:$version — AAR, sources, javadoc (${pages.size} files), POM, checksums")
    }
}
