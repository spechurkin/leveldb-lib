"""Verify OS locking and delayed engine release across separate processes."""
import ctypes as c
import os
import subprocess
import sys
import tempfile
from pathlib import Path

library_path = os.environ["MULTILEVELDB_LIBRARY"]
dll_dir = None
if os.name == "nt" and Path(library_path).is_file():
    dll_dir = os.add_dll_directory(str(Path(library_path).resolve().parent))
lib = c.CDLL(library_path)
P = c.c_void_p
PP = c.POINTER(P)
lib.ml_db_open.argtypes = [c.c_char_p, c.c_size_t, P, PP, PP]
lib.ml_options_create.argtypes = [c.c_uint32, PP, PP]
lib.ml_options_set.argtypes = [P, c.c_uint32, c.c_uint64, PP]
lib.ml_options_destroy.argtypes = [P]
lib.ml_db_destroy.argtypes = [P]
lib.ml_db_close.argtypes = [P, PP]
lib.ml_iterator_create.argtypes = [P, P, P, PP, PP]
lib.ml_iterator_destroy.argtypes = [P]
lib.ml_error_free.argtypes = [P]


def open_db(path, options=None):
    db, error = P(), P()
    encoded = str(path).encode("utf-8")
    status = lib.ml_db_open(encoded, len(encoded), options, c.byref(db), c.byref(error))
    lib.ml_error_free(error)
    return status, db


if len(sys.argv) > 1:
    status, db = open_db(sys.argv[1])
    lib.ml_db_destroy(db)
    sys.exit(status)

with tempfile.TemporaryDirectory(dir=os.environ["ML_TEST_ROOT"], prefix="process-lock-") as root:
    path = Path(root) / "данные-😀"
    options, error = P(), P()
    assert lib.ml_options_create(0, c.byref(options), c.byref(error)) == 0
    assert lib.ml_options_set(options, 1, 1, c.byref(error)) == 0
    status, db = open_db(path, options)
    assert status == 0
    lib.ml_options_destroy(options)
    iterator = P()
    assert lib.ml_iterator_create(db, None, None, c.byref(iterator), c.byref(error)) == 0


    def child():
        return subprocess.run([sys.executable, __file__, str(path)], check=False).returncode


    assert child() == 5
    assert lib.ml_db_close(db, c.byref(error)) == 0
    lib.ml_db_destroy(db)
    assert child() == 5, "Iterator must retain the engine and its directory lock"
    lib.ml_iterator_destroy(iterator)
    assert child() == 0
print("Cross-process locking and delayed release: passed")
