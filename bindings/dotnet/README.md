# C# and F# (.NET)

## Setup

C# and F# share one NuGet package. It contains a `net10.0` assembly
for .NET 10 and 11 and a `net462` assembly for Windows .NET Framework 4.6.2
through 4.8.1. Install a matching native ABI 1.1 library, then either set
`MULTILEVELDB_LIBRARY` to its absolute path or put it on the native loader's
search path. The library stays loaded until the process exits.

Framework consumers use `MultiLevelDB.Managed.dll`; the different assembly name
prevents Windows from confusing the managed assembly with `multileveldb.dll`.
Keep that exact native filename, including when using `MULTILEVELDB_LIBRARY`.
Framework support also depends on the host Windows support lifecycle. See the
[release matrix](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/COMPATIBILITY.md) for supported branches and checks.

```sh
dotnet build bindings/dotnet -c Release
dotnet pack bindings/dotnet -c Release -o build/packages
```

Consumers can reference the project or install the locally built NuGet package.
The package includes the assembly, XML IntelliSense reference, README and
LICENSE. The engine is a separate native dependency. Runtime/dependency DLLs
must also be discoverable; see [BUILDING](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md).

## Open a database

### C#

```csharp
using System;
using MultiLevelDB;

using var options = new OpenOptions().Compression(Compression.Snappy).CacheSize(0);
using var db = Database.Create("data", options);
using var writes = new WriteOptions().Sync(true);
db.Put(new byte[] {0, 255}, new byte[] {0, 128, 255}, writes);
byte[]? value = db.Get(new byte[] {0, 255});
using var snapshot = db.CreateSnapshot();
using var batch = new Batch();
batch.Put(new byte[] {0, 255}, Array.Empty<byte>());
db.Write(batch);
byte[]? old = db.Get(new byte[] {0, 255}, snapshot: snapshot);
```

### F#

```fsharp
open MultiLevelDB

let example () =
    use options = new OpenOptions()
    options.Compression(Compression.Snappy).CacheSize(0UL) |> ignore
    use db = Database.Create("data", options)
    use writes = new WriteOptions()
    writes.Sync(true) |> ignore
    db.Put([|0uy; 255uy|], [|0uy; 128uy; 255uy|], writes)
    let value = db.Get([|0uy; 255uy|]) |> Option.ofObj
    use cursor = db.Iterator()
    cursor.SeekToFirst()
    while cursor.IsValid do
        let key, bytes = cursor.Key(), cursor.Value()
        cursor.Next()
    cursor.Status()
    value
```

`Database.Create(path, options = null)` creates a new DB and fails if it exists.
It copies options, forcing creation/existence flags without modifying the
caller's builder. `Database.Open(path, options = null)` uses engine defaults,
including creation disabled. Both return an owned Database, accept strict
Unicode paths encoded as UTF-8, and require existing parent directories.
Empty/NUL paths and unpaired surrogates fail. Close never deletes data.

Keys/values are non-null byte arrays; empty keys/values and arbitrary octets
are supported. Arrays are borrowed only during synchronous calls; callers must
not mutate them concurrently. Returned arrays are copies. Get returns null only
for missing keys; a zero-length array is an existing empty value. In F#, use
`Option.ofObj` to preserve this distinction and `use` for deterministic cleanup.

## Methods and resource lifetime

Public API in namespace `MultiLevelDB`:

- Database: `Create`, `Open`, `Get(key, ReadOptions? options, Snapshot? snapshot)`,
  `Put(key, value, WriteOptions? options)`, `Delete(key, WriteOptions? options)`,
  `Write(Batch, WriteOptions? options)`, `CreateSnapshot()`,
  `Iterator(ReadOptions? options, Snapshot? snapshot)`. Optional arguments default
  to null. Write methods return void.
- `new Batch()` allocates a database-independent sequence. `Put`/`Delete` copy
  arrays and return this batch; `Clear` discards pending operations. `Write`
  applies the batch atomically in insertion order without consuming it.
- Cursor: `SeekToFirst`, `SeekToLast`, `Seek(key)` (first key >= target), `Next`,
  `Previous`, `IsValid` property, `Key()`, `Value()`, `Status()`. New cursors are
  unpositioned. Passing an edge succeeds and becomes invalid; accessing/moving an
  already invalid cursor throws NotFound. Always check Status after scanning.
- Snapshot: native same-instance view from CreateSnapshot. A foreign view fails
  InvalidArgument. Existing cursors retain their view after Snapshot.Dispose.
- Resource: `Dispose()` releases once; C# `using` and F# `use` call it. Repeated
  disposal is harmless; closed calls throw DatabaseClosed. SafeHandle supplies
  fallback cleanup. DB disposal does not invalidate existing cursors; dependents
  retain the engine and its directory lock until they are disposed too.

## Options

Options are owned resources. Fluent methods set native settings and return the
builder; operations copy them. Use separate kinds:

- `OpenOptions`: `CreateIfMissing(bool)` false, `ErrorIfExists(bool)` false,
  `ParanoidChecks(bool)` false, `WriteBufferSize(ulong)` 4 MiB,
  `MaxOpenFiles(ulong)` 1000, `BlockSize(ulong)` 4096,
  `BlockRestartInterval(ulong)` 16, `Compression(Compression)` Snappy/None,
  `CacheSize(ulong)` unset engine policy; zero selects zero capacity.
- `ReadOptions`: `VerifyChecksums(bool)` false, `FillCache(bool)` true.
- `WriteOptions`: `Sync(bool)` false; true requests synchronous engine flush.

Native range validation applies; positive tuning values may be clamped by the
engine. No public raw-pointer API is exposed. All operations and SafeHandle
cleanup share one adapter lock across .NET threads; engine work blocks callers.
Batches provide atomic writes without transactional reads/conflict detection.

## Errors

Native failures throw `DatabaseException`, with stable enum `Code` and original
diagnostic `Message`. `ErrorCode`: Ok, NotFound, InvalidArgument, IoError,
Corruption, DatabaseLocked, DatabaseClosed, Unsupported, InternalError. Only
missing Get maps NotFound to null. Null required arrays/paths throw
ArgumentNullException; invalid Unicode uses EncoderFallbackException. Oversized
native buffers fail with OverflowException after freeing the allocation. Native
loader/ABI initialization failures use CLR loader/initializer exceptions.

## Tests and API reference

```sh
dotnet run --project tests/dotnet -c Release
dotnet run --project tests/fsharp -c Release
dotnet run --project examples/csharp -c Release
dotnet run --project examples/fsharp -c Release
```

Every public type/member has XML documentation. `GenerateDocumentationFile` and
warnings-as-errors validate coverage during build. The XML file beside the DLL
is included automatically in NuGet and supplies the same editor help to C# and
F#. See generated `bin/Release/net10.0/MultiLevelDB.xml` for the reference.
