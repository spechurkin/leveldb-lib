use crate::{
    error::error,
    ffi::{self, Handle},
    native::{buffer, call, create},
    Batch, DbIterator, ErrorCode, OpenOptions, ReadOptions, Result, Snapshot, WriteOptions,
};
use std::{marker::PhantomData, path::Path, ptr};

/// An owned database handle; drops the engine reference when it leaves scope.
///
/// `Send + Sync` permits concurrent operations on the same database. Engine
/// directory locks prevent independently reopening it while a retained engine
/// is alive. Closing does not delete the database directory or its records.
pub struct Database {
    handle: Handle,
}
// LevelDB supports concurrent DB operations. The C handle serializes close/state access.
unsafe impl Send for Database {}
unsafe impl Sync for Database {}
impl Database {
    /// Create a new LevelDB directory and return its open handle.
    ///
    /// Parent directories must exist. Storage settings are copied without
    /// changing `options`; create-if-missing and error-if-exists are forced true.
    /// `None` selects the remaining engine defaults. Paths follow [`Self::open`].
    /// Requires native ABI 1.1 or later.
    ///
    /// # Errors
    /// Returns `InvalidArgument` for an existing database or invalid path/options,
    /// `DatabaseLocked` for identified contention (which may be checked first),
    /// or the same native errors as [`Self::open`].
    pub fn create(path: impl AsRef<Path>, options: Option<&OpenOptions>) -> Result<Self> {
        let path = path
            .as_ref()
            .to_str()
            .ok_or_else(|| error(ErrorCode::InvalidArgument, "Path is not Unicode"))?
            .as_bytes();
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        let handle =
            create(|h, e| unsafe { ffi::ml_db_create(path.as_ptr(), path.len(), o, h, e) })?;
        Ok(Self { handle })
    }
    /// Open an existing database, or create it when configured to do so.
    ///
    /// `path` must be nonempty Unicode without NUL. Relative paths use the
    /// process's working directory. `options: None` selects engine defaults;
    /// supplied options are copied and need not outlive the call.
    ///
    /// # Errors
    /// Returns `InvalidArgument` for invalid paths/options or a missing database
    /// when creation is disabled, `DatabaseLocked` for identified contention,
    /// `Unsupported` for another ABI major, or the engine's I/O/corruption error.
    pub fn open(path: impl AsRef<Path>, options: Option<&OpenOptions>) -> Result<Self> {
        let path = path
            .as_ref()
            .to_str()
            .ok_or_else(|| error(ErrorCode::InvalidArgument, "Path is not Unicode"))?
            .as_bytes();
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        let handle = create(|h, e| unsafe { ffi::ml_db_open(path.as_ptr(), path.len(), o, h, e) })?;
        Ok(Self { handle })
    }
    /// Logically close this handle without deleting stored data; repeatable.
    ///
    /// Further operations return `DatabaseClosed`. Exclusive borrowing prevents
    /// closing while this Rust handle is borrowed by a snapshot or iterator.
    /// Drop later frees the native handle. Native failures return [`Error`](crate::Error).
    pub fn close(&mut self) -> Result<()> {
        call(|e| unsafe { ffi::ml_db_close(self.handle, e) })
    }
    /// Read `key`, returning an owned value, or `None` when the key is absent.
    ///
    /// An existing zero-byte value is `Some(Vec::new())`. `options: None` uses
    /// default read settings. `snapshot: None` reads the current view; a snapshot
    /// must belong to this exact database instance. No text decoding occurs.
    ///
    /// # Errors
    /// Returns `InvalidArgument` for an oversized key/foreign snapshot,
    /// `DatabaseClosed` after close, or an engine I/O/corruption/native error.
    pub fn get(
        &self,
        key: &[u8],
        options: Option<&ReadOptions>,
        snapshot: Option<&Snapshot<'_>>,
    ) -> Result<Option<Vec<u8>>> {
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        let s = snapshot.map_or(ptr::null_mut(), |s| s.handle);
        match buffer(|p, n, e| unsafe {
            ffi::ml_db_get(self.handle, key.as_ptr(), key.len(), o, s, p, n, e)
        }) {
            Ok(v) => Ok(Some(v)),
            Err(e) if e.code == ErrorCode::NotFound => Ok(None),
            Err(e) => Err(e),
        }
    }
    /// Insert or replace `key` with exactly `value`; empty slices are valid.
    ///
    /// Input slices are borrowed only for this call. `None` uses nonsynchronous
    /// engine write defaults. Returns `InvalidArgument` for engine length
    /// overflow, `DatabaseClosed`, or an engine/native write failure.
    pub fn put(&self, key: &[u8], value: &[u8], options: Option<&WriteOptions>) -> Result<()> {
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        call(|e| unsafe {
            ffi::ml_db_put(
                self.handle,
                key.as_ptr(),
                key.len(),
                value.as_ptr(),
                value.len(),
                o,
                e,
            )
        })
    }
    /// Delete `key`, succeeding also when it was already absent.
    ///
    /// `None` uses default write settings. Returns `InvalidArgument` for an
    /// oversized key, `DatabaseClosed`, or an engine/native write failure.
    pub fn delete(&self, key: &[u8], options: Option<&WriteOptions>) -> Result<()> {
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        call(|e| unsafe { ffi::ml_db_delete(self.handle, key.as_ptr(), key.len(), o, e) })
    }
    /// Atomically apply the batch's operations in insertion order.
    ///
    /// The batch is neither consumed nor cleared and may be reused with another
    /// database. `None` uses default write settings. This supplies no read or
    /// conflict-check transaction. Returns `DatabaseClosed` or a write failure.
    pub fn write(&self, batch: &Batch, options: Option<&WriteOptions>) -> Result<()> {
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        call(|e| unsafe { ffi::ml_batch_write(self.handle, batch.handle, o, e) })
    }
    /// Capture the engine's current committed read view, borrowing this database.
    ///
    /// Pass it to [`Self::get`] or [`Self::iterator`]. Holding a snapshot can
    /// retain older table versions. Returns `DatabaseClosed` or a native error.
    pub fn snapshot(&self) -> Result<Snapshot<'_>> {
        let handle = create(|h, e| unsafe { ffi::ml_snapshot_create(self.handle, h, e) })?;
        Ok(Snapshot {
            handle,
            _db: PhantomData,
        })
    }
    /// Create an initially unpositioned, streaming cursor over this database.
    ///
    /// `None` options select read defaults. An optional snapshot must belong to
    /// this instance; its native state is retained independently, so its Rust
    /// wrapper may be dropped after this call. The cursor borrows the database.
    /// Seek before accessing records; order is LevelDB's bytewise comparator.
    ///
    /// # Errors
    /// Returns `InvalidArgument` for a foreign snapshot, `DatabaseClosed`, or
    /// the engine/native error encountered while creating the cursor.
    pub fn iterator(
        &self,
        options: Option<&ReadOptions>,
        snapshot: Option<&Snapshot<'_>>,
    ) -> Result<DbIterator<'_>> {
        let o = options.map_or(ptr::null_mut(), |o| o.0 .0);
        let s = snapshot.map_or(ptr::null_mut(), |s| s.handle);
        let handle = create(|h, e| unsafe { ffi::ml_iterator_create(self.handle, o, s, h, e) })?;
        Ok(DbIterator {
            handle,
            _db: PhantomData,
        })
    }
}
impl Drop for Database {
    fn drop(&mut self) {
        unsafe { ffi::ml_db_destroy(self.handle) }
    }
}
