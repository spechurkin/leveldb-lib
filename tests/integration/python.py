"""Read the shared fixture after Node's update and leave a Python-owned record."""
import os
from multileveldb import Database
from pathlib import Path

with Database.open(os.environ["ML_INTEROP_PATH"]) as db:
    for line in Path(os.environ["ML_FIXTURE"]).read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        key, value = (b"" if field == "-" else bytes.fromhex(field) for field in line.split("\t"))
        if db.get(key) != value + b"\0\xfe":
            raise AssertionError("Python fixture mismatch")
    db.put(b"python\0\xff", b"\0\x80\xff")
