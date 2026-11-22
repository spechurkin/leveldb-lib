//! Binary-safe access to Google LevelDB through C ABI 1.
//!
//! Install the native shared library and set `ML_NATIVE_DIR` before linking.
//! Keys and values are arbitrary bytes; paths must be Unicode. Missing records
//! are [`None`], while existing empty values are `Some(Vec::new())`.
//!
//! All resources use RAII. A [`Database`] supports concurrent reads and writes;
//! mutable cursor/batch operations require exclusive access. Database snapshots
//! and iterators borrow their originating database, preventing premature close.
//! See each operation's errors for native failures; diagnostics remain in
//! [`Error::message`]. Returned vectors are independent owned copies.
//!
//! # Example
//! ```no_run
//! use multileveldb::{Database, OpenOptions, Result};
//! # fn main() -> Result<()> {
//! let mut options = OpenOptions::new()?;
//! options.create_if_missing(true)?;
//! let db = Database::open("data", Some(&options))?;
//! db.put(&[0, 255], &[0, 128], None)?;
//! assert_eq!(db.get(&[0, 255], None, None)?, Some(vec![0, 128]));
//! # Ok(()) }
//! ```
#![warn(missing_docs)]
mod batch;
mod database;
mod error;
mod ffi;
mod iterator;
mod native;
mod options;
mod snapshot;

pub use batch::Batch;
pub use database::Database;
pub use error::{Error, ErrorCode, Result};
pub use iterator::DbIterator;
pub use options::{Compression, OpenOptions, ReadOptions, WriteOptions};
pub use snapshot::Snapshot;
