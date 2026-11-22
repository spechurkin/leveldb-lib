# Local validation

Development date: 2026-11-14. Host: Windows x86_64. This record describes actual
local execution, not the outcome of a remote CI run.

## Tool versions used

- CMake 3.29, GCC/MinGW 13.2.0.
- Rust stable 1.100.0, x86_64-pc-windows-gnu.
- Node.js 26.13.0, Node-API 8 binding, TypeScript 7.0.2.
- Corretto JDK 21.0.8 for initial direct checks; JDK 25.0.5 for the final
  configured JVM artifacts, with JNA 5.19.1 and Gradle 9.9.1.
- Perl 5.42.3, FFI::Platypus 2.11.
- Google LevelDB 1.23 and Snappy 1.3.1 at the pinned commits.

## Executed checks

- CTest: public C integration, official-engine C++ compatibility, invalid ABI
  arguments/error mapping, and independent-process directory locking.
- Binary empty keys/values, embedded NUL, bytes 128/255, repeated operations,
  large values, deletion and reopening.
- All opening settings and separate read/write options, including sync writes.
- Iterator first/last/seek, both directions, validity, key/value copies and
  end-of-scan status; invalid cursor movement returns an error safely.
- Atomic batches, clear/delete, snapshots and snapshot iterators surviving
  release of the original snapshot handle.
- Engine lifetime retained by an iterator after DB close/destroy. Another process
  remains locked out until the iterator releases the engine.
- Four threads write through a shared native database; scoped Rust threads use
  the safe shared Database API.
- Cyrillic paths in every adapter, plus CJK and supplementary Unicode in native
  tests. Invalid UTF-8 and engine length overflows are rejected.
- Official LevelDB creates and compacts a Snappy database; the ABI reads it and
  writes data which the official engine subsequently reads.
- Rust adapter integration tests and `cargo package` verification.
- JVM integration tests with assertions; Gradle build and local Maven repository
  staging, including sources/Javadoc jars.
- Node integration test, CommonJS/ESM exports, queued close, and TypeScript checks
  against both module forms.
- Perl integration assertions, ExtUtils::MakeMaker test harness and `make dist`.
- Sequential shared-fixture chain: Rust write, JVM read, Node modify, Perl read,
  C verify, with each process closing the directory before the next opens it.
- Frozen ABI symbol list loaded through the real Windows loader.
- Native CMake installation and separate C/C++ consumers using find_package.
- Native ZIP, npm package, verified Rust crate and Perl distribution packaging.
- Doxygen generation with missing-member/parameter warnings treated as errors;
  rustdoc with warnings denied and a compiled doctest; Javadoc with Werror;
  generated JSDoc and parsed/rendered Perl POD.
- Documentation coverage check for all 46 Node declaration items and inherited
  close methods on every exported resource reference.
- Java and Kotlin examples compiled and executed against the JVM jar.
- After extracting the JVM's nested public types into separate files: Gradle
  `build` passed, including integration assertions and Javadoc with Werror.
  All 19 public top-level types were verified in the main, sources and Javadoc
  jars, with no stale nested API classes. Java and Kotlin examples were compiled
  and executed against the rebuilt main jar.

JDK local communication initially failed because AF_UNIX socket creation used
the sandbox's long temporary path. Supplying a short build-local temporary
directory via java.io.tmpdir/jdk.net.unixdomain.tmpdir allowed the normal Gradle
daemon and worker build to finish. No machine-wide configuration was changed.

## Explicit database creation (API 1.1)

- Rebuilt the native library and Node addon; all four CTest checks passed.
- Creation checks passed for Rust, JVM, Node.js and Perl, and two focused Python
  unittest cases passed. Checks cover default/custom settings, an empty new
  database, binary writes and reopening, rejection of existing/locked databases,
  preservation of stored values and caller settings, and invalid paths.
- Node TypeScript checks, generated JSDoc and all 46 declaration documentation
  checks passed. Rust doctest and rustdoc with warnings denied passed; the
  doctest used the writable `build/tmp` directory for temporary output.
- JVM sources compiled with JDK 25.0.5; integration tests with assertions and
  Javadoc with Werror passed directly. The Gradle invocation could not connect
  to its daemon in this sandbox, so this check does not certify Gradle packaging.
- Python used the build DLL and explicitly registered the MinGW runtime DLL
  directory with `os.add_dll_directory` in the test launcher on Windows.
- The loader check passed for all frozen ABI 1.0 exports and the new ABI 1.1
  `ml_db_create` export. Perl POD syntax checking passed.

## Additional adapters (2026-11-14)

The following checks were executed on the same Windows x86_64 MinGW host:

