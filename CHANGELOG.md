# Changelog

## 1.0.0 - 2026-11-22

- Stabilize distribution version 1.0.0 across native C/C++, Rust, JVM, Node.js/TypeScript,
  Perl, .NET (C#/F#), Python, Ruby and Go packages, examples and documentation.
- Fix Cargo package/interop crate name (`multileveldb`) in CI and integration runner,
  bundle `LICENSE` in Rust and Python packages, clean Node prebuild staging, and
  register the native DLL directory on Windows in Python's ctypes loader.
- Define `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` for MSVC builds so `multileveldb.dll`
  remains compatible with Windows host processes bundling older `msvcp140.dll` runtimes.
- Target Java 8 bytecode with Java 8-compatible fallback cleanup and examples;
  test one JAR on supported Java LTS releases and Java 27.
- Add `net462` and `net10.0` NuGet assets, Framework-specific native loading,
  and C#/F# runtime checks for .NET 10/11 and installed Windows Framework 4.x.
- Add supported-release CI matrices for Python, Node, TypeScript, Kotlin, Ruby,
  Perl, Go and Rust (including MSRV), plus C/C++ consumer standard checks.
- Honor caller-selected C/C++ standards and use `multileveldb.dll` as the
  native Windows filename for both MinGW and MSVC builds.

- Add C# and F# support through a shared P/Invoke/SafeHandle .NET assembly,
  Python ctypes, Ruby Fiddle and Go cgo adapters, with all native capabilities.
- Add per-language API references, runnable examples, adapter/fixture tests,
  NuGet/wheel/gem packaging and CI checks for the new adapters.

- Add explicit database creation: `ml_db_create` and `Database.create`/`create`
  factories for Rust, JVM, Node.js/TypeScript, Perl, Python, Ruby, C#/F# and Go. Return an open handle,
  reject existing databases and preserve caller settings and existing records.
- Advance the public API to 1.1.0 and binary ABI to 1.1; retain ABI 1.0 exports.

## 0.1.0 - 2026-11-14

- Introduce API 1.0.0 and C ABI 1.0 over Google LevelDB 1.23 and Snappy 1.3.1.
- Add binary CRUD, separate operation options, streaming bidirectional iterators,
  atomic write batches, snapshots, explicit error codes and resource ownership.
- Add independent Rust, JVM, Node-API and Perl bindings.
- Split JVM options, resources, error codes and exceptions into top-level
  `io.multileveldb` types, each in its own documented source file. Callers of
  the earlier nested `Database` types must update imports and recompile.
- Add UTF-8 paths, including a Windows environment using wide filesystem APIs.
- Add native, adapter and sequential cross-language integration tests.
- Add six-platform build configurations and distribution packaging.

This is the first release. Only Windows x86_64 has been exercised locally;
other platforms require their CI runs before a release is certified.
