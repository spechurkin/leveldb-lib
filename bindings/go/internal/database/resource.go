package database

import (
	failures "github.com/spechurkin/leveldb-lib/bindings/go/internal/errors"
)

import (
	"runtime"
	"sync"
	"unsafe"
)

var gate sync.Mutex

type resource struct {
	pointer unsafe.Pointer
	free    func(unsafe.Pointer)
}

func own(pointer unsafe.Pointer, free func(unsafe.Pointer)) *resource {
	r := &resource{pointer, free}
	runtime.SetFinalizer(r, func(r *resource) { r.close() })
	return r
}

func (r *resource) handle() (unsafe.Pointer, error) {
	if r == nil || r.pointer == nil {
		return nil, &failures.Error{Code: failures.DatabaseClosed, Message: "resource is closed"}
	}
	return r.pointer, nil
}

func (r *resource) close() error {
	gate.Lock()
	defer gate.Unlock()
	if r != nil && r.pointer != nil {
		r.free(r.pointer)
		r.pointer = nil
		runtime.SetFinalizer(r, nil)
	}
	return nil
}
