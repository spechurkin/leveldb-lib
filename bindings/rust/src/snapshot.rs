use crate::{
    ffi::{self, Handle},
    Database,
};
use std::marker::PhantomData;

/// An immutable engine read view borrowing its database for `'db`.
///
/// `Send + Sync` allows concurrent snapshot reads. Drop releases this wrapper's
/// reference; snapshot iterators keep their own native reference until dropped.
/// There are no mutation methods: use it as a database read/iterator argument.
pub struct Snapshot<'db> {
    pub(super) handle: Handle,
    pub(super) _db: PhantomData<&'db Database>,
}
// Native snapshots are immutable and retain their originating engine.
unsafe impl Send for Snapshot<'_> {}
unsafe impl Sync for Snapshot<'_> {}
impl Drop for Snapshot<'_> {
    fn drop(&mut self) {
        unsafe { ffi::ml_snapshot_release(self.handle) }
    }
}