- .NET SDK 10.0.404: C# and F# integration executables, including explicit create,
  binary/empty/missing data, builders, batches, native snapshots, cursors and
  deterministic release. C# additionally exercised shared threads, foreign
  snapshots and a 1 MiB value. Build had zero warnings/errors with XML
  documentation enabled. NuGet package contents include DLL, XML, README, LICENSE.
- Python 3.15.2: all six unittest cases passed, including the existing creation
  tests, binary/options/lifetimes, fallback finalization and shared threads.
  CTest now runs that suite as well: all five native/Python CTest entries passed.
  The pure-Python wheel was built and inspected; pydoc HTML was generated and
  public method docstrings checked. MinGW runtime DLLs were staged next to the
  build DLL so a normal Python import/test launcher could load it.
- Ruby 4.0.9 with Fiddle 1.1.8: real-engine adapter tests, explicit create,
  binary/options/batch/snapshot/cursor/reopen/closed/foreign-view assertions,
  fallback GC cleanup and gem packaging. RDoc 7.0.4 generated HTML with all
  33 public classes/modules/attributes/methods documented (100%).
- Go 1.27.4 with GCC 13.2/cgo: adapter tests and go vet passed. Coverage included
  options, binary/empty/missing data, 1 MiB data, batches, snapshots, cursor
  retention, foreign views, shared goroutines, zero-value resources and Create
  without mutation of caller settings. Go doc generated the public reference.
  TMP/TEMP/GOTMPDIR used build/tmp to avoid sandbox temp-directory file denial.
- The full sequential fixture chain passed: Rust -> JVM -> Node.js -> Perl ->
  Python -> Ruby -> C# -> F# -> Go -> C. C verified each new adapter's binary
  sentinel as well as the shared fixture after Node's modification.
- Added the missing top-level JVM Options resource base required by the current
  option builders; JDK 25 compiled the adapters for the complete fixture chain.
  The standalone JVM integration executable and strict Javadoc also passed.
- All five new runnable language examples compiled/executed against the same
  example database. Installed Python wheel and Ruby gem consumers completed
  native create/put/get/close round trips. Documentation links and workflow YAML
  parsed successfully.

## Binding module split (2026-11-14)

After separating the .NET, Rust, Node.js, Python, Ruby, Perl and Go bindings into
resource, option, error and native modules:

- All existing adapter suites passed, including C# and F# executables and the
  JVM integration executable. The rebuilt Node addon used its extracted worker.
  All five CTest checks passed, including Python and native ABI/locking tests.
- The complete sequential fixture chain passed: Rust -> JVM -> Node.js -> Perl
  -> Python -> Ruby -> C# -> F# -> Go -> C.
- Rust tests and the doctest passed without compiler warnings; strict rustdoc
  passed. Node CommonJS/ESM tests, TypeScript checks and generated JSDoc passed;
  all 46 declaration items and inherited close methods remain documented.
  Python pydoc exposes every public class; Ruby RDoc documents all 33 public
  items (100%). Perl POD checking and Go vet passed.
- .NET assemblies built for net462 and net10.0 with zero warnings/errors. The
  NuGet archive contains both framework assemblies and their XML documentation.
  The Rust crate passed cargo package verification. Python wheel, npm archive,
  Ruby gem and Perl distribution contents include the extracted modules.
  Extracted Python/Ruby/Perl consumers passed their adapter suites; an extracted
  npm consumer passed CommonJS/ESM binary create/put/get/close and TypeScript
  checks against the packaged declarations.
- Python AST comparisons and Node source comparisons confirmed that resource
  method bodies were preserved. The native Node resource, job and worker bodies
  also match their original source.

The local Perl distribution build used Windows tar and a Python gzip helper
because the installed MSYS tools could not create their IPC objects in the
sandbox. F# restore used cached packages with NuGet auditing disabled for that
command because the vulnerability feed was unavailable. No repository setting
was changed for either local workaround.

## JVM, Go and Node directory grouping (2026-11-14)

- JVM now uses real `io.multileveldb.options`, `io.multileveldb.errors` and
  `io.multileveldb.internal` packages. Core database/resource classes remain in
  `io.multileveldb`. Tests, examples and API documentation use the new imports;
  callers importing moved types must update imports and recompile.
- Gradle `clean build publish` passed, including integration assertions,
  Javadoc with Werror, main/sources/Javadoc jars and the local Maven staging
  repository. Archive checks confirmed the new class/source paths, absence of
  obsolete root-package classes, and exclusion of internal implementation
  classes from the public Javadoc. Java and Kotlin examples compiled and ran
  against the resulting main jar.
