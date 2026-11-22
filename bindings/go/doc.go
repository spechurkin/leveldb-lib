// Package multileveldb provides synchronous, binary-safe LevelDB access through
// C ABI 1. Install the native library/header and set CGO_CFLAGS/CGO_LDFLAGS before
// building. Calls and cleanup share a lock. Close resources promptly; finalizers
// are only a fallback. Never share one database directory between processes.
package multileveldb
