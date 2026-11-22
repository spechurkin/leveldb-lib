# C ABI 1.1

Include `multileveldb.h` from the installation's `include` directory. To look
up function signatures, see the [public header](https://github.com/spechurkin/leveldb-lib/blob/HEAD/native/include/multileveldb.h).

The ABI supports 64-bit targets. It uses C linkage and the Windows cdecl
calling convention. Handles are opaque. Use `size_t` for byte lengths, signed
32-bit integers for status codes, unsigned 32-bit integers for option and
feature IDs, and unsigned 64-bit integers for option values.

## Versioning

`ml_library_version()` reports the SemVer distribution version, currently 1.0.0.
`ml_api_version()` reports public API 1.1.0. `ml_abi_version()` returns
`0x00010001` (major in the high 16 bits, minor in the low 16 bits).
`ml_leveldb_version()` reports 1.23. Returned version strings have static lifetime.
Clients must require the ABI major they understand and feature-test optional
capabilities. Unknown feature IDs return false.

ABI 1.1 adds `ml_db_create`, with the same signature and ownership as
`ml_db_open`. It forces create-if-missing and error-if-exists on copied OPEN
settings, returning an open handle only for a new database. Existing databases
fail with INVALID_ARGUMENT; lock acquisition may fail first. It does not create
parent directories. ABI 1.0 symbols and behavior remain available; adapters
using the new factory require a matching ABI 1.1 native library.

ABI 1 uses opaque option objects instead of public parameter structs. There is
therefore no exposed parameter layout to enlarge, align or size-check. Adding
option IDs, feature IDs and functions is compatible; existing identifiers,
function signatures and meanings are frozen. If a future function needs a
public struct, it must take explicit `struct_size` and `version` fields, reject
undersized inputs before accessing fields, and use a new versioned symbol for
incompatible changes. New languages do not change the ABI version.

## Status and errors

Every fallible operation returns a status and accepts optional `ml_error**`.
The error output is initialized to NULL. Pass a fresh output slot, or free its
previous error before reusing it. The caller owns returned errors; obtain the
borrowed, NUL-terminated UTF-8 message with `ml_error_message`, then call
`ml_error_free`. If error allocation itself fails, the nonzero status still
indicates failure but the error pointer can be NULL. C++ exceptions are caught
inside exported fallible operations.

Stable codes: 0 OK; 1 NOT_FOUND; 2 INVALID_ARGUMENT; 3 IO_ERROR; 4 CORRUPTION;
5 DATABASE_LOCKED; 6 DATABASE_CLOSED; 7 UNSUPPORTED; 8 INTERNAL_ERROR.

The original engine Status string is retained. Unknown I/O failures stay
IO_ERROR. Windows lock/share violations and known POSIX engine lock diagnostics
map to DATABASE_LOCKED. Unrecognized lock diagnostics remain IO_ERROR rather
than inventing a precise cause. A missing database without create-if-missing is
LevelDB INVALID_ARGUMENT; missing records are NOT_FOUND.

## Buffers

Inputs are borrowed for the call and may contain any bytes. NULL is permitted
only for zero-length binary input. Path bytes must be nonempty strict UTF-8,
without embedded NUL. The library never decodes keys or values. LevelDB's
32-bit encoded lengths limit keys to UINT32_MAX minus the eight-byte internal
suffix, and values to UINT32_MAX. Oversized inputs are rejected before reading
their data. Language/runtime allocation capacities may impose lower limits.

Successful get/key/value calls allocate an independent copy, including a
non-NULL allocation for a zero-length value. Output pointer/length slots are
reset to NULL/zero on failure. Free copies only with `ml_buffer_free`, including
iterator copies. They remain valid after iterator movement or destruction.
No iterator-owned data pointer escapes the ABI. NOT_FOUND never means empty.

The output slots themselves must be valid, distinct writable objects. As with
ordinary C libraries, arbitrary invalid pointers, forged handles, overlapping
output slots and use-after-destroy are caller contract violations. Pointer
validity cannot be inferred from a raw address.

## Handle lifetime and threads

All destroy/release/free functions accept NULL. Each non-NULL owned handle must
be destroyed exactly once. `ml_db_close` is idempotent and retains the allocated
handle; operations on that closed handle return DATABASE_CLOSED. Finally call
`ml_db_destroy`. Destroy also closes an open database. Reusing a pointer after
destroy is invalid. Managed adapters make repeated close harmless.

Database operations and close may run concurrently on a live handle. Operations
that already acquired database state may finish after close. Destroy requires
exclusive access to that handle. All dependent iterators and snapshots retain
the engine, including its lock. Snapshot handles can be released after creating
an iterator: that iterator retains the snapshot state independently.

Snapshots are immutable; concurrently reading an existing snapshot is safe.
Release of a particular snapshot handle must not race with a call using that
handle. Each iterator, batch and options object requires external serialization
when mutated, used or destroyed. Different iterators may be used concurrently.
Options are copied into engine operation settings; they can be freed after the
call. A snapshot must originate from the same database state as its read.

## Options and cursors

Create options with kind OPEN=0, READ=1 or WRITE=2. NULL options select LevelDB
defaults. Setters reject fields of another kind, non-boolean boolean values,
integer overflow, invalid compression and unknown fields. Numeric engine tuning
values may be clamped by LevelDB itself. Compression NONE=0, SNAPPY=1; official
Snappy is included in the default build.

Open fields 1..9 are create-if-missing, error-if-exists, paranoid-checks,
write-buffer-size, max-open-files, block-size, block-restart-interval,
compression and cache-size. Read fields 10/11 are verify-checksums/fill-cache.
Write field 12 is sync. Cache size zero creates an LRU cache with zero capacity.

Position a new cursor with a seek before reading from it. A seek can
succeed with no matching record, leaving the cursor invalid. Call
`ml_iterator_valid` before requesting the key or value. Calling next or
previous on an invalid cursor returns `ML_NOT_FOUND`. At the end of the
scan, call `ml_iterator_status` to check for an engine error.