- Go now has `internal/database`, `internal/options` and `internal/errors`.
  Root type aliases/factories preserve the module import. `go test ./...` and
  `go vet ./...` passed. A separate consumer module exercised binary data,
  batches, snapshots, cursors, zero-value resources and errors.As via that public
  import. CI documentation generation also captures the internal type references.
- Node resources/declarations moved to `src/resources`, and native sources and
  exports to `src/native`. The rebuilt addon passed the existing tests;
  TypeScript and all 46 declaration/JSDoc checks passed. The npm archive passed
  extracted CommonJS/ESM binary round trips and packaged TypeScript checks.
- The ten-language sequential fixture chain passed after the package changes.

Gradle required local IPC access outside the sandbox. Its child JVMs used the
repository's `build/tmp` directory for temporary files. Publication was only to
the repository-local staging directory; no registry upload was performed.

## Platform certification limits

Linux, macOS, all ARM64 targets, MSVC builds and the AddressSanitizer/UBSan job
were not executed on this Windows host. The CI matrix prepares those builds;
their successful results are required before certifying a cross-platform release.
Perl ARM64 coverage is explicitly excluded where setup-perl has no supported
runner/toolchain configuration. No packages have been published to registries.

Local Windows packages use MinGW and require its compiler runtime DLLs on PATH.
They are development artifacts; the CI Windows release configuration uses MSVC.

## Language version compatibility (2026-11-14)

Executed on Windows x86_64 with the real native engine:

- One Gradle-built JAR passed integration assertions on Corretto Java
  8u412, 11.0.27, 17.0.18, 21.0.8 and 25.0.5. Every class in the main JAR
  was inspected and has class-file version 52 (Java 8). Tests include explicit
  double-close, closed-resource access and phantom-reference fallback cleanup.
  The Java example also compiled and ran using Java 8.
- Gradle 9.9.1 `build publish runtimeDependencies` passed, including strict
  Javadoc and staging Maven publication. Gradle required execution outside
  the sandbox to connect to its own local daemon.
- The Kotlin 2.4.21 example built and ran through Gradle using the Java 8 JAR.
- Node.js 22.23.5, 24.22.3 and 26.13.0 passed both integration cases. The
  Node 22/24 runs reused the existing Node-API 8 addon, without recompilation.
  The local addon was built with Node 26 headers; CI separately builds with
  Node 22 headers before testing the same binary across all three releases.
  The existing TypeScript 7.0.2 check passed.
- C# and F# integration programs passed on .NET 10.0.15 and the installed
  Windows .NET Framework 4.x runtime. Framework consumers compiled against
  the 4.6.2 reference assemblies. Both library targets built without warnings
  or errors. The NuGet package was created and inspected for
  `lib/net462/MultiLevelDB.Managed.dll` and `lib/net10.0/MultiLevelDB.dll`.
  The Framework assembly name avoids a Windows native-loader collision with
  `multileveldb.dll`. MinGW and MSVC now use that same native filename.
- Python 3.11.14, 3.14 and 3.15.2 each passed all six adapter unittests. Ruby 4.0.9 and
  Go 1.27.4 passed their native adapter integration checks.
- Rust 1.70.0 (GNU target) and stable 1.100.0 passed workspace tests and the
  compiled doctest using the version-3 Cargo lockfile. The minimum toolchain
  was installed only under the ignored `build/compat-rustup` directory.
- All five native/Python CTest checks passed after rebuilding the DLL/addon.
  Installed C/C++ consumers compiled with strict standard modes and ran in
  five configurations: C99/C++11, C11/C++14, C17/C++17, C23/C++20 and C23/C++23.
- actionlint 1.7.12 accepted both GitHub Actions workflows with no diagnostics.
- A separate native implementation build with `CMAKE_CXX_STANDARD=23` passed
  all five CTest checks. Its Python consumers needed matching MinGW runtime
  DLLs staged beside the shared library, as documented for Windows builds.
  The UTF-8 integration fixture now also compiles with C++20/23 `char8_t`.
- Gradle's `-PtestJavaVersion=8` launcher selection passed integration tests
  while Gradle itself ran on JDK 25.

The new compatibility workflow has not been executed remotely. Python 3.12–3.13,
Java 27, .NET 11, TypeScript 6, older supported Ruby/Perl releases and Go 1.26
remain CI checks, not locally certified combinations. The Windows runner's
installed 4.x runtime does not prove execution on a machine with exactly
Framework 4.6.2. Platform/architecture gaps documented above still apply.

## Release 1.0.0 validation (2026-11-22)

Host: Windows x86_64. Toolchains used:

