# Python

## Setup

This binding uses `ctypes` from Python 3.10 or later. Install a native library
with ABI 1.1 and set `MULTILEVELDB_LIBRARY` to its absolute path before your
first database call. If the variable is unset, Python uses the platform
loader's search path.

```sh
python -m pip install ./bindings/python
```

From source, set `PYTHONPATH=bindings/python`. On Windows place compiler runtime
DLLs alongside multileveldb.dll, or register their directory
with `os.add_dll_directory()` and keep that directory handle alive. Python's
Windows dependency loader does not search PATH. On Linux/macOS configure the
native loader path as described in [BUILDING](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md).

## Open a database

```python
from multileveldb import Database, Batch

with Database.create("data", compression="snappy") as db:
    db.put(b"\0\xff", b"\0\x80\xff", {"sync": True})
    assert db.get(b"\0\xff") == b"\0\x80\xff"
    assert db.get(b"absent") is None
    with db.create_snapshot() as snapshot, Batch() as batch:
        batch.put(b"\0\xff", b"").delete(b"obsolete")
        db.write(batch)
        assert db.get(b"\0\xff", snapshot=snapshot) == b"\0\x80\xff"
    with db.iterator() as cursor:
        cursor.seek_to_first()
        while cursor.is_valid():
            key, value = cursor.key(), cursor.value()
            cursor.next()
        cursor.status()
```

`Database.create(path, **open_options)` creates a new standard LevelDB, forcing
create-if-missing and error-if-exists without changing the caller's dictionary.
The method returns an open handle or raises an error if the database exists.
Use `Database.open(path, **open_options)` to open an existing database. Pass
`create_if_missing=True` if either outcome is fine. Both methods accept a
`str` or text `PathLike` path that encodes as strict UTF-8. Create the parent
directories first. Empty paths and paths containing NUL raise an error.

Keys/values accept `bytes`, `bytearray` or `memoryview`; text is rejected. Inputs
are copied to immutable bytes before use. Outputs are independent `bytes` that
survive cursor movement and close. `get` returns `None` only for missing keys;
`b''` is an existing empty value. Empty keys are valid.

## Methods and resource lifetime

The public API is `Database`, `Batch`, `Snapshot`, `Cursor`, `Error`:

- Database: `create`, `open`, `get(key, read_options=None, snapshot=None)`,
  `put(key, value, write_options=None)`, `delete(key, write_options=None)`,
  `write(batch, write_options=None)`, `create_snapshot()`, `iterator(read_options=None, snapshot=None)`.
- `Batch()` allocates a database-independent batch. `put`/`delete` copy inputs
  and return the batch for chaining; `clear()` discards pending operations.
  `write` applies operations atomically in order without consuming the batch.
- Cursor: `seek_to_first`, `seek_to_last`, `seek(key)` (first key >= target),
  `next`, `previous`, `is_valid`, `key`, `value`, `status`. New cursors are
  unpositioned. Moving past an edge succeeds and invalidates the cursor;
  moving/accessing an already invalid cursor raises `NOT_FOUND`. Check `status`
  after a scan; `is_valid` alone does not report engine errors.
- Snapshots are native same-instance read views. A foreign snapshot fails with
  `INVALID_ARGUMENT`. Existing cursors retain their own snapshot/engine reference.
- All resources implement idempotent `close()` and `with` cleanup. Further
  operations raise `DATABASE_CLOSED`. A fallback weakref finalizer releases
  abandoned handles; explicit close is needed for prompt directory lock release.

## Options

Opening keyword names: `create_if_missing=False`, `error_if_exists=False`,
`paranoid_checks=False`, `write_buffer_size=4194304`, `max_open_files=1000`,
`block_size=4096`, `block_restart_interval=16`, `compression='snappy'`
(`'none'` also accepted), `cache_size` (unset uses engine policy; zero selects
zero capacity). Read dictionaries accept `verify_checksums=False` and
`fill_cache=True`; write dictionaries accept `sync=False`. Numeric values must
be uint64 integers; booleans accept bool or 0/1. Wrong-kind/unknown names are
rejected. Engine bounds may clamp tuning values; sizes/counts must fit native
types and be positive except cache capacity.

All calls and resource cleanup share one reentrant adapter lock, including calls
from different Python threads. Engine work blocks the caller. Cursors stream
records; snapshots can retain older table versions and directory locks even
after DB close. WriteBatch provides atomic writes, without transactional reads.

## Errors

Native failures raise `Error`, exposing numeric `status`, stable `code` and the
original `message`. Codes: `NOT_FOUND`, `INVALID_ARGUMENT`, `IO_ERROR`,
`CORRUPTION`, `DATABASE_LOCKED`, `DATABASE_CLOSED`, `UNSUPPORTED`,
`INTERNAL_ERROR`. Only missing `get` maps NOT_FOUND to None. Python argument
shape errors use TypeError/ValueError; malformed path text raises UnicodeError.
Missing/incompatible native code raises a loader error/RuntimeError on first use.
Native output/error allocations are freed after copying, even if copying fails.

## Tests and API reference

```sh
python -m unittest discover -s bindings/python/tests -v
python -m pydoc multileveldb
python -m pydoc -w multileveldb
python -m pip wheel --no-deps ./bindings/python -w build/packages
```

Run HTML generation in `build/docs/python` with an absolute PYTHONPATH to keep
generated files outside source. Source docstrings describe every public method;
the wheel includes them and LICENSE. The wheel does not bundle the native engine.
