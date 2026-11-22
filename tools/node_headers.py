"""Fetch Node-API build inputs matching the current Node runtime, with checksums."""
import hashlib
import os
import subprocess
import tarfile
import urllib.request
from pathlib import Path

root = Path(__file__).resolve().parents[1]
version = subprocess.check_output(["node", "-p", "process.versions.node"], text=True).strip()
arch = subprocess.check_output(["node", "-p", "process.arch"], text=True).strip()
base = f"https://nodejs.org/download/release/v{version}/"
destination = root / "build/node-inputs"
destination.mkdir(parents=True, exist_ok=True)
checksums = dict(
    line.split()[::-1] for line in urllib.request.urlopen(base + "SHASUMS256.txt").read().decode().splitlines())


def fetch(name):
    file = destination / Path(name).name
    if not file.exists() or hashlib.sha256(file.read_bytes()).hexdigest() != checksums[name]:
        urllib.request.urlretrieve(base + name, file)
    if hashlib.sha256(file.read_bytes()).hexdigest() != checksums[name]:
        raise RuntimeError(f"Checksum mismatch: {name}")
    return file


archive = fetch(f"node-v{version}-headers.tar.gz")
with tarfile.open(archive) as package:
    package.extractall(destination, filter="data")
include = destination / f"node-v{version}/include/node"
values = {"include": str(include)}
if os.name == "nt":
    values["library"] = str(fetch(f"win-{arch}/node.lib"))
for key, value in values.items():
    print(f"{key}={value}")
if os.environ.get("GITHUB_OUTPUT"):
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        for key, value in values.items():
            print(f"{key}={value}", file=output)
