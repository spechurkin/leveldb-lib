package options

// Compression controls newly written blocks.
type Compression uint64

const (
	// NoCompression writes uncompressed blocks.
	NoCompression Compression = 0
	// Snappy uses the bundled Snappy codec (the default).
	Snappy Compression = 1
)

// OpenOptions holds opening settings. Zero numeric values mean engine defaults,
// except CacheSize whose non-nil pointer explicitly selects a capacity (even 0).
// Compression nil selects Snappy. Engine bounds can clamp positive tuning values.
type OpenOptions struct {
	CreateIfMissing      bool   // Default false; parent directories must exist.
	ErrorIfExists        bool   // Default false.
	ParanoidChecks       bool   // Default false.
	WriteBufferSize      uint64 // Default 4 MiB.
	MaxOpenFiles         uint64 // Default 1000; must fit C int.
	BlockSize            uint64 // Default 4096 uncompressed bytes.
	BlockRestartInterval uint64 // Default 16; must fit C int.
	Compression          *Compression
	CacheSize            *uint64
}

// ReadOptions controls checksum verification and cache filling.
type ReadOptions struct {
	VerifyChecksums bool  // Default false.
	FillCache       *bool // Nil means true; false still permits cached reads.
}

// WriteOptions controls durability.
type WriteOptions struct {
	Sync bool // Default false; true requests synchronous engine flush.
}
