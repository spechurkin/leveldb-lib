# MultiLevelDB

[![Build and test](https://github.com/spechurkin/leveldb-lib/actions/workflows/build.yml/badge.svg?event=push)](https://github.com/spechurkin/leveldb-lib/actions/workflows/build.yml)
[![Language compatibility](https://github.com/spechurkin/leveldb-lib/actions/workflows/compatibility.yml/badge.svg?event=push)](https://github.com/spechurkin/leveldb-lib/actions/workflows/compatibility.yml)
[![Latest release](https://img.shields.io/github/v/release/spechurkin/leveldb-lib)](https://github.com/spechurkin/leveldb-lib/releases/latest)
[![0BSD license](https://img.shields.io/badge/license-0BSD-blue)](https://github.com/spechurkin/leveldb-lib/blob/HEAD/LICENSE)

MultiLevelDB lets you read and write Google LevelDB databases from C, C++,
Rust, Java and other JVM languages, Node.js/TypeScript, Perl, C#, F#, Ruby,
Python and Go. The bindings share a native library that uses LevelDB's
standard on-disk format and default bytewise ordering.

Use it to store byte strings locally or to open an existing LevelDB
directory from another language. You can scan keys in order with a cursor,
apply several writes atomically with a batch, or read a consistent view with
a snapshot.

## Quick start

Most bindings need a native library for your OS and architecture.
Download native archives and adapter distributions from [GitHub Releases](https://github.com/spechurkin/leveldb-lib/releases).
Find registry packages under [GitHub Packages](https://github.com/spechurkin/leveldb-lib/packages).
A prepared Node.js package includes the addon and its companion library.
For a source checkout or an installed native archive, follow the
[build and loader instructions](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md).

The snippets below write to a database named `data`. Run them in a scratch
directory. For another language, see the [runnable examples](https://github.com/spechurkin/leveldb-lib/blob/HEAD/examples/README.md).

### Node.js

With the package available to your app, save this as `example.cjs` and run
`node example.cjs`. If you installed `@spechurkin/multileveldb` from GitHub
Packages, use that name in `require`. The first read returns an empty
buffer. The read after deletion returns `null`.

```javascript
const { Database } = require("multileveldb");

async function main() {
  const db = await Database.open("data", { createIfMissing: true });
  try {
    const key = Buffer.from([0, 255]);
    await db.put(key, Buffer.alloc(0));
    console.log(await db.get(key)); // <Buffer >

    await db.delete(key);
    console.log(await db.get(key)); // null
  } finally {
    await db.close();
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
```

### Rust

```rust,no_run
use multileveldb::{Database, OpenOptions, Result};

fn main() -> Result<()> {
    let mut options = OpenOptions::new()?;
    options.create_if_missing(true)?;
    let db = Database::open("data", Some(&options))?;

    db.put(&[0, 255], &[0, 128, 255], None)?;
    assert_eq!(db.get(&[0, 255], None, None)?, Some(vec![0, 128, 255]));
    Ok(())
}
```

### Opening, creating and closing a database

By default, `open` requires an existing database. Enable create-if-missing
to open or create one, as in the examples. Use your binding's `create`
function to require a new database. It returns an open handle and reports an
error if the database already exists. Create the parent directories first.

Close cursors and snapshots as well as the database before reopening the
directory. They can keep the LevelDB lock alive after the database handle
closes. Rust releases its handles when they go out of scope.

## Build the native library

You need CMake 3.20+, Git and a C++17 compiler. The first configure fetches
the pinned LevelDB 1.23 and Snappy 1.3.1 sources. For an offline build, point
CMake at local copies of those sources. See the [build guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md#native).

```sh
git clone https://github.com/spechurkin/leveldb-lib.git
cd leveldb-lib
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix build/install
```

The install directory contains the shared library, C header, CMake targets and
license notices. For applications and older CTest versions on Windows, add
the DLL directory to `PATH`. It is usually `build/native` for a local build
or `build/native/Release` with Visual Studio. CMake 3.22+ sets the path for
the native CTest tests.

## Documentation

- Find method names and return values in the [language API guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/API.md).
- Set up your binding with the [Node.js](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/node/README.md),
  [Rust](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/rust/README.md), [Perl](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/perl/README.md),
  [.NET](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/dotnet/README.md), [Python](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/python/README.md),
  [Ruby](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/ruby/README.md) or [Go](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/go/README.md) guide.
  See the [build guide](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md#jvm) for JVM setup.
- Check [platforms, language versions and database formats](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/COMPATIBILITY.md)
  and see which checks ran in the [local validation record](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/VALIDATION.md).
- For binding authors, see the [architecture](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/ARCHITECTURE.md),
  [C ABI and ownership rules](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/ABI.md), [binding checklist](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/ARCHITECTURE.md#adding-a-language-binding)
  and [commands to generate API references](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/DOCUMENTATION.md).
- See what's changed in the [changelog](https://github.com/spechurkin/leveldb-lib/blob/HEAD/CHANGELOG.md) or report a problem
  on the [issue tracker](https://github.com/spechurkin/leveldb-lib/issues).

Package version **1.0.0**, public API **1.1.0**, C ABI **1.1**.

## License and contributions

MultiLevelDB is licensed under the [BSD Zero Clause License (0BSD)](https://github.com/spechurkin/leveldb-lib/blob/HEAD/LICENSE).
LevelDB and Snappy are distributed under their own licenses.

You retain copyright in your contributions and license them under 0BSD when
you submit them for inclusion in MultiLevelDB. No separate contributor agreement
or copyright assignment is required.
