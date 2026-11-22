use crate::{
    error::error,
    ffi::{self, Handle},
    Error, ErrorCode, Result,
};
use std::{ffi::CStr, ptr};

fn check(status: i32, e: Handle) -> Result<()> {
    // The native error owns its NUL-terminated message until ml_error_free.
    let message = unsafe {
        let message = if e.is_null() {
            String::new()
        } else {
            CStr::from_ptr(ffi::ml_error_message(e))
                .to_string_lossy()
                .into_owned()
        };
        ffi::ml_error_free(e);
        message
    };
    if status == 0 {
        return Ok(());
    }
    let code = match status {
        1 => ErrorCode::NotFound,
        2 => ErrorCode::InvalidArgument,
        3 => ErrorCode::IoError,
        4 => ErrorCode::Corruption,
        5 => ErrorCode::DatabaseLocked,
        6 => ErrorCode::DatabaseClosed,
        7 => ErrorCode::Unsupported,
        _ => ErrorCode::InternalError,
    };
    Err(Error { code, message })
}
pub(super) fn call(f: impl FnOnce(*mut Handle) -> i32) -> Result<()> {
    let mut e = ptr::null_mut();
    let status = f(&mut e);
    check(status, e)
}
pub(super) fn create(f: impl FnOnce(*mut Handle, *mut Handle) -> i32) -> Result<Handle> {
    if unsafe { ffi::ml_abi_version() } >> 16 != 1 {
        return Err(error(ErrorCode::Unsupported, "Incompatible native ABI"));
    }
    let mut h = ptr::null_mut();
    call(|e| f(&mut h, e))?;
    Ok(h)
}
pub(super) fn buffer(
    f: impl FnOnce(*mut *mut u8, *mut usize, *mut Handle) -> i32,
) -> Result<Vec<u8>> {
    let mut p = ptr::null_mut();
    let mut n = 0;
    call(|e| f(&mut p, &mut n, e))?;
    // Successful C calls allocate even for empty values; free with the same allocator.
    unsafe {
        let bytes = std::slice::from_raw_parts(p, n).to_vec();
        ffi::ml_buffer_free(p);
        Ok(bytes)
    }
}
