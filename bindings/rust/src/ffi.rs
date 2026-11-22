//! Private C ABI declarations. Function contracts and pointer ownership are
//! defined in `native/include/multileveldb.h`; this module is not exported.
//! Callers keep inputs/handles alive, serialize resource destruction, check
//! every status, and free successful buffers/errors with the native allocator.
use std::ffi::{c_char, c_void};
pub type Handle = *mut c_void;
#[link(name = "multileveldb")]
extern "C" {
    pub fn ml_abi_version() -> u32;
    pub fn ml_error_message(e: Handle) -> *const c_char;
    pub fn ml_error_free(e: Handle);
    pub fn ml_buffer_free(p: *mut u8);
    pub fn ml_options_create(kind: u32, out: *mut Handle, e: *mut Handle) -> i32;
    pub fn ml_options_set(o: Handle, field: u32, value: u64, e: *mut Handle) -> i32;
    pub fn ml_options_destroy(o: Handle);
    pub fn ml_db_open(p: *const u8, n: usize, o: Handle, out: *mut Handle, e: *mut Handle) -> i32;
    pub fn ml_db_create(p: *const u8, n: usize, o: Handle, out: *mut Handle, e: *mut Handle)
        -> i32;
    pub fn ml_db_close(db: Handle, e: *mut Handle) -> i32;
    pub fn ml_db_destroy(db: Handle);
    pub fn ml_db_get(
        db: Handle,
        k: *const u8,
        n: usize,
        o: Handle,
        s: Handle,
        out: *mut *mut u8,
        len: *mut usize,
        e: *mut Handle,
    ) -> i32;
    pub fn ml_db_put(
        db: Handle,
        k: *const u8,
        n: usize,
        v: *const u8,
        z: usize,
        o: Handle,
        e: *mut Handle,
    ) -> i32;
    pub fn ml_db_delete(db: Handle, k: *const u8, n: usize, o: Handle, e: *mut Handle) -> i32;
    pub fn ml_iterator_create(
        db: Handle,
        o: Handle,
        s: Handle,
        out: *mut Handle,
        e: *mut Handle,
    ) -> i32;
    pub fn ml_iterator_destroy(i: Handle);
    pub fn ml_iterator_seek_first(i: Handle, e: *mut Handle) -> i32;
    pub fn ml_iterator_seek_last(i: Handle, e: *mut Handle) -> i32;
    pub fn ml_iterator_seek(i: Handle, k: *const u8, n: usize, e: *mut Handle) -> i32;
    pub fn ml_iterator_next(i: Handle, e: *mut Handle) -> i32;
    pub fn ml_iterator_previous(i: Handle, e: *mut Handle) -> i32;
    pub fn ml_iterator_valid(i: Handle) -> u32;
    pub fn ml_iterator_key(i: Handle, out: *mut *mut u8, n: *mut usize, e: *mut Handle) -> i32;
    pub fn ml_iterator_value(i: Handle, out: *mut *mut u8, n: *mut usize, e: *mut Handle) -> i32;
    pub fn ml_iterator_status(i: Handle, e: *mut Handle) -> i32;
    pub fn ml_batch_create(out: *mut Handle, e: *mut Handle) -> i32;
    pub fn ml_batch_destroy(b: Handle);
    pub fn ml_batch_clear(b: Handle, e: *mut Handle) -> i32;
    pub fn ml_batch_put(
        b: Handle,
        k: *const u8,
        n: usize,
        v: *const u8,
        z: usize,
        e: *mut Handle,
    ) -> i32;
    pub fn ml_batch_delete(b: Handle, k: *const u8, n: usize, e: *mut Handle) -> i32;
    pub fn ml_batch_write(db: Handle, b: Handle, o: Handle, e: *mut Handle) -> i32;
    pub fn ml_snapshot_create(db: Handle, out: *mut Handle, e: *mut Handle) -> i32;
    pub fn ml_snapshot_release(s: Handle);
}
