use std::fmt;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
/// Stable native status categories, excluding success.
pub enum ErrorCode {
    /// An iterator has no current record; missing database gets become `None`.
    NotFound = 1,
    /// An argument, option, UTF-8 path or snapshot/database pairing is invalid.
    InvalidArgument = 2,
    /// Filesystem I/O failed, including unclassified lock errors.
    IoError = 3,
    /// The engine detected malformed or corrupt stored data.
    Corruption = 4,
    /// The engine identified contention for the database directory's lock.
    DatabaseLocked = 5,
    /// An operation used a logically closed database.
    DatabaseClosed = 6,
    /// The loaded ABI or a requested capability is unsupported.
    Unsupported = 7,
    /// An unexpected native exception or unknown native status occurred.
    InternalError = 8,
}
#[derive(Debug)]
/// A native failure with its stable category and original diagnostic.
///
/// Implements [`std::error::Error`] and formats as `Category: message`.
pub struct Error {
    /// Machine-readable failure category; do not classify by message text.
    pub code: ErrorCode,
    /// Owned diagnostic copied before the native error is freed.
    pub message: String,
}
impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}: {}", self.code, self.message)
    }
}
impl std::error::Error for Error {}
/// The result of a fallible library operation.
pub type Result<T> = std::result::Result<T, Error>;
pub(super) fn error(code: ErrorCode, message: &str) -> Error {
    Error {
        code,
        message: message.into(),
    }
}
