package database

/*
#include <multileveldb.h>
*/
import "C"

import (
	"runtime"
	"unsafe"
)

// Batch is a reusable database-independent ordered write sequence. Copies share
// ownership; the zero value is closed. Changes are pending until Database.Write.
type Batch struct{ r *resource }

// NewBatch allocates an empty native batch.
func NewBatch() (*Batch, error) {
	gate.Lock()
	defer gate.Unlock()
	if err := abi(); err != nil {
		return nil, err
	}
	var pointer *C.ml_batch
	var native *C.ml_error
	status := C.ml_batch_create(&pointer, &native)
	if err := checked(status, native); err != nil {
		return nil, err
	}
	return &Batch{own(unsafe.Pointer(pointer), func(p unsafe.Pointer) { C.ml_batch_destroy((*C.ml_batch)(p)) })}, nil
}

// Close releases a batch once without applying pending operations.
func (b *Batch) Close() error { return b.r.close() }

// Put appends copied binary key/value data.
func (b *Batch) Put(key, value []byte) error {
	gate.Lock()
	defer gate.Unlock()
	h, err := b.r.handle()
	if err != nil {
		return err
	}
	var native *C.ml_error
	status := C.ml_batch_put((*C.ml_batch)(h), data(key), C.size_t(len(key)), data(value), C.size_t(len(value)), &native)
	runtime.KeepAlive(b)
	return checked(status, native)
}

// Delete appends a copied deletion.
func (b *Batch) Delete(key []byte) error {
	gate.Lock()
	defer gate.Unlock()
	h, err := b.r.handle()
	if err != nil {
		return err
	}
	var native *C.ml_error
	status := C.ml_batch_delete((*C.ml_batch)(h), data(key), C.size_t(len(key)), &native)
	runtime.KeepAlive(b)
	return checked(status, native)
}

// Clear discards all queued operations without changing a database.
func (b *Batch) Clear() error {
	gate.Lock()
	defer gate.Unlock()
	h, err := b.r.handle()
	if err != nil {
		return err
	}
	var native *C.ml_error
	status := C.ml_batch_clear((*C.ml_batch)(h), &native)
	runtime.KeepAlive(b)
	return checked(status, native)
}
