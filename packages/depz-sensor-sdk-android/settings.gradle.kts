pluginManagement {
    repositories { google(); mavenCentral(); gradlePluginPortal() }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        // the unreleased Java SDK only (see sdk/build.gradle.kts)
        mavenLocal { content { includeGroup("io.github.depz-ai") } }
    }
}
rootProject.name = "depz-sensor-sdk-android"
include(":sdk", ":sample")
