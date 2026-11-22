import ctypes as C
import os
import sys
import threading
from ctypes.util import find_library

from .errors import Error

_lock = threading.RLock()
_library = None
_P, _N, _U, _Q = C.c_void_p, C.c_size_t, C.c_uint32, C.c_uint64


def _lib():
    global _library
    if _library is None:
        filename = "multileveldb.dll" if os.name == "nt" else "libmultileveldb.dylib" if sys.platform == "darwin" else "libmultileveldb.so"
        target = os.environ.get("MULTILEVELDB_LIBRARY") or find_library("multileveldb") or filename
        if os.name == "nt" and os.path.isfile(target) and hasattr(os, "add_dll_directory"):
            with os.add_dll_directory(os.path.dirname(os.path.abspath(target))):
                lib = C.CDLL(target)
        else:
            lib = C.CDLL(target)
        signatures = {
            "abi_version": (_U, []), "error_message": (C.c_char_p, [_P]),
            "error_free": (None, [_P]), "buffer_free": (None, [_P]),
            "options_create": (C.c_int32, [_U, C.POINTER(_P), C.POINTER(_P)]),
            "options_set": (C.c_int32, [_P, _U, _Q, C.POINTER(_P)]),
            "options_destroy": (None, [_P]),
            "db_open": (C.c_int32, [_P, _N, _P, C.POINTER(_P), C.POINTER(_P)]),
            "db_create": (C.c_int32, [_P, _N, _P, C.POINTER(_P), C.POINTER(_P)]),
            "db_destroy": (None, [_P]),
            "db_get": (C.c_int32, [_P, _P, _N, _P, _P, C.POINTER(_P), C.POINTER(_N), C.POINTER(_P)]),
            "db_put": (C.c_int32, [_P, _P, _N, _P, _N, _P, C.POINTER(_P)]),
            "db_delete": (C.c_int32, [_P, _P, _N, _P, C.POINTER(_P)]),
            "iterator_create": (C.c_int32, [_P, _P, _P, C.POINTER(_P), C.POINTER(_P)]),
            "iterator_destroy": (None, [_P]), "iterator_valid": (_U, [_P]),
            "iterator_seek": (C.c_int32, [_P, _P, _N, C.POINTER(_P)]),
            "batch_create": (C.c_int32, [C.POINTER(_P), C.POINTER(_P)]),
            "batch_destroy": (None, [_P]),
            "batch_put": (C.c_int32, [_P, _P, _N, _P, _N, C.POINTER(_P)]),
            "batch_delete": (C.c_int32, [_P, _P, _N, C.POINTER(_P)]),
            "batch_write": (C.c_int32, [_P, _P, _P, C.POINTER(_P)]),
            "snapshot_create": (C.c_int32, [_P, C.POINTER(_P), C.POINTER(_P)]),
            "snapshot_release": (None, [_P]),
        }
        for name in ("seek_first", "seek_last", "next", "previous", "status"):
            signatures["iterator_" + name] = (C.c_int32, [_P, C.POINTER(_P)])
        signatures["batch_clear"] = (C.c_int32, [_P, C.POINTER(_P)])
        for name in ("key", "value"):
            signatures["iterator_" + name] = (C.c_int32, [_P, C.POINTER(_P), C.POINTER(_N), C.POINTER(_P)])
        for name, (result, args) in signatures.items():
            function = getattr(lib, "ml_" + name)
            function.restype, function.argtypes = result, args
        if lib.ml_abi_version() >> 16 != 1:
            raise RuntimeError("Incompatible MultiLevelDB ABI")
        _library = lib
    return _library


def _call(name, *args):
    lib = _lib()
    error = _P()
    status = getattr(lib, "ml_" + name)(*args, C.byref(error))
    try:
        if status:
            message = (lib.ml_error_message(error) or b"").decode("utf-8", "replace")
            raise Error(status, message)
    finally:
        lib.ml_error_free(error)


def _create(name, *args):
    handle = _P()
    _call(name, *args, C.byref(handle))
    return handle


def _bytes(value):
    if not isinstance(value, (bytes, bytearray, memoryview)):
        raise TypeError("Expected bytes, bytearray or memoryview")
    return bytes(value)


def _read(name, *args):
    pointer, length = _P(), _N()
    try:
        _call(name, *args, C.byref(pointer), C.byref(length))
        return C.string_at(pointer, length.value)
    finally:
        _lib().ml_buffer_free(pointer)
