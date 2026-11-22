# Language APIs

## Capabilities

Every binding can open or create a database, read and write bytes, scan with
a cursor, apply an atomic batch and read from a snapshot. Each binding has
separate options for opening, reading and writing. LevelDB compares keys
byte by byte. Node.js runs blocking LevelDB calls on worker threads. In other
bindings, the calls run synchronously on the caller's thread.

| Binding | Missing key from `get` | How to release resources | Sharing a database handle |
|---|---|---|---|
| C / C++ | `ML_NOT_FOUND` | Destroy or release each handle | Concurrent operations on a live database |
| Rust | `None` | Drop or leave scope | `Database` is `Send + Sync` |
| JVM | `null` | Try-with-resources or `close()` | The binding synchronizes calls |
| Node.js / TypeScript | Promise resolves to `null` | Await `close()` | Calls on shared resources run in queue order |
| Perl | `undef` | Call `close` | Open separate directories in separate ithreads |
| C# / F# | `null` | C# `using`, F# `use` or `Dispose()` | The binding serializes calls |
| Python | `None` | `with` or `close()` | The binding serializes calls |
| Ruby | `nil` | A resource block or `close` | The binding serializes calls |
| Go | `found == false` | `defer db.Close()` or call `Close()` | The binding serializes calls |

A present key can hold zero bytes. A new cursor has no position.
Seek to the first key, last key or a key at or above your target. Check
validity before reading the current key and value, then check status when
the scan ends. Close the cursor and any snapshot to release their references
to the database and its lock. See the [C ABI lifetime rules](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/ABI.md#handle-lifetime-and-threads).

By default, opening requires an existing database. LevelDB uses a 4 MiB
write buffer, up to 1,000 open files, 4 KiB blocks, a restart interval of 16
and Snappy compression. The defaults enable read caching and disable
paranoid checks, checksum verification and synchronous writes. LevelDB
can clamp tuning values. The public API covers the operations described
here. Properties, approximate sizes, manual compaction and custom comparator
callbacks are outside its scope.

## C

`ml_db_create(path, length, options, out, error)` creates a new database and
returns its open handle. `options` may be NULL. Creation copies OPEN settings,
forcing `ML_CREATE_IF_MISSING` and `ML_ERROR_IF_EXISTS` true without changing
the original options. Existing databases fail with `ML_INVALID_ARGUMENT`;
directory contention may fail first with `ML_DATABASE_LOCKED`. Parent
directories must already exist. This operation is available since API 1.1.

Use `ml_options_create`, `ml_options_set` and `ml_db_open`. Every fallible
operation returns `ml_status`; pass `ml_error**` to retain diagnostics. Read with
`ml_db_get`, write/delete with `ml_db_put`/`ml_db_delete`. Buffers returned from
get or iterator key/value must be freed using `ml_buffer_free`.

Cursor calls are `ml_iterator_create`, `seek_first`, `seek_last`, `seek`, `next`,
`previous`, `valid`, `key`, `value`, `status`, and `destroy` with the common
`ml_iterator_` prefix. Batches use `ml_batch_create/put/delete/clear/write/destroy`.
Snapshots use `ml_snapshot_create/release` and are optional read arguments.
See the installed header for exact signatures and [ABI rules](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/ABI.md).

## Rust

`OpenOptions::new()`, `ReadOptions::new()` and `WriteOptions::new()` return owned
builders with fallible setters. Options use snake_case names; `Compression` is
`None` or `Snappy`. `Database::open(path, options)` returns `Result<Database>`.
`Database::create(path, options)` creates a new database with the same return
type and the creation policy described above; use `None` for default storage
settings.

- `get(key, read_options, snapshot) -> Result<Option<Vec<u8>>>`
- `put(key, value, write_options)`, `delete(key, write_options)`
- `Batch::new()`, `put`, `delete`, `clear`, `Database::write(&batch, options)`
- `snapshot() -> Snapshot<'db>`
- `iterator(read_options, snapshot) -> DbIterator<'db>`
- Cursor: `seek_to_first`, `seek_to_last`, `seek`, `next`, `previous`,
  `is_valid`, `key`, `value`, `status`.

Database is Send + Sync. Iterators are Send but not Sync. Mutating cursor and
batch methods require exclusive borrowing. Snapshot/iterator lifetimes prevent
dropping or explicitly closing their borrowed database. A cursor internally
retains its snapshot after the Snapshot wrapper is dropped. All resources use
Drop; `Database::close(&mut self)` provides explicit logical close. Errors expose
`ErrorCode` and the native diagnostic.

## JVM

Package `io.multileveldb`, entry point `Database.open(Path, OpenOptions)`.
Each public type has its own source file: `OpenOptions`, `ReadOptions`,
`WriteOptions`, `Compression`, `Batch`, `Snapshot`, `Cursor`, `Code`,
`DbException` and its exception subclasses are top-level classes. Core handles
remain in `io.multileveldb`; settings and compression are in
`io.multileveldb.options`, and status/exception types are in
`io.multileveldb.errors`. `internal.Resource` supplies ownership/close behavior;
`options.Options` is the base for
the three option types. All resources implement AutoCloseable with phantom-reference
fallback. Use try-with-resources.

`Database.create(Path)` and `Database.create(Path, OpenOptions)` return a new
open database and reject existing databases with `InvalidArgumentException`
(or `LockedException` when lock acquisition fails first).

Import these types directly, for example
`import io.multileveldb.options.OpenOptions;` and
`import io.multileveldb.errors.DbException;`. Construct settings with
`new OpenOptions().createIfMissing(true)`. Update old root-package imports to
these subpackages; root wildcard imports do not include subpackages.
This changes their Java binary names; recompile
JVM callers against the updated jar. Method behavior and the native ABI are
unchanged.

`get(byte[])` returns byte[] or null; overloads take ReadOptions and Snapshot.
Put/delete/write overloads take WriteOptions. Builders use camelCase names and
`Compression.NONE/SNAPPY`. `createBatch`, `createSnapshot`, `iterator` return
Batch, Snapshot and Cursor. Cursor methods are `seekToFirst`, `seekToLast`,
`seek`, `next`, `previous`, `isValid`, `key`, `value`, `status`.

Failures throw subclasses of `DbException`: NotFoundException,
InvalidArgumentException, IoException, CorruptionException, LockedException,
ClosedException, UnsupportedException, or DbException with INTERNAL_ERROR.
All exceptions expose `code`. Missing get returns null instead of throwing.
Native buffers exceeding Java's byte[] capacity are rejected after freeing them.
No arbitrary smaller value limit is imposed.

Resource methods serialize against close, including fallback cleanup. Kotlin,
Scala and Groovy can call this Java API directly. Java 8+ (built with JDK 25) and a 64-bit JNA runtime
are required. The native library remains a separately installable dependency.

## Node.js and TypeScript

Both `require('multileveldb')` and `import { Database } from 'multileveldb'` work.
`Database.open(path, options)` resolves to a database. All operations, including
cursor movement and close, return promises. Keys/values accept Buffer and
Uint8Array and are copied at invocation time. Get resolves to Buffer or null.
`Database.create(path, options)` resolves to a newly created database, rejecting
existing databases with `INVALID_ARGUMENT` (or `DATABASE_LOCKED` first).

`put`, `get`, `delete`, `createBatch`, `write`, `createSnapshot`, `iterator`
match JVM naming. Read options and snapshot are separate optional arguments.
All option names use camelCase, with compression `'none'` or `'snappy'`.
Native failures reject with Error carrying stable string `code`. Invalid JS
argument shapes throw TypeError before work is queued.

Native Node-API workers perform blocking calls. Operations sharing resources
are ordered by invocation. A write involving a batch participates in both
queues. Close waits for previously queued work, and further operations fail
DATABASE_CLOSED. Independent resources can run concurrently. Finalizers are a
fallback, not a substitute for awaiting close. In-process queues provide
ordering, not transactions; only WriteBatch supplies atomic multi-key writes.

## Perl

`use MultiLevelDB;` then `MultiLevelDB::Database->open($path, \%open_options)`.
`MultiLevelDB::Database->create($path, \%open_options)` creates a new database;
the options hash may be omitted and is never modified. Existing databases throw
`INVALID_ARGUMENT`, or `DATABASE_LOCKED` if lock acquisition fails first.
Get returns an octet string or undef; `''` is an existing empty value.
Use `put`, `get`, `delete`, `create_batch`, `write`, `create_snapshot`, `iterator`.
Get and iterator accept optional read-options hash and Snapshot. Write methods
accept an optional write-options hash. Option names are snake_case and
compression is `'none'` or `'snappy'`; boolean options use 0/1.

Cursor methods are `seek_to_first`, `seek_to_last`, `seek`, `next`, `previous`,
`is_valid`, `key`, `value`, `status`. Batches support put/delete/clear. All
resources have idempotent close and DESTROY fallback. Native errors throw a
`MultiLevelDB::Error` object with code/message accessors and string overload.
Invalid Perl argument shapes raise normal Perl exceptions.

Binary arguments must be octet strings. Characters above 255 are rejected;
explicitly encode textual application data before passing it. Paths accept
Unicode strings or valid UTF-8 octets and are encoded with error checking.
Resources are not shareable across Perl ithreads (`CLONE_SKIP`); each thread
must open its own independent database directory.

## C# and F#

Both use the same package on .NET 10/11 or Windows Framework 4.6.2+. `Database.Create(path, options)`
creates a fresh database and rejects an existing DB without mutating settings;
`Database.Open` keeps creation disabled by default. Keys/values are byte arrays;
Get returns an owned array or null (F#: `Option.ofObj`). Fluent `OpenOptions`,
`ReadOptions`, `WriteOptions` builders expose all settings. Resources implement
IDisposable with SafeHandle fallback: use C# `using` or F# `use`.

Database methods: `Get`, `Put`, `Delete`, `Write`, `CreateSnapshot`, `Iterator`.
`new Batch()` supplies `Put`, `Delete`, `Clear`; cursors expose `SeekToFirst`,
`SeekToLast`, `Seek`, `Next`, `Previous`, `IsValid`, `Key`, `Value`, `Status`.
Native failures throw DatabaseException retaining ErrorCode and engine Message.
Calls and cleanup share an adapter lock. See the [.NET reference](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/dotnet/README.md).

## Python

`from multileveldb import Database, Batch` on Python 3.10+. `Database.create(path,
**options)` creates exclusively; `Database.open` uses defaults. Binary inputs
accept bytes/bytearray/memoryview. `get` returns bytes or None; empty bytes are
an existing empty value. Methods use snake_case: `put`, `get`, `delete`, `write`,
`create_snapshot`, `iterator`, cursor `seek_to_first`/`seek_to_last`/`seek`/
`next`/`previous`/`is_valid`/`key`/`value`/`status`, Batch `put`/`delete`/`clear`.
Read/write options are separate dictionaries; snapshots are separate arguments.
Resources support `with` and idempotent `close`, with weakref finalizer fallback.
`Error` preserves status/code/message; missing get alone maps NOT_FOUND to None.
Calls and cleanup share a reentrant adapter lock. See the [Python reference](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/python/README.md).

## Ruby

`require 'multileveldb'` on Ruby 3.1+ with Fiddle. `MultiLevelDB::Database.create`
creates exclusively; `open` uses defaults. Both support block cleanup. Inputs are
raw String bytes, outputs are copied ASCII-8BIT Strings; missing get is nil.
Methods match Python's snake_case names, except cursor `valid?`. Read options and
snapshot use keywords; open/write options use keyword hashes. Resources support
`close` and `use { |resource| ... }`, with fallback finalizers. Native Error exposes
status/code/message. Calls and cleanup share a Monitor across threads. See the
[Ruby reference](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/ruby/README.md).

## Go

The cgo package exposes `Create(path, *OpenOptions)` and `Open`, returning
`(*Database, error)`. Inputs are []byte, including nil for empty; Get returns
`([]byte, bool, error)` so found=false distinguishes missing from empty data.
Typed OpenOptions/ReadOptions/WriteOptions structs use nil for engine defaults.
Methods match .NET names. `NewBatch()` is database-independent. Cursor IsValid
returns `(bool, error)`; Key/Value return copied slices. All resources have
idempotent `Close() error` and fallback runtime finalizers. Copies share ownership.
`*Error` retains Code and Message; use errors.As. Calls and cleanup share a mutex
across goroutines. See the [Go reference](https://github.com/spechurkin/leveldb-lib/blob/HEAD/bindings/go/README.md).
