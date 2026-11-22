# Compatibility

Check the platform and runtime requirements before choosing a native build.
If you're opening data written by another application, start with
[LevelDB data](#leveldb-data).

## Platform targets

The native implementation and build configuration target Windows x86_64/ARM64,
Linux x86_64/ARM64 and macOS x86_64/ARM64. All require 64-bit processes with the
matching architecture's shared library. JVM length mappings intentionally target
these 64-bit platforms; 32-bit JVMs are rejected before the library is loaded.
The JVM artifact is built with JDK 25 using `--release 8`, and requires Java 8+.
The build JDK and the consumer's runtime are independent.

## Language release policy

The supported stable release lines below were checked against upstream policies
on **2026-11-14**. Security-maintenance releases count as supported. Preview,
alpha, beta and release-candidate versions are excluded. Use the latest patch
release in each line. Vendor-specific extended support does not automatically
extend this project's validation policy.

- Python: **3.11, 3.12, 3.13, 3.14, 3.15**, using the same pure-Python wheel.
  The adapter's historical syntax floor remains 3.10; that line is upstream EOL.
- Node.js: **22, 24, 26**. JavaScript runs on Node; the package does not target
  browsers or other JavaScript engines. Node-API 8 allows one addon binary per
  OS/architecture to serve all these releases. Compatibility CI builds it on
  Node 22 and reuses it on every listed runtime without recompiling.
- TypeScript: **6.0 and 7.0**, with CommonJS and ESM declaration consumers.
  Node typings are checked against each supported Node major. TypeScript is a
  development dependency, not a runtime requirement of the package.
- Java: **8, 11, 17, 21, 25, 27**. One Java 8 JAR, including its JNA runtime
  dependency, is tested on each JVM. Native access is enabled only on Java 17+
  because the command-line option does not exist on Java 8/11. Cleanup uses
  phantom references rather than the Java 9+ Cleaner API.
- Kotlin/JVM: **2.4**, currently 2.4.21, consuming the Java 8 JAR. The Kotlin
  example is compiled and run in CI. Kotlin's supported JVM standard-library
  release lines follow its published security-support window.
- .NET, for both C# and F#: **10 and 11**. The NuGet package contains a
  `net10.0` assembly consumable by both releases. CI runs separate C# and F#
  consumers for each runtime. Library sources are compiled as C# 8.0 rather than
  inheriting a changing `latest` language setting.
- Windows .NET Framework **4.6.2 through 4.8.1**: a separate `net462` assembly
  uses the Windows loader and an explicit UTF-8 decoder. Supported Framework
  versions depend on the Windows lifecycle. CI compiles against 4.6.2 reference
  assemblies and runs on the runner's installed 4.x runtime; this does not
  certify execution on each individual historical Windows/Framework combination.
  The native DLL must be named `multileveldb.dll`. Framework 3.5 is not a target
  of this adapter.
- Ruby: **3.3, 3.4, 4.0**, with Fiddle. Older syntax compatibility is retained
  at Ruby 3.1, but EOL lines are outside the supported-release test matrix.
- Perl: **5.40, 5.42, 5.44**, with FFI::Platypus. The module retains its Perl
  5.20 syntax floor. Development releases with odd minor versions are excluded.
- Go: **1.26 and 1.27**, with cgo. The module's Go 1.20 source floor remains;
  CI uses `GOTOOLCHAIN=local` so automatic toolchain downloads cannot conceal
  failure on the selected compiler.
- Rust: current **stable**, plus the declared **1.70.0 MSRV** as an additional
  backward-compatibility check. Rust has no parallel LTS release lines. The
  version-3 Cargo lockfile remains readable by the minimum Cargo version.
- C consumers: **C99, C11, C17, C23**; C++ consumers: **C++11, C++14, C++17,
  C++20, C++23**. ISO language standards have no runtime support lifecycle.
  Installed-header consumers are built and executed with strict standard modes.
  Building the native implementation still needs C++17 or newer; CMake honors
  caller-selected newer standards instead of resetting them to C++17.

`.github/workflows/compatibility.yml` checks these release lines on x86_64,
independently of the six-platform matrix in `build.yml`. Python and Ruby checks
install their built packages before testing. Adding a workflow is not evidence
that it has run: see [VALIDATION.md](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/VALIDATION.md) for local execution results.
Recheck this list and its matrices when upstream releases enter or leave support.

## Platform validation and paths

The CI matrix builds and tests native C/C++, Rust, Node and JVM on all six
targets, with Python and .NET (C#/F#). Perl, Ruby and the sequential language chain are enabled on Linux x86_64, Windows
x86_64 and both macOS targets. The setup-perl action does not list Linux ARM64
or Windows ARM64 among its supported runner images, so those Perl runs are
explicitly excluded. A native Perl plus FFI::Platypus toolchain for those ARM64
systems can use the same module without modifying the core. This is a validation
and distribution-toolchain gap, not a claim of tested Perl support there.

Go/cgo runs on the four Unix CI targets. Windows Go requires a compatible MinGW
native import library; the MSVC matrix build does not supply one, so Windows CI
explicitly skips Go. Windows x86_64 MinGW Go was exercised locally. Ruby follows
the same runner subset as Perl; no ARM64 Windows/Linux Ruby certification is
claimed. Managed source floors are Python 3.10+, Ruby 3.1+ with Fiddle,
.NET 10+ or Windows Framework 4.6.2+ for C#/F#, and Go 1.20+ with cgo. Their output memory limits
follow byte-array/slice capacity and native size_t, with returned native buffers
freed even if copying fails.

Only **Windows x86_64 with MinGW** was available for local execution during
initial development. The other matrix entries and sanitizer job have been
prepared but not run in this workspace. Read VALIDATION.md for exact evidence.
CI configuration alone is not proof of platform support.

Windows paths are strict UTF-8 at the ABI and UTF-16 at filesystem calls. Tests
include Cyrillic, CJK and supplementary-plane characters. Invalid UTF-8 and NUL
paths are rejected. Windows long-path behavior remains subject to the process's
manifest/system policy and Win32 filesystem constraints. No lossy ANSI fallback
is used. On POSIX, API 1.0 accepts UTF-8 paths; non-UTF-8 filesystem names are not
exposed by this API.

## LevelDB data

MultiLevelDB opens standard Google LevelDB 1.23 databases that use the
default bytewise comparator, including databases with Snappy-compressed
tables. The native integration test writes and compacts a database with
Google LevelDB, opens and writes to it through the C ABI, then reopens it with
Google LevelDB.

Compatibility covers that format and comparator. RocksDB, encrypted
forks and databases that rely on browser-specific extensions or custom
comparators are outside that scope.

## Engine limits

- Each open database holds a directory lock. Close it and its cursors and
  snapshots before opening the same directory from another process.
- Opening a database always uses the standard LevelDB mode. There is no
  read-only option.
- A batch applies its writes atomically. It offers neither conflict detection
  nor transactional reads. A snapshot holds a consistent read view of the
  live database.
- Iterators stream data and follow the engine comparator. They can retain old
  versions; snapshots can increase disk use while held.
- Non-sync writes retain LevelDB's usual crash durability semantics. Use sync
  write options when the application requires the engine's synchronous flush.
- Key/value limits follow engine and platform capacities. JVM byte[] and Node
  Buffer capacities are language/runtime limits. No artificial small record
  limit is introduced by the ABI.
- Filesystem I/O errors and corruption retain engine diagnostics. Not every
  operating system's lock error string can be classified precisely; unknown
  cases remain IO_ERROR.

## Upstream references

- [Python release status](https://devguide.python.org/versions/)
- [Node.js release schedule](https://github.com/nodejs/Release)
- [Java support roadmap](https://www.oracle.com/java/technologies/java-se-support-roadmap.html)
- [Kotlin standard-library security support](https://kotlinlang.org/docs/releases.html#standard-library-security-support)
- [.NET support policy](https://dotnet.microsoft.com/en-us/platform/support/policy/dotnet-core)
- [.NET Framework support policy](https://dotnet.microsoft.com/en-us/platform/support/policy/dotnet-framework)
- [Ruby maintenance branches](https://www.ruby-lang.org/en/downloads/branches/)
- [Perl maintenance policy](https://perldoc.perl.org/perlpolicy#MAINTENANCE-AND-SUPPORT)
- [Go release policy](https://go.dev/doc/devel/release#policy)
- [TypeScript release notes](https://www.typescriptlang.org/docs/handbook/release-notes/overview.html)

- [Google LevelDB](https://github.com/google/leveldb/tree/1.23)
- [Google Snappy releases](https://github.com/google/snappy/releases)
- [Node-API async work](https://nodejs.org/api/n-api.html#asynchronous-operations)
- [JNA](https://github.com/java-native-access/jna)
- [FFI::Platypus](https://metacpan.org/pod/FFI::Platypus)
- [GitHub runner labels](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
- [setup-perl supported platforms](https://github.com/shogo82148/actions-setup-perl#supported-platforms)
