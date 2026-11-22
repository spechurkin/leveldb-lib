use crate::{
    ffi::{self, Handle},
    native::{call, create},
    Result,
};

pub(super) struct Options(pub(super) Handle);
impl Options {
    fn new(kind: u32) -> Result<Self> {
        Ok(Self(create(|h, e| unsafe {
            ffi::ml_options_create(kind, h, e)
        })?))
    }
    fn set(&mut self, field: u32, value: u64) -> Result<&mut Self> {
        call(|e| unsafe { ffi::ml_options_set(self.0, field, value, e) })?;
        Ok(self)
    }
}
impl Drop for Options {
    fn drop(&mut self) {
        unsafe { ffi::ml_options_destroy(self.0) }
    }
}
// Options are immutable during calls; mutation requires exclusive Rust access.
unsafe impl Send for Options {}
unsafe impl Sync for Options {}
/// Opening settings, copied by [`Database::open`](crate::Database::open) and freed on drop.
///
/// Construct with [`Self::new`]; setters return the same mutable builder.
/// Only path/engine settings belong here, not snapshot or write durability.
pub struct OpenOptions(pub(super) Options);
/// Read settings, copied for each get/iterator creation and freed on drop.
///
/// Snapshots are passed separately to the database operation.
pub struct ReadOptions(pub(super) Options);
/// Write durability settings, copied for each write and freed on drop.
pub struct WriteOptions(pub(super) Options);
#[derive(Debug, Clone, Copy)]
/// The engine's block-compression setting for newly written tables.
///
/// Changing compression does not rewrite existing records or prevent reading
/// existing Snappy-compressed tables.
pub enum Compression {
    /// Write uncompressed blocks.
    None = 0,
    /// Use official Snappy compression, the engine default.
    Snappy = 1,
}
macro_rules! option {
    ($name:ident, $field:expr, bool, $doc:literal) => {
        #[doc = $doc]
        ///
        /// Returns this builder on success. Native validation/allocation
        /// failures are returned as [`Error`](crate::Error); the builder remains owned.
        pub fn $name(&mut self, value: bool) -> Result<&mut Self> {
            self.0.set($field, u64::from(value))?;
            Ok(self)
        }
    };
    ($name:ident, $field:expr, $ty:ty, $doc:literal) => {
        #[doc = $doc]
        ///
        /// Returns this builder on success. Invalid values return
        /// [`ErrorCode::InvalidArgument`](crate::ErrorCode::InvalidArgument); unavailable compression returns
        /// [`ErrorCode::Unsupported`](crate::ErrorCode::Unsupported). Engine tuning bounds may clamp values.
        pub fn $name(&mut self, value: $ty) -> Result<&mut Self> {
            self.0.set($field, value as u64)?;
            Ok(self)
        }
    };
}
impl OpenOptions {
    /// Allocate opening options initialized to LevelDB defaults.
    ///
    /// Creation is disabled, compression is Snappy, and the engine provides its
    /// own cache. Returns `Unsupported` for another ABI major, or a native
    /// allocation error if construction fails.
    pub fn new() -> Result<Self> {
        Ok(Self(Options::new(0)?))
    }
    option!(create_if_missing, 1, bool, "Set whether opening a missing directory creates a database. Default: false. Parent directories must already exist.");
    option!(error_if_exists, 2, bool, "Set whether opening an existing database fails. Default: false. Lock acquisition can fail before the existence check.");
    option!(paranoid_checks, 3, bool, "Enable additional engine consistency checks during opening/recovery. Default: false; this is separate from read checksum verification.");
    option!(write_buffer_size, 4, usize, "Set the positive in-memory write-buffer target in bytes. Default: 4 MiB; LevelDB may clamp the requested size.");
    option!(max_open_files, 5, u32, "Set the positive engine open-file budget. Default: 1000; the value must fit a C int, and engine minimum/maximum bounds apply.");
    option!(block_size, 6, usize, "Set the positive uncompressed table-block target in bytes. Default: 4096; this is not a maximum record size.");
    option!(block_restart_interval, 7, u32, "Set the positive number of keys between table-block restart points. Default: 16; the value must fit a C int.");
    option!(compression, 8, Compression, "Choose compression for newly written blocks. Default: Snappy. Existing table compression is unchanged.");
    option!(cache_size, 9, usize, "Install an owned LRU block cache with this byte capacity. Zero gives a zero-capacity cache; omitting this setter retains the engine's default cache policy.");
}
impl ReadOptions {
    /// Allocate read options with checksum verification off and cache filling on.
    ///
    /// Returns `Unsupported` for another ABI major or a native allocation error.
    pub fn new() -> Result<Self> {
        Ok(Self(Options::new(1)?))
    }
    option!(
        verify_checksums,
        10,
        bool,
        "Set whether the engine verifies checksums of blocks read from storage. Default: false."
    );
    option!(fill_cache, 11, bool, "Set whether this read/iterator populates the block cache. Default: true. False does not disable use of already cached blocks.");
}
impl WriteOptions {
    /// Allocate write options with synchronous flushing disabled.
    ///
    /// Returns `Unsupported` for another ABI major or a native allocation error.
    pub fn new() -> Result<Self> {
        Ok(Self(Options::new(2)?))
    }
    option!(sync, 12, bool, "Set whether writes request LevelDB's synchronous storage flush before success. Default: false. This preserves the engine's durability semantics, not a general transaction.");
}
