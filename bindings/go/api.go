package multileveldb

import (
	"github.com/spechurkin/leveldb-lib/bindings/go/internal/database"
	"github.com/spechurkin/leveldb-lib/bindings/go/internal/errors"
	"github.com/spechurkin/leveldb-lib/bindings/go/internal/options"
)

// Code is a stable native status identifier.
type Code = errors.Code

const (
	// OK means success.
	OK = errors.OK
	// NotFound means an absent key or invalid cursor position.
	NotFound = errors.NotFound
	// InvalidArgument means an invalid path, option, length or foreign snapshot.
	InvalidArgument = errors.InvalidArgument
	// IOError is an engine/filesystem failure.
	IOError = errors.IOError
	// Corruption means malformed engine data.
	Corruption = errors.Corruption
	// DatabaseLocked means identified directory lock contention.
	DatabaseLocked = errors.DatabaseLocked
	// DatabaseClosed means a closed or uninitialized resource was used.
	DatabaseClosed = errors.DatabaseClosed
	// Unsupported means an unknown setting or unavailable capability.
	Unsupported = errors.Unsupported
	// InternalError means an unexpected native failure.
	InternalError = errors.InternalError
)

// Error preserves a native Code and the original engine Message.
type Error = errors.Error

// Compression controls newly written blocks.
type Compression = options.Compression

const (
	// NoCompression writes uncompressed blocks.
	NoCompression = options.NoCompression
	// Snappy uses the bundled Snappy codec (the default).
	Snappy = options.Snappy
)

// OpenOptions holds opening settings. Zero numeric values mean engine defaults,
// except CacheSize whose non-nil pointer explicitly selects a capacity (even 0).
// Compression nil selects Snappy. Engine bounds can clamp positive tuning values.
type OpenOptions = options.OpenOptions

// ReadOptions controls checksum verification and cache filling.
type ReadOptions = options.ReadOptions

// WriteOptions controls durability.
type WriteOptions = options.WriteOptions

// Database owns a directory handle. Copies share ownership; Close affects all.
// The zero value is closed. Methods serialize operations against all cleanup.
type Database = database.Database

// Batch is a reusable database-independent ordered write sequence. Copies share
// ownership; the zero value is closed. Changes are pending until Database.Write.
type Batch = database.Batch

// Snapshot is an immutable same-instance native read view retaining the engine.
// Copies share ownership; the zero value is closed.
type Snapshot = database.Snapshot

// Cursor is an owned bidirectional streaming iterator. Copies share position and
// ownership; the zero value is closed. Check Status after scanning.
type Cursor = database.Cursor

// Open opens a UTF-8 path. Nil options use defaults with creation disabled.
func Open(path string, opts *OpenOptions) (*Database, error) { return database.Open(path, opts) }

// Create opens a new database, rejecting existing directories without changing opts.
func Create(path string, opts *OpenOptions) (*Database, error) { return database.Create(path, opts) }

// NewBatch allocates an empty native batch.
func NewBatch() (*Batch, error) { return database.NewBatch() }
