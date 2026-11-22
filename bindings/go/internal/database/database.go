package database

import (
	failures "github.com/spechurkin/leveldb-lib/bindings/go/internal/errors"
	settings "github.com/spechurkin/leveldb-lib/bindings/go/internal/options"
)

/*
#include <multileveldb.h>
*/
import "C"

import (
	"runtime"
	"unicode/utf8"
	"unsafe"
)

// Database owns a directory handle. Copies share ownership; Close affects all.
// The zero value is closed. Methods serialize operations against all cleanup.
type Database struct{ r *resource }

// Open opens a UTF-8 path. Nil options use defaults with creation disabled.
func Open(path string, opts *settings.OpenOptions) (*Database, error) {
	return open(path, opts, false)
}

func open(path string, opts *settings.OpenOptions, create bool) (*Database, error) {
	gate.Lock()
	defer gate.Unlock()
	if err := abi(); err != nil {
		return nil, err
	}
	if !utf8.ValidString(path) {
		return nil, &failures.Error{Code: failures.InvalidArgument, Message: "path must be UTF-8"}
	}
	o, err := options(0, openSettings(opts))
	if err != nil {
		return nil, err
	}
	defer C.ml_options_destroy(o)
	var pointer *C.ml_db
	var native *C.ml_error
	bytes := []byte(path)
	var status C.ml_status
	if create {
		status = C.ml_db_create(data(bytes), C.size_t(len(bytes)), o, &pointer, &native)
	} else {
		status = C.ml_db_open(data(bytes), C.size_t(len(bytes)), o, &pointer, &native)
	}
	if err := checked(status, native); err != nil {
		return nil, err
	}
	return &Database{own(unsafe.Pointer(pointer), func(p unsafe.Pointer) { C.ml_db_destroy((*C.ml_db)(p)) })}, nil
}

// Create creates a new database, returning an error if it already exists.
func Create(path string, opts *settings.OpenOptions) (*Database, error) {
	return open(path, opts, true)
}

// Close releases the wrapper once; existing snapshots/cursors retain the engine.
func (d *Database) Close() error { return d.r.close() }

// Get returns independent bytes, found and error. Empty existing values return
// a non-nil empty slice and found=true; missing keys return nil, false, nil.
func (d *Database) Get(key []byte, opts *settings.ReadOptions, snapshot *Snapshot) ([]byte, bool, error) {
	gate.Lock()
	defer gate.Unlock()
	h, err := d.r.handle()
	if err != nil {
		return nil, false, err
	}
	s, err := snapshotHandle(snapshot)
	if err != nil {
		return nil, false, err
	}
	o, err := options(1, readSettings(opts))
	if err != nil {
		return nil, false, err
	}
	defer C.ml_options_destroy(o)
	var pointer *C.uint8_t
	var length C.size_t
	var native *C.ml_error
	status := C.ml_db_get((*C.ml_db)(h), data(key), C.size_t(len(key)), o, s, &pointer, &length, &native)
	runtime.KeepAlive(d)
	runtime.KeepAlive(snapshot)
	if status == 1 {
		C.ml_error_free(native)
		return nil, false, nil
	}
	if err := checked(status, native); err != nil {
		return nil, false, err
	}
	value, err := copyBuffer(pointer, length)
	return value, err == nil, err
}

// Put inserts/replaces exact bytes. Nil slices are valid empty keys/values.
func (d *Database) Put(key, value []byte, opts *settings.WriteOptions) error {
	gate.Lock()
	defer gate.Unlock()
	h, err := d.r.handle()
	if err != nil {
		return err
	}
	o, err := options(2, writeSettings(opts))
	if err != nil {
		return err
	}
	defer C.ml_options_destroy(o)
	var native *C.ml_error
	status := C.ml_db_put((*C.ml_db)(h), data(key), C.size_t(len(key)), data(value), C.size_t(len(value)), o, &native)
	runtime.KeepAlive(d)
	return checked(status, native)
}

// Delete removes a binary key; absent keys also succeed.
func (d *Database) Delete(key []byte, opts *settings.WriteOptions) error {
	gate.Lock()
	defer gate.Unlock()
	h, err := d.r.handle()
	if err != nil {
		return err
	}
	o, err := options(2, writeSettings(opts))
	if err != nil {
		return err
	}
	defer C.ml_options_destroy(o)
	var native *C.ml_error
	status := C.ml_db_delete((*C.ml_db)(h), data(key), C.size_t(len(key)), o, &native)
	runtime.KeepAlive(d)
	return checked(status, native)
}

// Write atomically applies a live batch in insertion order without clearing it.
func (d *Database) Write(batch *Batch, opts *settings.WriteOptions) error {
	gate.Lock()
	defer gate.Unlock()
	h, err := d.r.handle()
	if err != nil {
		return err
	}
	if batch == nil {
		return &failures.Error{Code: failures.InvalidArgument, Message: "batch is nil"}
	}
	b, err := batch.r.handle()
	if err != nil {
		return err
	}
	o, err := options(2, writeSettings(opts))
	if err != nil {
		return err
	}
	defer C.ml_options_destroy(o)
	var native *C.ml_error
	status := C.ml_batch_write((*C.ml_db)(h), (*C.ml_batch)(b), o, &native)
	runtime.KeepAlive(d)
	runtime.KeepAlive(batch)
	return checked(status, native)
}
