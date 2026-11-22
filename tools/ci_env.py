"""Expose a single-configuration native build to the adapter test processes."""
import os
from pathlib import Path

root = Path(__file__).resolve().parents[1]
native = root / "build/native"
suffix = "*multileveldb.dll" if os.name == "nt" else "libmultileveldb.dylib" if os.sys.platform == "darwin" else "libmultileveldb.so"
library = next(native.glob(suffix)).resolve()
values = {
    "ML_NATIVE_DIR": str(native),
    "MULTILEVELDB_LIBRARY": str(library),
    "ML_TEST_ROOT": str(root / "build"),
    "LD_LIBRARY_PATH": str(native),
    "DYLD_LIBRARY_PATH": str(native),
}
with open(os.environ["GITHUB_ENV"], "a", encoding="utf-8") as output:
    for key, value in values.items():
        print(f"{key}={value}", file=output)
with open(os.environ["GITHUB_PATH"], "a", encoding="utf-8") as output:
    print(native, file=output)
