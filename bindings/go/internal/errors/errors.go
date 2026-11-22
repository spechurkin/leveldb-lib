package errors

import (
	"fmt"
)

// Code is a stable native status identifier.
type Code int32

const (
	// OK means success.
	OK Code = iota
	// NotFound means an absent key or invalid cursor position.
	NotFound
	// InvalidArgument means an invalid path, option, length or foreign snapshot.
	InvalidArgument
	// IOError is an engine/filesystem failure.
	IOError
	// Corruption means malformed engine data.
	Corruption
	// DatabaseLocked means identified directory lock contention.
	DatabaseLocked
	// DatabaseClosed means a closed or uninitialized resource was used.
	DatabaseClosed
	// Unsupported means an unknown setting or unavailable capability.
	Unsupported
	// InternalError means an unexpected native failure.
	InternalError
)

// Error preserves a native Code and the original engine Message.
type Error struct {
	// Code is the stable native status.
	Code Code
	// Message is the original native diagnostic.
	Message string
}

// Error formats the stable numeric code and diagnostic.
func (e *Error) Error() string { return fmt.Sprintf("multileveldb (%d): %s", e.Code, e.Message) }
