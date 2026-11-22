# Contributing to MultiLevelDB

Read the [Code of Conduct](CODE_OF_CONDUCT.md) before taking part in project discussions or submitting changes.

## Report a bug or propose a change

Search the [issue tracker](https://github.com/spechurkin/leveldb-lib/issues) before opening an issue. For a bug, include the MultiLevelDB version or commit, your operating system and architecture, the binding and runtime version if applicable, and steps to reproduce the problem. Show what you expected and what happened. If the bug involves an existing database, say which application or binding created it and include a small script or sample with synthetic data if possible.

Open an issue before implementing a substantial feature or changing a public API, the C ABI, or database compatibility. Describe the use case and what the change would mean for existing callers and databases.

## Build and test

See the [build guide](docs/BUILDING.md) for toolchains, platform setup, and test commands for each language binding. From the repository root, build and test the native library with:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run the tests for any binding you change. For a bug fix, add a focused regression test when feasible. For changes to the C interface, consult the [ABI guide](docs/ABI.md). If a change affects how different bindings read or write the same database, use the [cross-language tests](docs/BUILDING.md#cross-language-tests).

## Open a pull request

Keep the pull request focused on one change. Link the relevant issue and explain what changed, why, and how you tested it. Call out changes to public APIs or compatibility, and update the relevant documentation when behavior changes. For performance changes, include benchmark results and describe the test environment.

This guide draws on the [Google LevelDB contribution guide](https://github.com/google/leveldb/blob/main/CONTRIBUTING.md) and the [RocksDB contribution guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Contribution-Guide).
