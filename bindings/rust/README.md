# multileveldb

Read and write LevelDB with a safe Rust API.

## Setup

Install the matching native archive or build the shared library from source.
Set `ML_NATIVE_DIR` to the directory containing the library or Windows
import library, then put the shared library on the platform loader path.
On Windows, match Cargo's GNU or MSVC target to the native build toolchain.

## Open a database and write bytes

```rust,no_run
use multileveldb::{Database, OpenOptions, Result};

fn main() -> Result<()> {
    let mut options = OpenOptions::new()?;
    options.create_if_missing(true)?;
    let db = Database::open("data", Some(&options))?;
    db.put(&[0, 255], &[0, 128], None)?;
    assert_eq!(db.get(&[0, 255], None, None)?, Some(vec![0, 128]));
    Ok(())
}
```

Keys and values are bytes. `get` returns `None` for a missing key and
`Some(Vec::new())` for an empty value. Handles release their resources on
drop. Snapshots and iterators borrow the database. You can share a
`Database` between threads (`Send + Sync`) and move an iterator to another
thread (`Send`).

## Require a new database

`Database::create("new-data", None)?` creates a new database with default storage
settings and returns its open handle. Pass `Some(&options)` for copied tuning
settings; creation always enables create-if-missing and error-if-exists.
Existing databases fail with `InvalidArgument`, or `DatabaseLocked` when lock
acquisition fails first. Parent directories must exist. Requires native ABI 1.1.
