package database

/*
#include <multileveldb.h>
*/
import "C"

import (
	"runtime"
	"unsafe"
)

// Snapshot is an immutable same-instance native read view retaining the engine.
// Copies share ownership; the zero value is closed.
type Snapshot struct{ r *resource }

// CreateSnapshot captures the current committed read view.
func (d *Database) CreateSnapshot() (*Snapshot, error) {
	gate.Lock()
	defer gate.Unlock()
	h, err := d.r.handle()
	if err != nil {
		return nil, err
	}
	var pointer *C.ml_snapshot
	var native *C.ml_error
	status := C.ml_snapshot_create((*C.ml_db)(h), &pointer, &native)
	runtime.KeepAlive(d)
	if err := checked(status, native); err != nil {
		return nil, err
	}
	return &Snapshot{own(unsafe.Pointer(pointer), func(p unsafe.Pointer) { C.ml_snapshot_release((*C.ml_snapshot)(p)) })}, nil
}

// Close releases this view once. Existing snapshot cursors retain their view.
func (s *Snapshot) Close() error { return s.r.close() }

func snapshotHandle(s *Snapshot) (*C.ml_snapshot, error) {
	if s == nil {
		return nil, nil
	}
	h, err := s.r.handle()
	return (*C.ml_snapshot)(h), err
}
