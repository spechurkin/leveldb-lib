"""Stage platform binaries and notices for npm pack; does not publish anything."""
import os
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get("ML_BUILD_DIR", root / "build"))
platform, arch = subprocess.check_output(["node", "-p", "process.platform+' '+process.arch"], text=True).split()
destination = root / f"bindings/node/prebuilds/{platform}-{arch}"
if destination.exists():
    shutil.rmtree(destination)
destination.mkdir(parents=True, exist_ok=True)
addon = next((build / "bindings/node").rglob("multileveldb.node"))
shutil.copy2(addon, destination / addon.name)
native = build / "native"
for file in native.rglob("*"):
    if not file.is_file():
        continue
    if platform == "win32":
        matched = file.name.endswith(".dll")
    elif platform == "darwin":
        matched = file.name.endswith(".dylib")
    else:
        matched = file.name == "libmultileveldb.so" or file.name.startswith("libmultileveldb.so.")
    if matched:
        shutil.copy2(file, destination / file.name)
shutil.copy2(root / "LICENSE", root / "bindings/node/LICENSE")
for source, name in [(build / "_deps/leveldb-src/LICENSE", "LICENSE.leveldb"),
                     (build / "_deps/snappy-src/COPYING", "LICENSE.snappy")]:
    shutil.copy2(source, destination / name)
print(destination)
