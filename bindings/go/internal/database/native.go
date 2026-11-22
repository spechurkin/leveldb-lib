package database

import (
	failures "github.com/spechurkin/leveldb-lib/bindings/go/internal/errors"
)

/*
#cgo LDFLAGS: -lmultileveldb
#include <multileveldb.h>
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func checked(status C.ml_status, native *C.ml_error) error {
	defer C.ml_error_free(native)
	if status == 0 {
		return nil
	}
	return &failures.Error{Code: failures.Code(status), Message: C.GoString(C.ml_error_message(native))}
}

func abi() error {
	if C.ml_abi_version()>>16 != 1 {
		return &failures.Error{Code: failures.Unsupported, Message: "incompatible MultiLevelDB ABI"}
	}
	return nil
}

func data(value []byte) *C.uint8_t {
	if len(value) == 0 {
		return nil
	}
	return (*C.uint8_t)(unsafe.Pointer(&value[0]))
}

func copyBuffer(pointer *C.uint8_t, length C.size_t) ([]byte, error) {
	defer C.ml_buffer_free(pointer)
	if uint64(length) > uint64(^uint(0)>>1) {
		return nil, fmt.Errorf("native buffer exceeds Go slice capacity")
	}
	return append([]byte{}, unsafe.Slice((*byte)(unsafe.Pointer(pointer)), int(length))...), nil
}
