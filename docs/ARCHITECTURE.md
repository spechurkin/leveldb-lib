# Architecture

Each language binding uses the same C ABI. The native library calls Google
LevelDB, which owns the database files, write-ahead log, compaction and locking.

```text
Rust   JVM/JNA   Node-API   Perl/Platypus   C#/F# P/Invoke   Python/ctypes   Ruby/Fiddle   Go/cgo
  \       |         |           |               |               |             |         /
                           multileveldb.h: versioned C ABI
                              |
                 Google LevelDB C++ API
                              |
                    LevelDB directory
```

The native library validates input, converts options, manages handles and
catches C++ exceptions. It calls LevelDB's C++ API so it can map typed statuses
such as corruption or a missing key to stable error codes.

Every binding uses LevelDB's default bytewise comparator. A database created
with a custom comparator requires that comparator to reopen. The public ABI
also keeps environment objects and filter callbacks internal.

On POSIX, LevelDB uses its own filesystem environment. On Windows, an
`EnvWrapper` converts UTF-8 paths to UTF-16 before calling Win32 filesystem
APIs. It serializes random reads that share a file cursor. LevelDB handles
scheduling, timing and the on-disk format on both platforms. The native
library links Snappy statically.

The bindings build independently. Rust calls the C ABI directly, the JVM
uses JNA, Node uses Node-API, and Perl uses FFI::Platypus. Python uses ctypes,
Ruby uses Fiddle, and Go uses cgo. C# and F# share a P/Invoke assembly.

## Binding source layout

Bindings separate database operations, batches, snapshots, cursors, settings and
native interop. Their entry points and implementation directories are:

- .NET: one type per file under `bindings/dotnet/src`; `Options`, `Errors` and
  `Interop` contain the builders, failure types and P/Invoke ownership code.
- JVM: `io.multileveldb` contains Database, Batch, Snapshot and Cursor;
  `io.multileveldb.options` contains the builders and compression enum,
  `io.multileveldb.errors` contains status codes and typed failures, and
  `io.multileveldb.internal` contains JNA calls and resource ownership.
  The directory paths match these Java packages under `src/main/java`.
- Rust: `src/lib.rs` re-exports the resource, option and error modules;
  `ffi.rs` declares the ABI and `native.rs` converts native calls and buffers.
- Node: `src/index.cjs` and `index.d.ts` re-export the modules in `resources`;
  `internal` owns loading, queuing and option validation, `types` holds shared
  declarations, and `native` separates worker execution from the Node-API bridge.
- Python: `multileveldb/__init__.py` exports resource and error classes;
  `_native`, `_resource` and `_options` handle ctypes, ownership and settings.
- Ruby and Perl: `lib/multileveldb.rb` and `lib/MultiLevelDB.pm` load the class
  files in their matching directories, including the internal native boundary.
- Go: `api.go` retains the public `multileveldb` import with type aliases and
  factories. `internal/database` owns resources, native calls and shared
  ownership; `internal/options` and `internal/errors` hold settings and failures.
  `doc.go` contains the package contract.

## Resource graph

The database state owns the LevelDB instance and its optional block cache.
A snapshot holds a reference to that state. A cursor holds its own reference
to the database and to any snapshot it reads from. A batch is independent of
the database and can be written more than once.

Closing a C database handle marks it closed and releases its reference to
LevelDB. Existing cursors can keep reading. They and any open snapshots keep
the engine and directory lock alive, so reopening the database can return
`DATABASE_LOCKED` until you release those resources.

## Adding a language binding

Use the same native library as the existing bindings. Start with the
[public C header](https://github.com/spechurkin/leveldb-lib/blob/HEAD/native/include/multileveldb.h) and follow the
[ABI rules for errors, memory and threading](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/ABI.md).

### Implement the binding

1. Link or load the matching native shared library and include/map the C header.
2. Check `ml_abi_version() >> 16 == 1` before creating handles.
3. Map fixed integers and native `size_t` correctly. In particular, Windows
   `unsigned long` is 32-bit even on x86_64; it is not `size_t`.
4. Preserve octets and explicit lengths. Never route keys/values through a
   NUL-terminated string or an implicit UTF-8 conversion. Encode only paths.
5. Wrap owned handles with deterministic close and a fallback destructor.
   Serialize a handle's destruction against all operations using it.
6. Copy returned data into a language-owned byte type, then always free it with
   `ml_buffer_free`, even if conversion fails. Free native errors after copying
   their message. Preserve status codes and raw engine diagnostics.
7. Distinguish NOT_FOUND from an existing zero-byte value using an optional
   value or the language's equivalent. Cursor NOT_FOUND is an error.
8. Expose separate open/read/write options and pass the snapshot as a read
   dependency. Never substitute an in-memory copy for a LevelDB snapshot.
9. Keep the shared library loaded until all its resources and finalizers finish.

For examples of ownership and status mapping, see the ctypes, Fiddle,
P/Invoke or cgo implementations in the [binding source layout](#binding-source-layout).

### Test against the other bindings

Run your binding's tests with Unicode paths, a locked directory, cursor
movement and repeated close. Check allocation and cleanup on the platforms
you support, including the mapping for native pointer widths.

The shared binary fixture lives in `tests/fixtures/binary.tsv`: each line has
hex key, TAB, hex value, with `-` for an empty sequence. The integration driver
runs Rust, JVM, Node, Perl, Python, Ruby, C#, F#, Go and C sequentially against one directory so the
LevelDB process lock is respected.
