use crate::{
    ffi::{self, Handle},
    native::{buffer, call},
    Database, Result,
};
use std::marker::PhantomData;

/// A streaming bidirectional cursor borrowing a database for `'db`.
///
/// Created unpositioned. `Send` permits moving it between threads; it is not
/// `Sync`. Mutating movement requires `&mut self`. Drop releases the cursor and
/// its retained engine/snapshot. Key/value methods return independent copies.
pub struct DbIterator<'db> {
    pub(super) handle: Handle,
    pub(super) _db: PhantomData<&'db Database>,
}
// Moving an iterator is safe; accessing it concurrently is deliberately unsupported.
unsafe impl Send for DbIterator<'_> {}
impl DbIterator<'_> {
    /// Seek to the first record, or become invalid when the view is empty.
    ///
    /// Empty views are successful; check [`Self::is_valid`]. Engine I/O and
    /// corruption failures are returned as [`Error`](crate::Error).
    pub fn seek_to_first(&mut self) -> Result<()> {
        call(|e| unsafe { ffi::ml_iterator_seek_first(self.handle, e) })
    }
    /// Seek to the last record; an empty view succeeds with an invalid cursor.
    /// Returns engine I/O/corruption/native errors.
    pub fn seek_to_last(&mut self) -> Result<()> {
        call(|e| unsafe { ffi::ml_iterator_seek_last(self.handle, e) })
    }
    /// Seek to the first key greater than or equal to `key` in bytewise order.
    ///
    /// No qualifying key is a successful invalid position. Returns
    /// `InvalidArgument` for engine key-length overflow or an engine/native error.
    pub fn seek(&mut self, key: &[u8]) -> Result<()> {
        call(|e| unsafe { ffi::ml_iterator_seek(self.handle, key.as_ptr(), key.len(), e) })
    }
    /// Advance one record; moving past the last record succeeds and invalidates.
    ///
    /// Returns `NotFound` if already invalid, or an engine/native error.
    /// Call [`Self::status`] after a scan ends to distinguish exhaustion/errors.
    pub fn next(&mut self) -> Result<()> {
        call(|e| unsafe { ffi::ml_iterator_next(self.handle, e) })
    }
    /// Move one record backward; moving before the first succeeds and invalidates.
    /// Returns `NotFound` if already invalid, or an engine/native error.
    pub fn previous(&mut self) -> Result<()> {
        call(|e| unsafe { ffi::ml_iterator_previous(self.handle, e) })
    }
    /// Whether the cursor currently refers to a record.
    ///
    /// False can mean unpositioned, exhausted or failed; it does not replace
    /// [`Self::status`] and does not itself report engine failures.
    pub fn is_valid(&self) -> bool {
        unsafe { ffi::ml_iterator_valid(self.handle) != 0 }
    }
    /// Return the engine cursor's error status without moving it.
    /// Normal exhaustion/unpositioned state is `Ok(())`; I/O/corruption is `Err`.
    pub fn status(&self) -> Result<()> {
        call(|e| unsafe { ffi::ml_iterator_status(self.handle, e) })
    }
    /// Copy the current binary key into an owned vector.
    /// The copy survives movement/drop. Returns `NotFound` when invalid, or the
    /// cursor's engine/native error.
    pub fn key(&self) -> Result<Vec<u8>> {
        buffer(|p, n, e| unsafe { ffi::ml_iterator_key(self.handle, p, n, e) })
    }
    /// Copy the current binary value, preserving a zero-length stored value.
    /// The copy survives movement/drop. Returns `NotFound` when invalid, or the
    /// cursor's engine/native error.
    pub fn value(&self) -> Result<Vec<u8>> {
        buffer(|p, n, e| unsafe { ffi::ml_iterator_value(self.handle, p, n, e) })
    }
}
impl Drop for DbIterator<'_> {
    fn drop(&mut self) {
        unsafe { ffi::ml_iterator_destroy(self.handle) }
    }
}
