package database

import (
	settings "github.com/spechurkin/leveldb-lib/bindings/go/internal/options"
)

/*
#include <multileveldb.h>
*/
import "C"

import (
	"runtime"
	"unsafe"
)

// Cursor is an owned bidirectional streaming iterator. Copies share position and
// ownership; the zero value is closed. Check Status after scanning.
type Cursor struct{ r *resource }

// Iterator creates an unpositioned cursor, optionally using a same-instance view.
func (d *Database) Iterator(opts *settings.ReadOptions, snapshot *Snapshot) (*Cursor, error) {
	gate.Lock()
	defer gate.Unlock()
	h, err := d.r.handle()
	if err != nil {
		return nil, err
	}
	s, err := snapshotHandle(snapshot)
	if err != nil {
		return nil, err
	}
	o, err := options(1, readSettings(opts))
	if err != nil {
		return nil, err
	}
	defer C.ml_options_destroy(o)
	var pointer *C.ml_iterator
	var native *C.ml_error
	status := C.ml_iterator_create((*C.ml_db)(h), o, s, &pointer, &native)
	runtime.KeepAlive(d)
	runtime.KeepAlive(snapshot)
	if err := checked(status, native); err != nil {
		return nil, err
	}
	return &Cursor{own(unsafe.Pointer(pointer), func(p unsafe.Pointer) { C.ml_iterator_destroy((*C.ml_iterator)(p)) })}, nil
}

// Close releases the cursor's engine and snapshot references once.
func (c *Cursor) Close() error { return c.r.close() }

func (c *Cursor) move(op int, key []byte) error {
	gate.Lock()
	defer gate.Unlock()
	h, err := c.r.handle()
	if err != nil {
		return err
	}
	p := (*C.ml_iterator)(h)
	var native *C.ml_error
	var status C.ml_status
	switch op {
	case 0:
		status = C.ml_iterator_seek_first(p, &native)
	case 1:
		status = C.ml_iterator_seek_last(p, &native)
	case 2:
		status = C.ml_iterator_seek(p, data(key), C.size_t(len(key)), &native)
	case 3:
		status = C.ml_iterator_next(p, &native)
	case 4:
		status = C.ml_iterator_previous(p, &native)
	case 5:
		status = C.ml_iterator_status(p, &native)
	}
	runtime.KeepAlive(c)
	return checked(status, native)
}

// SeekToFirst positions at the first record; empty views become invalid.
func (c *Cursor) SeekToFirst() error { return c.move(0, nil) }

// SeekToLast positions at the last record; empty views become invalid.
func (c *Cursor) SeekToLast() error { return c.move(1, nil) }

// Seek positions at the first key >= target; no match becomes invalid.
func (c *Cursor) Seek(key []byte) error { return c.move(2, key) }

// Next advances; passing end succeeds, moving an invalid cursor fails NotFound.
func (c *Cursor) Next() error { return c.move(3, nil) }

// Previous moves backward; an already invalid cursor fails NotFound.
func (c *Cursor) Previous() error { return c.move(4, nil) }

// Status checks recorded engine errors; ordinary exhaustion succeeds.
func (c *Cursor) Status() error { return c.move(5, nil) }

// IsValid checks position without checking engine status. Closed cursors error.
func (c *Cursor) IsValid() (bool, error) {
	gate.Lock()
	defer gate.Unlock()
	h, err := c.r.handle()
	if err != nil {
		return false, err
	}
	valid := C.ml_iterator_valid((*C.ml_iterator)(h)) != 0
	runtime.KeepAlive(c)
	return valid, nil
}

func (c *Cursor) read(key bool) ([]byte, error) {
	gate.Lock()
	defer gate.Unlock()
	h, err := c.r.handle()
	if err != nil {
		return nil, err
	}
	var pointer *C.uint8_t
	var length C.size_t
	var native *C.ml_error
	var status C.ml_status
	if key {
		status = C.ml_iterator_key((*C.ml_iterator)(h), &pointer, &length, &native)
	} else {
		status = C.ml_iterator_value((*C.ml_iterator)(h), &pointer, &length, &native)
	}
	runtime.KeepAlive(c)
	if err := checked(status, native); err != nil {
		return nil, err
	}
	return copyBuffer(pointer, length)
}

// Key returns a copied binary key; invalid position errors NotFound.
func (c *Cursor) Key() ([]byte, error) { return c.read(true) }

// Value returns a copied binary value, including an empty non-nil slice.
func (c *Cursor) Value() ([]byte, error) { return c.read(false) }
