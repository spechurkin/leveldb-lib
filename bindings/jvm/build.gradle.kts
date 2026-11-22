plugins { `java-library`; `maven-publish`; signing }
group = "io.multileveldb"
version = "1.0.0"
repositories { mavenCentral() }
dependencies { api("net.java.dev.jna:jna:5.19.1") }
java {
    toolchain { languageVersion.set(JavaLanguageVersion.of(25)) }
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
    withSourcesJar()
    withJavadocJar()
}
tasks.withType<JavaCompile>().configureEach {
    options.encoding = "UTF-8"
    options.release.set(8)
}
val testJavaVersion = providers.gradleProperty("testJavaVersion").orElse("25").map(String::toInt)
tasks.withType<JavaExec>().configureEach {
    javaLauncher.set(javaToolchains.launcherFor {
        languageVersion.set(testJavaVersion.map { JavaLanguageVersion.of(it) })
    })
    jvmArgs("-ea")
    if (testJavaVersion.get() >= 17) jvmArgs("--enable-native-access=ALL-UNNAMED")
}
tasks.withType<Javadoc>().configureEach {
    exclude("**/internal/**")
    (options as StandardJavadocDocletOptions).apply {
        encoding = "UTF-8"
        addBooleanOption("Werror", true)
    }
}
val integrationTest = tasks.register<JavaExec>("integrationTest") {
    dependsOn(tasks.testClasses)
    classpath = sourceSets.test.get().runtimeClasspath
    mainClass.set("io.multileveldb.IntegrationTest")
}
tasks.check { dependsOn(integrationTest) }
tasks.test {
    failOnNoDiscoveredTests = false
    dependsOn(integrationTest)
}
tasks.register<JavaExec>("interopTest") {
    dependsOn(tasks.testClasses)
    classpath = sourceSets.test.get().runtimeClasspath
    mainClass.set("io.multileveldb.InteropTest")
}
publishing {
    publications {
        create<MavenPublication>("library") {
            from(components["java"])
            pom {
                val repoUrl =
                    providers.environmentVariable("ML_REPO_URL").orElse("https://github.com/spechurkin/leveldb-lib")
                name.set("MultiLevelDB")
                description.set("Binary-safe LevelDB access through the multileveldb C ABI")
                url.set(repoUrl)
                licenses { license { name.set("0BSD"); url.set("https://opensource.org/license/0bsd") } }
                developers {
                    developer {
                        id.set("multileveldb")
                        name.set("MultiLevelDB contributors")
                        url.set(repoUrl)
                    }
                }
                scm {
                    val base = repoUrl.get()
                    url.set(base)
                    connection.set("scm:git:$base.git")
                    developerConnection.set("scm:git:$base.git")
                }
            }
        }
    }
    repositories {
        maven { name = "staging"; url = uri(layout.buildDirectory.dir("repository")) }
        if (System.getenv("ML_PUBLISH_GITHUB_PACKAGES") == "true") {
            maven {
                name = "GitHubPackages"
                url =
                    uri("https://maven.pkg.github.com/${(System.getenv("GITHUB_REPOSITORY") ?: "spechurkin/leveldb-lib").lowercase()}")
                credentials {
                    username = System.getenv("GITHUB_ACTOR") ?: ""
                    password = System.getenv("GITHUB_TOKEN") ?: ""
                }
            }
        }
    }
}
signing {
    val signingKey = System.getenv("GPG_SIGNING_KEY")
    val signingPassword = System.getenv("GPG_PASSPHRASE")
    isRequired = !signingKey.isNullOrBlank()
    if (!signingKey.isNullOrBlank()) {
        useInMemoryPgpKeys(signingKey, signingPassword ?: "")
        sign(publishing.publications["library"])
    }
}
tasks.jar { from("../../LICENSE") { into("META-INF") } }
tasks.register<Copy>("runtimeDependencies") {
    from(configurations.runtimeClasspath)
    into(layout.buildDirectory.dir("runtime"))
}
