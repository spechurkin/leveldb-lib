# Examples

Build or install the native library and [configure the loader](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md#loader-configuration)
before running an example. Run the commands below from the repository root.
Each example opens `example-db` in its working directory, creating it if
needed. Run one example at a time when they share a working directory,
because LevelDB locks the database directory.

- C: compile `c/main.c` with `-I<prefix>/include -L<prefix>/lib -lmultileveldb`.
  On Windows, link the matching import library and put the DLL on PATH.
- C++: compile `cpp/main.cpp` with the same C header/import library.
- Rust: set ML_NATIVE_DIR, then `cargo run --manifest-path examples/rust/Cargo.toml`.
- Java: add the built JVM jar and JNA jar to the classpath; compile and run
  `java/Example.java` with Java 8+ (the library itself is built with JDK 25).
- Kotlin: compile `kotlin/Example.kt` with the same classpath and Kotlin's
  standard library, or build the JVM binding first and run
  `gradle -p examples/kotlin run` with Kotlin 2.4.21. No additional native adapter is needed.
- JavaScript. [Build the Node addon](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md#nodejs), then run
  `node examples/javascript/main.cjs`.
- TypeScript: install/link the Node package, TypeScript and matching @types/node,
  then compile `typescript/main.ts` as ES2022 with NodeNext resolution in an
  ESM package. Its import resolves the packaged declarations.
- Perl: `perl -I bindings/perl/lib examples/perl/main.pl`.
- C#: `dotnet run --project examples/csharp -c Release` (.NET 10+).
- F#: `dotnet run --project examples/fsharp -c Release` (.NET 10+).
  Both reference the same MultiLevelDB assembly and use `using`/`use` cleanup.
- Python: install `bindings/python` (or set PYTHONPATH to that directory), then
  `python examples/python/main.py` (Python 3.10+).
- Ruby: `ruby -I bindings/ruby/lib examples/ruby/main.rb` (Ruby 3.1+, Fiddle).
- Go: configure cgo include/library paths as in [the binding guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/go/README.md),
  then `go -C examples/go run .` (Go 1.20+ and a C compiler).
