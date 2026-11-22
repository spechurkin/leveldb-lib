use crate::{
    ffi::{self, Handle},
    native::{call, create},
    Result,
};

/// An owned sequence of put/delete operations for an atomic [`Database::write`](crate::Database::write).
///
/// Batches are independent of a database, reusable after write, and freed on
/// drop. They are `Send`, not `Sync`; mutations require exclusive borrowing.
/// Adding an operation does not write anything until a database applies it.
pub struct Batch {
    pub(super) handle: Handle,
}
unsafe impl Send for Batch {}
impl Batch {
    /// Allocate an empty batch; returns `Unsupported` for another ABI major or a
    /// native allocation error. No resource call occurs before the ABI check.
    pub fn new() -> Result<Self> {
        Ok(Self {
            handle: create(|h, e| unsafe { ffi::ml_batch_create(h, e) })?,
        })
    }
    /// Append a put, copying both input slices, and return this batch for chaining.
    ///
    /// Empty keys/values are supported. Returns `InvalidArgument` for LevelDB's
    /// encoded-length overflow or a native allocation error. Does not write yet.
    pub fn put(&mut self, key: &[u8], value: &[u8]) -> Result<&mut Self> {
        call(|e| unsafe {
            ffi::ml_batch_put(
                self.handle,
                key.as_ptr(),
                key.len(),
                value.as_ptr(),
                value.len(),
                e,
            )
        })?;
        Ok(self)
    }
    /// Append a deletion, copy `key`, and return this batch for chaining.
    /// An absent key is harmless when applied. Oversized keys return
    /// `InvalidArgument`; native allocation failures are returned as [`Error`](crate::Error).
    pub fn delete(&mut self, key: &[u8]) -> Result<&mut Self> {
        call(|e| unsafe { ffi::ml_batch_delete(self.handle, key.as_ptr(), key.len(), e) })?;
        Ok(self)
    }
    /// Remove all queued operations without modifying any database.
    /// The same batch remains reusable. Native failures return [`Error`](crate::Error).
    pub fn clear(&mut self) -> Result<()> {
        call(|e| unsafe { ffi::ml_batch_clear(self.handle, e) })
    }
}
impl Drop for Batch {
    fn drop(&mut self) {
        unsafe { ffi::ml_batch_destroy(self.handle) }
    }
}
