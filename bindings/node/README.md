# multileveldb for Node.js

Read and write LevelDB from Node.js 22 or later. The package supports CommonJS,
ES modules and TypeScript.

## Setup

A prepared package includes `multileveldb.node` and its companion native library
under `prebuilds/<platform>-<arch>`. A source checkout looks for the addon in
`build/bindings/node`. Set `MULTILEVELDB_NODE` to the absolute path to the
addon if you built it elsewhere.

For a MinGW build on Windows, keep the compiler runtime DLLs with the addon
and `multileveldb.dll`.

## Read and write bytes

Save this example as `example.mjs` and run `node example.mjs`. If you
installed `@spechurkin/multileveldb` from GitHub Packages, use that name in
the import.

```javascript
import { Database } from 'multileveldb';

const db = await Database.open('data', { createIfMissing: true });
try {
  const key = Buffer.from([0, 255]);
  await db.put(key, Buffer.from([0, 128]));
  console.log(await db.get(key)); // <Buffer 00 80>
} finally {
  await db.close();
}
```

Pass a `Buffer` or `Uint8Array` for keys and values. Reads resolve to a
`Buffer`. A missing key resolves to `null`, while an empty value resolves to
a zero-length `Buffer`. In CommonJS, import with
`const { Database } = require('multileveldb')`.

Await `close()` on databases, cursors, snapshots and batches. Native errors
carry a stable string `code`.

## Require a new database

Use `await Database.create('new-data', options)` to open a newly created
database. Create the parent directories first. The binding copies optional
settings and sets `createIfMissing` and `errorIfExists` on that copy. If the
database exists, the promise rejects with `INVALID_ARGUMENT`, or with
`DATABASE_LOCKED` if lock acquisition fails first. This method needs native
ABI 1.1 or later.
