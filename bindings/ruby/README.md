# Ruby

## Setup

Use Ruby 3.1 or later and Fiddle 1.1 or later. If your Ruby distribution
omits Fiddle, install it with `gem install fiddle`. Install a matching native
ABI 1.1 library and set `MULTILEVELDB_LIBRARY` to its absolute path. The gem
loads the library on the first database or resource call.

```sh
cd bindings/ruby
gem build multileveldb.gemspec
gem install ./multileveldb-1.0.0.gem
```

From source use `ruby -I bindings/ruby/lib your_script.rb`. On Windows place
compiler runtime DLLs next to the selected library. The adapter temporarily
registers that directory for Windows dependency loading. On Linux/macOS use the
platform loader path described in [BUILDING](https://github.com/spechurkin/leveldb-lib/blob/HEAD/docs/BUILDING.md).

## Open a database

```ruby
require 'multileveldb'

MultiLevelDB::Database.create('data', compression: :snappy) do |db|
  db.put("\0\xff".b, "\0\x80\xff".b, sync: true)
  raise 'missing' unless db.get("\0\xff".b) == "\0\x80\xff".b
  db.create_snapshot.use do |snapshot|
    MultiLevelDB::Batch.new.use do |batch|
      batch.put("\0\xff".b, ''.b).delete('obsolete'.b)
      db.write(batch)
      raise 'snapshot' unless db.get("\0\xff".b, snapshot: snapshot) == "\0\x80\xff".b
    end
  end
  db.iterator.use do |cursor|
    cursor.seek_to_first
    while cursor.valid?
      key, value = cursor.key, cursor.value
      cursor.next
    end
    cursor.status
  end
end
```

`Database.create(path, **open_options)` returns an open new database and fails
if one exists; both creation/existence flags are forced without mutating the
options Hash. `Database.open(path, **open_options)` uses engine defaults,
including creation disabled. Both accept an optional block, ensuring close on
return/exception and returning the block result. Without a block they return an
owned Database. Paths are Strings encoded as strict UTF-8; parents must exist.
Invalid encoding, empty paths and embedded NUL fail. No data is deleted by close.

Keys/values are Strings whose raw bytes are copied without text transcoding;
use `.b` for explicit application binary data. Returned Strings use ASCII-8BIT
and remain valid after movement/close. `get` returns nil only for absent keys;
`''.b` is an existing empty value. Empty keys are valid.

## Methods and resource lifetime

Public methods:

- Database: `create`, `open`, `get(key, read_options: {}, snapshot: nil)`,
  `put(key, value, **write_options)`, `delete(key, **write_options)`,
  `write(batch, **write_options)`, `create_snapshot`,
  `iterator(read_options: {}, snapshot: nil)`.
- `Batch.new`: allocate an empty, database-independent batch. `put`/`delete`
  copy inputs and return self; `clear` discards pending operations. `write`
  atomically applies the live batch in order without consuming it.
- Cursor: `seek_to_first`, `seek_to_last`, `seek(key)` (first key >= target),
  `next`, `previous`, `valid?`, `key`, `value`, `status`. Cursors are initially
  unpositioned. Empty views/no seek match are successful invalid positions.
  Moving past an edge succeeds; moving/accessing an already invalid cursor
  raises NOT_FOUND. `valid?` does not check engine errors; finish with `status`.
- Snapshot: obtain from `create_snapshot`, pass only to the originating DB.
  Foreign snapshots fail INVALID_ARGUMENT. Existing cursors retain their view
  independently after closing either the DB or Snapshot wrapper.
- Resource: all resources inherit idempotent `close` and `use { |resource| ... }`,
  which ensures cleanup and returns the block result. Closed operations raise
  DATABASE_CLOSED. Finalizers are fallback cleanup; close promptly for locks.

## Options

Open options use symbol names: `create_if_missing: false`, `error_if_exists:
false`, `paranoid_checks: false`, `write_buffer_size: 4194304`, `max_open_files:
1000`, `block_size: 4096`, `block_restart_interval: 16`, `compression: :snappy`
(or `:none`), `cache_size:` (unset engine policy; zero means zero capacity).
Read options accept `verify_checksums: false`, `fill_cache: true`. Write options
accept `sync: false`. Integers must fit uint64; booleans accept true/false or
0/1. Wrong-kind/unknown settings and malformed values fail. Native numeric
ranges apply; engine bounds can clamp accepted tuning values.

All resource calls and cleanup share one Monitor across Ruby threads. Operations
are synchronous; DB work blocks the caller. Cursors stream records and can
retain directory locks/old engine versions. Batches provide atomic writes,
without transactional reads or conflict detection.

## Errors

Native failures raise `MultiLevelDB::Error` with numeric `status`, stable `code`
and original diagnostic via `message`. Codes: NOT_FOUND, INVALID_ARGUMENT,
IO_ERROR, CORRUPTION, DATABASE_LOCKED, DATABASE_CLOSED, UNSUPPORTED,
INTERNAL_ERROR. Only a missing `get` becomes nil. Wrong Ruby shapes use
TypeError/ArgumentError; encoding failures use Encoding errors. Missing native
code/symbols raise Fiddle::DLError; incompatible ABI raises LoadError. Native
buffers/errors are freed after copying, including conversion failures.

## Tests and API reference

```sh
ruby -I bindings/ruby/lib bindings/ruby/test/database_test.rb
rdoc --op build/docs/ruby bindings/ruby/lib bindings/ruby/README.md
```

All public methods have RDoc comments, included in the gem with README and
LICENSE. The gem does not bundle the native engine.
