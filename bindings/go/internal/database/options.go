package database

import (
	settings "github.com/spechurkin/leveldb-lib/bindings/go/internal/options"
)

/*
#include <multileveldb.h>
*/
import "C"

type setting struct {
	field uint32
	value uint64
}

func boolean(value bool) uint64 {
	if value {
		return 1
	}
	return 0
}

func openSettings(o *settings.OpenOptions) []setting {
	if o == nil {
		return nil
	}
	fields := []setting{{1, boolean(o.CreateIfMissing)}, {2, boolean(o.ErrorIfExists)}, {3, boolean(o.ParanoidChecks)}}
	for i, value := range []uint64{o.WriteBufferSize, o.MaxOpenFiles, o.BlockSize, o.BlockRestartInterval} {
		if value != 0 {
			fields = append(fields, setting{uint32(i + 4), value})
		}
	}
	if o.Compression != nil {
		fields = append(fields, setting{8, uint64(*o.Compression)})
	}
	if o.CacheSize != nil {
		fields = append(fields, setting{9, *o.CacheSize})
	}
	return fields
}

func readSettings(o *settings.ReadOptions) []setting {
	if o == nil {
		return nil
	}
	fields := []setting{{10, boolean(o.VerifyChecksums)}}
	if o.FillCache != nil {
		fields = append(fields, setting{11, boolean(*o.FillCache)})
	}
	return fields
}

func writeSettings(o *settings.WriteOptions) []setting {
	if o == nil {
		return nil
	}
	return []setting{{12, boolean(o.Sync)}}
}

func options(kind uint32, fields []setting) (*C.ml_options, error) {
	if fields == nil {
		return nil, nil
	}
	var pointer *C.ml_options
	var native *C.ml_error
	status := C.ml_options_create(C.uint32_t(kind), &pointer, &native)
	if err := checked(status, native); err != nil {
		return nil, err
	}
	for _, field := range fields {
		native = nil
		status = C.ml_options_set(pointer, C.uint32_t(field.field), C.uint64_t(field.value), &native)
		if err := checked(status, native); err != nil {
			C.ml_options_destroy(pointer)
			return nil, err
		}
	}
	return pointer, nil
}
