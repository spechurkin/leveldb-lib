"""Verify that all language package manifests declare the same SemVer release version."""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import tomllib
from pathlib import Path

SEMVER_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$")


def _regex_extract(path: Path, pattern: str) -> str:
    match = re.search(pattern, path.read_text(encoding="utf-8"), flags=re.MULTILINE)
    if not match:
        raise ValueError(f"Could not locate version in {path}")
    return match.group(1)


def collect_versions(root: Path) -> dict[str, str]:
    """Return a mapping of manifest path (relative to root) to declared version."""
    node_pkg = json.loads((root / "bindings/node/package.json").read_text(encoding="utf-8"))
    python_pkg = tomllib.loads((root / "bindings/python/pyproject.toml").read_text(encoding="utf-8"))
    rust_pkg = tomllib.loads((root / "bindings/rust/Cargo.toml").read_text(encoding="utf-8"))
    return {
        "CMakeLists.txt": _regex_extract(
            root / "CMakeLists.txt",
            r"^\s*project\(\s*multileveldb\s+VERSION\s+([^\s)]+)",
        ),
        "native/Doxyfile": _regex_extract(
            root / "native/Doxyfile",
            r"^\s*PROJECT_NUMBER\s*=\s*([^\s#]+)",
        ),
        "bindings/rust/Cargo.toml": rust_pkg["package"]["version"],
        "bindings/jvm/build.gradle.kts": _regex_extract(
            root / "bindings/jvm/build.gradle.kts",
            r'^\s*version\s*=\s*"([^"]+)"',
        ),
        "bindings/node/package.json": node_pkg["version"],
        "bindings/dotnet/MultiLevelDB.csproj": _regex_extract(
            root / "bindings/dotnet/MultiLevelDB.csproj",
            r"<Version>([^<]+)</Version>",
        ),
        "bindings/python/pyproject.toml": python_pkg["project"]["version"],
        "bindings/ruby/multileveldb.gemspec": _regex_extract(
            root / "bindings/ruby/multileveldb.gemspec",
            r"""spec\.version\s*=\s*['"]([^'"]+)['"]""",
        ),
        "bindings/perl/lib/MultiLevelDB.pm": _regex_extract(
            root / "bindings/perl/lib/MultiLevelDB.pm",
            r"""our\s+\$VERSION\s*=\s*['"]([^'"]+)['"]\s*;""",
        ),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", help="Git tag name (with optional leading 'v') to match.")
    parser.add_argument("--expected", help="Explicit expected SemVer version to match.")
    args = parser.parse_args(argv)

    expected = args.expected
    if args.tag:
        tag_version = args.tag[1:] if args.tag.startswith("v") else args.tag
        if expected and expected != tag_version:
            print(f"ERROR: --expected ({expected}) differs from --tag ({tag_version})", file=sys.stderr)
            return 1
        expected = tag_version

    if expected is not None and not SEMVER_RE.match(expected):
        print(f"ERROR: Expected version '{expected}' is not valid SemVer", file=sys.stderr)
        return 1

    root = Path(__file__).resolve().parents[1]
    versions = collect_versions(root)
    distinct = sorted(set(versions.values()))
    target = expected or distinct[0]

    mismatches = [
        (rel, ver) for rel, ver in versions.items() if ver != target or not SEMVER_RE.match(ver)
    ]
    if mismatches or len(distinct) != 1:
        print(f"ERROR: Version mismatch (expected {target}):", file=sys.stderr)
        for rel, ver in versions.items():
            marker = "OK  " if ver == target and SEMVER_RE.match(ver) else "FAIL"
            print(f"  [{marker}] {rel}: {ver}", file=sys.stderr)
        return 1

    print(f"version={target}")
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a", encoding="utf-8") as fh:
            print(f"version={target}", file=fh)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