- CMake 3.29.2, Ninja, GCC/MinGW 13.2.0 (Strawberry) and MSVC 19.44.35230.0 (VS 2022 BuildTools).
- Rust `1.100.0` (`stable-x86_64-pc-windows-gnu` and `1.100.0-x86_64-pc-windows-msvc`), Cargo lockfile v3.
- Node.js `v26.13.1`, TypeScript `7.0.2`, JSDoc `4.0.5`.
- Corretto JDK `8u412`, `11.0.27` (installed), `21.0.8` and `25.0.5.1`; Gradle `9.9.1`, JNA `5.19.1`, Kotlin `2.4.21`.
- .NET SDK `10.0.404` (`net10.0` and `net462` targets).
- Python `3.15.2`, `uv 0.13.9`.
- Ruby `4.0.9` with Fiddle `1.1.8` and RDoc `7.0.4`.
- Perl `5.42.3` with `FFI::Platypus`.
- Go `1.24.5` (`windows/amd64`, cgo with MinGW GCC 13.2.0).
- Doxygen (from Strawberry toolchain).

Executed checks for version `1.0.0` (API `1.1.0`, ABI `1.1`):

- Native MinGW Release (`build/`): all 5 CTest suites (`process_lock`, `python_binding`, `abi_contract`,
  `c_integration`, `cpp_integration`) passed after staging MinGW runtime DLLs next to `build/native/multileveldb.dll`;
  `tests/abi/check_exports.py` passed; installed C/C++ consumer (`tests/consumer`) built and passed 2/2 CTest checks;
  `cpack -G ZIP` produced `multileveldb-1.0.0-win64.zip`.
- Native MSVC Release (`build-msvc/`, subsequently removed): all 5 CTest suites (`process_lock`, `python_binding`,
  `abi_contract`, `c_integration`, `cpp_integration`) and `tests/abi/check_exports.py` passed with
  `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR`; `cargo +1.100.0-x86_64-pc-windows-msvc test --workspace --locked` passed;
  Corretto JDK 21 (`msvcp140.dll 14.29.30157.0`) passed `io.multileveldb.IntegrationTest` against the MSVC DLL.
- Rust GNU: `cargo +stable-x86_64-pc-windows-gnu test --workspace --locked` (database, interop, doctest) passed;
  `cargo package -p multileveldb --allow-dirty --locked` bundled 17 files including `bindings/rust/LICENSE`; `rustdoc`
  with `-D warnings` passed.
- JVM: `gradlew.bat -p bindings/jvm clean build publish runtimeDependencies` passed (`multileveldb-1.0.0.jar`, sources,
  Javadoc with `-Werror`, local Maven staging); the same JAR passed `io.multileveldb.IntegrationTest` on Corretto Java 8
  and Java 21; `examples/kotlin` ran via Gradle and passed.
- Node.js / TypeScript: `npm ci`, `npm test` (2/2), `npm run typecheck`, `npm run docs`,
  `node tools/check_node_docs.cjs` (46 API items + `close()`), `python tools/package_node.py`, and `npm pack`
  (`multileveldb-1.0.0.tgz`, 27 files) all passed.
- .NET (C# and F#): `dotnet build bindings/dotnet -c Release` passed with 0 warnings/errors; C# and F# integration
  suites (including explicit F# `Delete` checks) passed on both `net10.0` and `net462`; `dotnet pack` produced
  `MultiLevelDB.1.0.0.nupkg` containing both assemblies, XML docs, `README.md` and `LICENSE`.
- Python: 6/6 unittest cases passed; `uv build --wheel` produced clean `multileveldb-1.0.0-py3-none-any.whl` (verified
  no stale top-level `multileveldb.py` and `LICENSE` included); `pydoc -w` generated HTML.
- Ruby: `database_test.rb` passed; `gem build` produced `multileveldb-1.0.0.gem`; `rdoc` reported 100.00% documented (33
  items).
- Perl: `perl Makefile.PL`, `gmake test` (19/19), `podchecker`, `Pod::Simple::HTML`, and `gmake dist`
  (`MultiLevelDB-1.0.0.tar.gz`) passed.
- Go: `go -C bindings/go test -v ./...` (`TestDatabase` and `TestInvalidAndZeroHandles` passed; `TestInterop` skipped
  outside interop driver), `go vet ./...`, `go doc -all .`, and `examples/go` passed.
- Sequential 10-language integration (`tests/integration/run.py`):
  `Rust -> JVM -> Node.js -> Perl -> Python -> Ruby -> C# -> F# -> Go -> C: passed`.
- Doxygen (`native/Doxyfile` with `WARN_AS_ERROR = YES`): passed.

Remaining certification limits: Linux, macOS, ARM64 targets, ASan/UBSan, Go 1.26/1.27, TypeScript 6, Python 3.11–3.14,
.NET 11, Java 27, and Rust 1.70 MSRV were not re-executed on this Windows host in this 1.0.0 pass and remain CI matrix
targets.
