# Go

## Setup

Use Go 1.20 or later with cgo and a C compiler. Build the native library with
ABI 1.1 or install a matching archive. cgo needs the public header and the
library at build time. The program also needs the shared library at runtime.

The following commands use a local Linux build from the repository root.

```sh
export CGO_ENABLED=1
export CGO_CFLAGS="-I$PWD/native/include"
export CGO_LDFLAGS="-L$PWD/build/native"
export LD_LIBRARY_PATH="$PWD/build/native${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd bindings/go
go test ./...
go vet ./...
go doc -all .
```

For an installed native archive use its include/lib directories. On macOS use
DYLD_LIBRARY_PATH. On Windows use MinGW with a matching GNU import library,
set CC if necessary, and put runtime/native DLLs on PATH. An MSVC `.lib` alone
is not a cgo GNU import library. CI currently executes Go on Linux/macOS; the
Windows MinGW adapter is validated locally.

Import `github.com/spechurkin/leveldb-lib/bindings/go`. To use version 1.0.0
once the release tag is available, run this from your Go module.

```sh
go get github.com/spechurkin/leveldb-lib/bindings/go@v1.0.0
```

To use a source checkout, point a `replace` directive in your `go.mod`
at `bindings/go`. See the [runnable example's go.mod](https://github.com/spechurkin/leveldb-lib/blob/HEAD/examples/go/go.mod).

## Open a database

This snippet creates a new database from inside a function that returns an error.

```go
db, err := multileveldb.Create("data", nil)
if err != nil { return err }
defer db.Close()
if err := db.Put([]byte{0, 255}, []byte{0, 128, 255}, &multileveldb.WriteOptions{Sync: true}); err != nil {
    return err
}
value, found, err := db.Get([]byte{0, 255}, nil, nil)
// found distinguishes missing data from an existing empty slice.
_ = value
_ = found
return err
```

`Create(path, *OpenOptions)` creates a new database, forcing creation/existence
flags without modifying options, and fails if one exists. `Open(path,
*OpenOptions)` uses defaults with creation disabled. Both return `(*Database,
error)`, encode paths as strict UTF-8, require parent directories, and reject
empty/NUL/invalid UTF-8 paths. Close does not delete data.

Keys/values are byte slices; nil is a valid empty input. Inputs are borrowed only
during calls and must not be mutated concurrently. Outputs are independent
copies. `Get` returns `(value []byte, found bool, err error)`; missing is
`nil, false, nil`, while existing empty data is a non-nil empty slice with
found=true. There is no implicit text conversion.

## Methods and resource lifetime

Public API:

- Database: `Get(key, *ReadOptions, *Snapshot)`, `Put(key, value, *WriteOptions)`,
  `Delete(key, *WriteOptions)`, `Write(*Batch, *WriteOptions)`, `CreateSnapshot()`,
  `Iterator(*ReadOptions, *Snapshot)`, `Close()`. Optional settings/views use nil.
- `NewBatch() (*Batch, error)`: allocate a database-independent batch. `Put`,
  `Delete`, `Clear` return error. Queued binary inputs are copied. `Write`
  atomically applies operations in order without consuming or clearing them.
- Cursor: `SeekToFirst`, `SeekToLast`, `Seek(key)` (first key >= target), `Next`,
  `Previous`, `Status` return error; `IsValid() (bool, error)`,
  `Key()/Value() ([]byte, error)`. New cursors are unpositioned. Passing an edge
  succeeds and invalidates; accessing/moving an already invalid cursor returns
  NotFound. Check Status after a scan; IsValid alone does not inspect engine errors.
- Snapshot: native same-instance read view. A foreign view returns
  InvalidArgument. Existing snapshot cursors retain their own view after close.
- All resource `Close() error` methods are idempotent. Zero-value wrappers are
  closed and can safely be closed again. Copies share resource ownership and
  cursor position; closing a copy closes that wrapper's shared state. Runtime
  finalizers are a fallback. Do not pass nil wrapper receivers. DB close allows
  existing cursors to continue; dependents retain directory locks until closed.

## Options

`OpenOptions` booleans default false: CreateIfMissing, ErrorIfExists,
ParanoidChecks. Numeric zero selects defaults: WriteBufferSize (4 MiB),
MaxOpenFiles (1000), BlockSize (4096), BlockRestartInterval (16). Nonzero values
must fit native types; engine bounds may clamp them. `Compression *Compression`
nil selects Snappy; NoCompression and Snappy are enum values. `CacheSize *uint64`
nil uses engine policy; pointer-to-zero explicitly selects zero capacity.
`ReadOptions.VerifyChecksums` defaults false; `FillCache *bool` nil means true.
`WriteOptions.Sync` defaults false. True requests engine synchronous flush.

All calls and cleanup share an adapter mutex across goroutines. Operations are
synchronous and block callers. Cursors stream records and snapshots retain old
versions. Batches provide atomic writes without transactional reads/conflicts.

## Errors

Native failures return `*Error`, with stable numeric `Code` and original
`Message`. Use `errors.As`. Codes: OK, NotFound, InvalidArgument, IOError,
Corruption, DatabaseLocked, DatabaseClosed, Unsupported, InternalError. Missing
Get alone returns found=false without an error. Slice-capacity failures use a Go
error after freeing the native buffer. Missing native code is a link/loader
failure before main. A wrong ABI major returns Unsupported before allocation.

The root package keeps the public API in `api.go`. Implementation packages are
grouped under `internal/database`, `internal/options` and `internal/errors`.
Type aliases preserve the existing import path and methods. Every exported
declaration has Go documentation comments, including resource lifetime rules.
`go doc -all .` renders the public entry points; `go doc -all ./internal/database`
renders the resource methods. `go test ./...` includes the
fixture test `TestInterop`, which runs only under the sequential integration
driver; ordinary adapter runs skip that one test.
