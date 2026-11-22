# Building, testing and distribution

Build the native library first, set up your platform's loader and then build
the binding you need. Run the commands from the repository root unless a
step tells you to change directories.

## Toolchains

| What you are building | Tools |
|---|---|
| Native library | CMake 3.20+, a C++17 compiler and Git |
| Rust | Stable Rust and Cargo |
| JVM | JDK 25 and Gradle 9.9.1. The resulting JAR runs on Java 8+. |
| Node.js / TypeScript | Node.js 22+ and npm. The development lockfile pins TypeScript 7.0.2 and `@types/node` 26.6.6. |
| Perl | Perl 5.20+, FFI::Platypus 2.11+ and the make program your Perl uses |
| C# and F# | .NET SDK 10+. The package also targets Windows .NET Framework 4.6.2+. |
| Python | Python 3.10+ for the binding. Use Python 3.12+ for the download and integration tools. |
| Ruby | Ruby 3.1+ and Fiddle 1.1+ |
| Go | Go 1.20+, cgo and a matching C compiler |

See the [compatibility policy](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/COMPATIBILITY.md#language-release-policy) for
the runtime releases covered by CI.

Pinned engine dependencies:

- Google LevelDB 1.23: `99b3c03b3284f5886f9ef9a4ef703d57373e61be`.
- Google Snappy 1.3.1: `9c28114a38866f6deeaa826db918293bc28ae410`.
- JVM JNA 5.19.1.

The CMake build fetches source commits. Distributions statically include the
engine and Snappy behind one shared multileveldb library. Dependency test
programs and LevelDB's utility are excluded from the default build/install.

## Native

These commands use Ninja. For an offline configure, pass
`-DFETCHCONTENT_SOURCE_DIR_LEVELDB=<path>` and
`-DFETCHCONTENT_SOURCE_DIR_SNAPPY=<path>` with local checkouts at the pinned commits.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix build/install
```

For Visual Studio use an appropriate developer command prompt, or the generator
with `-A x64` or `-A ARM64`. Pass `--config Release` to the build and install
commands, and `-C Release` to CTest.
For MinGW, put its compiler, CMake, make and runtime DLL directories on PATH,
then use `-G "MinGW Makefiles"`. Match Cargo's GNU target to a MinGW build and
MSVC target to an MSVC build. Do not mix incompatible import-library formats.

The source header is `native/include/multileveldb.h`. CMake installs it as
`<prefix>/include/multileveldb.h`. Downstream C/C++ projects can use
`find_package(multileveldb CONFIG REQUIRED)` and link
`multileveldb::multileveldb`. No LevelDB headers are required by consumers.

## Loader configuration

Set these variables in the shell where you build and run the bindings.
`ML_NATIVE_DIR` gives Cargo the directory to link against.
`MULTILEVELDB_LIBRARY` selects the shared library for JVM, Perl, .NET, Python
and Ruby. Go uses its own cgo flags, and a prepared Node package locates
its native files next to the addon.

For a single-configuration build on Linux, run the following.

```sh
export ML_NATIVE_DIR="$PWD/build/native"
export MULTILEVELDB_LIBRARY="$ML_NATIVE_DIR/libmultileveldb.so"
export LD_LIBRARY_PATH="$ML_NATIVE_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export ML_TEST_ROOT="$PWD/build"
```

On macOS, use `libmultileveldb.dylib` and `DYLD_LIBRARY_PATH`. In Windows PowerShell, run the following.

```powershell
$env:ML_NATIVE_DIR = "$PWD\build\native"
$env:MULTILEVELDB_LIBRARY = "$env:ML_NATIVE_DIR\multileveldb.dll"
$env:PATH = "$env:ML_NATIVE_DIR;$env:PATH"
$env:ML_TEST_ROOT = "$PWD\build"
```

Both MSVC and MinGW produce `multileveldb.dll`; for MinGW also add its compiler runtime directory to
PATH. For multi-configuration generators add the configuration subdirectory.
For an installed archive use its lib/bin directories instead.

## Rust

```sh
cargo test --workspace --locked
cargo package -p multileveldb --allow-dirty
```

Set ML_NATIVE_DIR before building to locate the import/shared library. Cargo
does not compile C++ as a side effect. A prebuilt native archive can satisfy the
dependency without any compiler on the consumer's machine. The source crate
has no dependency on the other bindings.

## JVM

```sh
gradle -p bindings/jvm build
gradle -p bindings/jvm publishLibraryPublicationToStagingRepository
```

The second command stages a local Maven repository under
`bindings/jvm/build/repository`. The build produces
the main, sources and Javadoc jars. `test`/`check` invoke the plain Java
integration test, which is executed with assertions enabled. JNA is resolved
from Maven Central. The JVM uses MULTILEVELDB_LIBRARY, the explicit
`multileveldb.library` property, or JNA's standard library search.

To run the integration test on a separately installed Java 8 runtime, use
`gradle -p bindings/jvm integrationTest -PtestJavaVersion=8`. Run Gradle itself
with JDK 25. If Gradle cannot find the test JVM, add its home directory to
`org.gradle.java.installations.paths`.

If Gradle cannot connect to its daemon, compile the
main/test Java sources with `javac -encoding UTF-8 -cp <jna.jar>` and run
`java -ea -cp <classes>:<jna.jar> io.multileveldb.IntegrationTest` (use `;` on
Windows). That verifies Java/JNA behavior but does not verify Gradle packaging.
On Windows, a long sandbox TEMP path can also make JDK Unix-domain socket
creation fail with `PipeImpl` / `Invalid argument: connect`. A short writable
directory supplied through `JAVA_TOOL_OPTIONS` properties `java.io.tmpdir` and
`jdk.net.unixdomain.tmpdir` allows the standard Gradle daemon/worker connection
to use shorter socket paths without changing machine-wide settings.

## Node.js

Run `python tools/node_headers.py` to fetch the current runtime's headers and,
on Windows, its import library. The script verifies Node's SHA-256 manifest.
Replace the placeholders in the configure command with the printed paths.
Omit `NODE_LIBRARY` on Unix.

```sh
cmake -S . -B build -DML_BUILD_NODE=ON -DNODE_INCLUDE_DIR=<include> -DNODE_LIBRARY=<node.lib>
cmake --build build --parallel
npm ci --prefix bindings/node
npm test --prefix bindings/node
npm run typecheck --prefix bindings/node
python tools/package_node.py
cd bindings/node
npm pack
```

MinGW generates a GNU import library from
`bindings/node/src/native/node.def`. The addon uses the stable Node-API. Blocking engine work runs
in Node-API async workers. CommonJS and ESM are tested separately.

The packaging helper stages the addon, shared library and license notices under
`prebuilds/<platform>-<arch>`. Collect all six platform directories in one package
before a universal npm release. Keep the companion library adjacent to the addon.
Platform C/C++ runtime dependencies must also be available: system libc/libstdc++
on Linux, the Apple runtime on macOS, MSVC redistributables for MSVC Windows
builds, and matching GCC runtime DLLs for MinGW development builds.

## Perl

```sh
cd bindings/perl
perl Makefile.PL
make test
make dist
```

Use the make program from Perl's `%Config` (gmake on Strawberry). Set
MULTILEVELDB_LIBRARY first. `make dist` packages the Perl module, tests and
license. It does not bundle the engine; install a matching native archive.

## Cross-language tests

With all tools and adapter outputs available:

```sh
python tests/integration/run.py
python tests/abi/check_exports.py
```

The integration driver passes one database directory from process to process.
Rust writes the fixture. The JVM reads it, Node reads and updates it, and the
remaining bindings run in turn. C checks the final fixture and the marker
records at the end. Each process closes its handles before the next opens
the database. Override
ML_CARGO (for example `cargo +stable-x86_64-pc-windows-gnu`), ML_GRADLE, ML_JAVA
or ML_JAVA_CLASSPATH when needed. Set ML_BUILD_DIR for a nonstandard native
build directory. The JVM classpath override permits direct javac output.

Set ML_DOTNET, ML_RUBY and ML_GO to alternate executable paths if needed. The
driver sets Python's source import path. `ML_SKIP_GO=1` explicitly excludes Go
from this chain where no compatible cgo toolchain exists; no implicit skip is
performed. Python/.NET run in the six-platform CI matrix; Ruby follows the
existing Perl runner subset. Go runs on the four Unix matrix entries; Windows
MinGW is covered by local validation.

## C#, F#, Python, Ruby and Go

Build/install the native library first, then use these full setup/API guides:

- [.NET / C# and F#](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/dotnet/README.md): `dotnet build bindings/dotnet -c Release`,
  `dotnet run --project tests/dotnet -c Release` and
  `dotnet run --project tests/fsharp -c Release`. `dotnet pack` stages NuGet
  with the assembly and XML documentation.
- [Python](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/python/README.md): set PYTHONPATH to bindings/python,
  then `python -m unittest discover -s bindings/python/tests -v`.
  `python -m pip wheel --no-deps ./bindings/python -w build/packages` builds a
  wheel; `python -m build bindings/python` also makes an sdist with build installed.
- [Ruby](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/ruby/README.md): `ruby -I bindings/ruby/lib bindings/ruby/test/database_test.rb`;
  run `gem build multileveldb.gemspec` inside bindings/ruby for a local gem.
- [Go](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/go/README.md): set CGO_ENABLED=1 and CGO_CFLAGS/CGO_LDFLAGS to
  include/library paths, then `go -C bindings/go test ./...` and
  `go -C bindings/go vet ./...`. Windows needs a matching MinGW import library.

C# and F# test projects also accept `-p:TestTargetFramework=net11.0` or, on
Windows, `net462`. For `net462`, run `dotnet build` and then run the generated
EXE with Windows .NET Framework. The build restores reference assemblies
from NuGet.

These commands build and test local packages. Install the native library
separately to use them. On Windows, Python and Ruby may need the compiler
runtime DLLs alongside the native library. Python can also register a separate
runtime directory with `os.add_dll_directory`. If temporary file creation fails
in a Windows sandbox, set `TMP`, `TEMP` and `GOTMPDIR` to a short, writable
directory inside the build tree.

For native memory instrumentation on a supporting Unix compiler, configure
both C and C++ flags with `-fsanitize=address,undefined -fno-omit-frame-pointer`,
build Debug, and run CTest. CI includes this separate job.

## Publication

The [build workflow](https://github.com/spechurkin/leveldb-lib/actions/workflows/build.yml) and
[compatibility workflow](https://github.com/spechurkin/leveldb-lib/actions/workflows/compatibility.yml) build and
test packages. The [release workflow](https://github.com/spechurkin/leveldb-lib/actions/workflows/release.yml)
handles publishing with GitHub's built-in `GITHUB_TOKEN`.

Run `release-github-packages` manually with `dry_run: true` (the default) to
check version alignment, build all six native platforms, test the packages and
upload the staged files as workflow artifacts. A matching version tag such as
`v1.0.0` triggers publishing. Setting `dry_run: false` on a manual run also
selects publishing.

| Ecosystem         | Release destination                                                             | Authentication                                                     |
| ----------------- | ------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| JVM (Java/Kotlin) | GitHub Packages Maven (`https://maven.pkg.github.com/spechurkin/leveldb-lib`)   | Built-in `GITHUB_TOKEN` (`packages: write`)                        |
| Node.js / TS      | GitHub Packages npm (`https://npm.pkg.github.com/`, `@spechurkin/multileveldb`) | Built-in `GITHUB_TOKEN` (`packages: write`)                        |
| .NET (C#/F#)      | GitHub Packages NuGet (`https://nuget.pkg.github.com/spechurkin/index.json`)    | Built-in `GITHUB_TOKEN` (`packages: write`)                        |
| Ruby              | GitHub Packages RubyGems (`https://rubygems.pkg.github.com/spechurkin`)         | Built-in `GITHUB_TOKEN` (`packages: write`)                        |
| Native C/C++      | GitHub Release (`6` archives + `SHA256SUMS.txt` + SLSA attestation)             | Built-in `GITHUB_TOKEN` (`contents: write`, `attestations: write`) |
| Rust              | GitHub Release (`multileveldb-<version>.crate`)                                 | Built-in `GITHUB_TOKEN` (`contents: write`)                        |
| Python            | GitHub Release (`.whl` + sdist `.tar.gz`)                                       | Built-in `GITHUB_TOKEN` (`contents: write`)                        |
| Perl              | GitHub Release (`MultiLevelDB-<version>.tar.gz`)                                | Built-in `GITHUB_TOKEN` (`contents: write`)                        |
| Go                | Git subdirectory tag `bindings/go/vX.Y.Z`                                       | Built-in `GITHUB_TOKEN` (`contents: write`)                        |

### Manual release checklist

1. Require successful native and adapter jobs on each supported target; review
   the platform exclusions in the [compatibility policy](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/COMPATIBILITY.md).
2. Run `cpack -G ZIP` (or TGZ) in the native build directory. Inspect the archive:
   only shared/import libraries, headers, CMake metadata and notices are installed.
3. Test a consumer against the installed prefix, not just the build tree.
4. Use `cargo package`, then `cargo publish` with the registry credential supplied
   by the release operator. Publish the matching native artifacts alongside it.
5. Stage the JVM Maven artifacts with the local Gradle task above. Sign the
   repository bundle and upload it through the Maven Central Portal API
   (`central.sonatype.com`).
   JNA remains a normal Maven dependency.
6. Merge all prepared Node prebuild directories, run `npm pack --dry-run`, then
   `npm publish --provenance --access public` with the release operator's npm identity.
7. Run Perl `make disttest`, then upload the resulting distribution through PAUSE.
8. Preserve LevelDB and Snappy BSD notices in all binary packages. JNA's own
   dual license applies to JNA artifacts; compiler runtimes have their own terms.

Bump distribution SemVer independently from API/ABI versions. Preserve the
frozen ABI symbol baseline. Adding a language does not change ABI major/minor.
