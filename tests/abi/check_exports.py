"""Check the frozen ABI-1 symbol list through the platform's actual loader."""
import ctypes
import os
from pathlib import Path

library_path = os.environ["MULTILEVELDB_LIBRARY"]
dll_dir = None
if os.name == "nt" and Path(library_path).is_file():
    dll_dir = os.add_dll_directory(str(Path(library_path).resolve().parent))
library = ctypes.CDLL(library_path)
baseline = Path(__file__).with_name("v1-symbols.txt")
for symbol in baseline.read_text().splitlines():
    getattr(library, symbol)
library.ml_abi_version.restype = ctypes.c_uint32
assert library.ml_abi_version() >> 16 == 1
assert library.ml_abi_version() >= 0x00010001
getattr(library, "ml_db_create")
print("ABI 1.0 symbols and ABI 1.1 creation export: passed")
