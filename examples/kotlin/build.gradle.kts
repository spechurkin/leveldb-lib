plugins {
    kotlin("jvm") version "2.4.21"
    application
}
repositories { mavenCentral() }
java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
}
dependencies {
    implementation(files("../../bindings/jvm/build/libs/multileveldb-1.0.0.jar"))
    runtimeOnly("net.java.dev.jna:jna:5.19.1")
}
kotlin {
    jvmToolchain(25)
    compilerOptions { jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_1_8) }
    sourceSets.main { kotlin.srcDir("."); kotlin.include("Example.kt") }
}
application { mainClass.set("ExampleKt") }
