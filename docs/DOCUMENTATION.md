# API reference documentation

Use this page to generate the API reference for your language. Run the
commands from the repository root. For a first program and setup instructions,
start with the [examples](https://github.com/spechurkin/leveldb-lib/blob/HEAD/examples/README.md) and the
[build guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md).

## C and direct C++ callers: Doxygen

Doxygen reads [the public header](https://github.com/spechurkin/leveldb-lib/blob/HEAD/native/include/multileveldb.h) and
includes its rules for pointers, memory allocation and threading. C++ callers
use the same header and reference.

With Doxygen installed, run the following.

```sh
mkdir -p build/docs/c
doxygen native/Doxyfile
```

Open `build/docs/c/html/index.html`. The configuration treats undocumented public
members, missing parameter documentation and Doxygen errors as failures. The
annotated header is also part of the native installation and archives.

## Rust: rustdoc

The crate's public types, enum variants, fields, constructors, builder setters
and methods have rustdoc comments in the matching modules under `bindings/rust/src`.
`lib.rs` re-exports the public API and contains the package example.
Inherited standard-library trait methods retain their standard contracts;
resource types document their Drop behavior and Send/Sync guarantees.

```sh
RUSTDOCFLAGS="-D warnings" cargo doc --workspace --no-deps
cargo test --doc -p multileveldb
```

On PowerShell set `$env:RUSTDOCFLAGS = '-D warnings'` first. The generated entry
point is `target/doc/multileveldb/index.html`. The crate enables `missing_docs`;
the strict documentation build rejects missing descriptions and broken links.
Normal native linking/loader setup is needed when executing doctests.

## JVM: Javadoc

Public packages, classes, enums, fields, constructors, inherited resource close
and every overload are documented with Javadoc, including parameter/return and
exception tags. Java, Kotlin, Scala and Groovy consume the same Java API.
Each public type lives in its matching file under
`bindings/jvm/src/main/java/io/multileveldb`; its Javadoc stays with that type.
Core resource classes remain in `io.multileveldb`, option builders and compression
live in `io.multileveldb.options`, and failures live in `io.multileveldb.errors`.
Import these subpackages explicitly; a wildcard import of the root package does
not include them.
`package-info.java` describes package usage and links to the main resource,
option and error types. `internal.NativeApi`, `internal.NativeCalls` and
`internal.Resource` implement the JNA boundary and ownership; they are hidden
from the public Javadoc reference.

```sh
gradle -p bindings/jvm javadoc javadocJar
```

Open `bindings/jvm/build/docs/javadoc/index.html`. A Javadoc warning fails the
task. Maven staging includes `multileveldb-1.0.0-javadoc.jar` and the sources jar.
Application runtimes that restrict native access may require
`--enable-native-access=ALL-UNNAMED` when using JNA on the classpath.

## JavaScript and TypeScript: JSDoc and declaration comments

`bindings/node/src` documents all exported classes, inherited close,
methods, option typedefs and native errors with JSDoc. Each resource has matching
`.cjs` and `.d.ts` files in `resources`; `index.cjs` and `index.d.ts` re-export
the public API. Native addon sources and Windows exports live in `native`.
Declaration comments accompany every interface/property/method for editor
help and TypeScript consumers. Binary arguments use Uint8Array/Buffer; native
failures reject promises, while some JS shape errors throw before queuing.

```sh
npm ci --prefix bindings/node
npm run docs --prefix bindings/node
npm run typecheck --prefix bindings/node
node tools/check_node_docs.cjs
```

Open `build/docs/node/index.html`. Generated pages include inherited close for
Database, Batch, Snapshot and Cursor. The checker verifies every declared API
item has JSDoc and that these inherited methods appear in generated references.
Both source/declaration comments are included in the npm package.

## Perl: POD/perldoc

All classes, their individual methods and inherited resource hooks are described
in the POD following `__END__` in `bindings/perl/lib/MultiLevelDB.pm`. The POD
also documents option hashes, octets versus Unicode, exceptions and the internal
FFI routines' ownership for source readers.

```sh
mkdir -p build/docs/perl
perldoc -F bindings/perl/lib/MultiLevelDB.pm
perl -MPod::Checker -e 'exit podchecker($ARGV[0])' bindings/perl/lib/MultiLevelDB.pm
perl -MPod::Simple::HTML -e 'my $p=Pod::Simple::HTML->new; $p->output_fh(*STDOUT); $p->parse_file($ARGV[0])' bindings/perl/lib/MultiLevelDB.pm > build/docs/perl/MultiLevelDB.html
```

POD tools can read the module without loading FFI or the native library.
The Perl distribution includes the POD. Once installed, read it with
`perldoc MultiLevelDB`.

## C# and F#: XML documentation

Both languages consume `bindings/dotnet/MultiLevelDB.csproj`. Every public type
and member has an XML documentation summary, with binary/default/ownership/error
contracts in the [.NET guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/dotnet/README.md). Build with .NET 10+:

```sh
dotnet build bindings/dotnet -c Release
dotnet pack bindings/dotnet -c Release -o build/packages
```

Warnings are errors and XML generation is enabled, so undocumented public
members fail the build. `bindings/dotnet/bin/Release/net10.0/MultiLevelDB.xml`
travels beside the assembly and in NuGet, supplying IntelliSense to C# and F#.
Both languages have separately compiled runnable examples and integration tests.
Public types live in individual files under `bindings/dotnet/src`, with option
builders in `Options`, error types in `Errors` and native plumbing in `Interop`.

## Python: docstrings/pydoc

All public resource methods have docstrings in the modules under
`bindings/python/multileveldb`. The package's `__init__.py` exports the public API.
In a POSIX shell, use an absolute `PYTHONPATH` so the module still resolves
when you change directories.

```sh
export PYTHONPATH="$PWD/bindings/python"
python -m pydoc multileveldb
mkdir -p build/docs/python
(cd build/docs/python && python -m pydoc -w multileveldb)
```

In PowerShell, set `$env:PYTHONPATH = "$PWD\bindings\python"`. Create
`build/docs/python`, change to that directory and run
`python -m pydoc -w multileveldb`.

Open `build/docs/python/multileveldb.html`. The module loads native code lazily,
so pydoc requires no engine. The [Python guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/python/README.md)
documents every class/method, option, default, return type and error category.
Docstrings, README and LICENSE are included in the wheel/source distribution.

## Ruby: RDoc

Every public class/method has RDoc comments in the files under
`bindings/ruby/lib/multileveldb`; `lib/multileveldb.rb` loads the public API.
With RDoc installed, from the repository root:

```sh
rdoc --op build/docs/ruby bindings/ruby/lib bindings/ruby/README.md
```

Open `build/docs/ruby/index.html`. Parsing does not load Fiddle/native code. The
[Ruby guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/ruby/README.md) covers binary strings, all options,
blocks, exceptions and lifetime rules. Source comments travel with the gem.

## Go: go doc

Every exported declaration has Go documentation, including options fields,
errors, resources and all cursor methods. With native headers configured:

```sh
go -C bindings/go doc -all .
go -C bindings/go vet ./...
```

The [Go guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/go/README.md) documents cgo setup, every public method,
defaults and missing/empty semantics. These sources are the distributable module;
the generated reference can be captured with `go doc -all . > reference.txt`.
Root aliases retain the public import path. Method and field documentation
remains with the types in `internal/database`, `internal/options` and
`internal/errors`; for example, `go doc -all ./internal/database` renders the
resource implementation reference.
