const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const {Database} = require('../src/index.cjs');

test('create requires a new database and preserves options and stored data', async () => {
    const root = await fs.mkdtemp(path.join(process.env.ML_TEST_ROOT || os.tmpdir(), 'multileveldb-create-'));
    const name = path.join(root, 'новая');
    const opts = Object.freeze({createIfMissing: false, errorIfExists: false, compression: 'none', cacheSize: 0});
    let db;
    try {
        db = await Database.create(name, opts);
        const cursor = await db.iterator();
        try {
            await cursor.seekToFirst();
            assert.equal(await cursor.isValid(), false);
        } finally {
            await cursor.close();
        }
        await db.put(Buffer.from([0, 255]), Buffer.from([128, 0]));
        await assert.rejects(Database.create(name), {code: 'DATABASE_LOCKED'});
        await db.close();
        await assert.rejects(Database.create(name, opts), {code: 'INVALID_ARGUMENT'});
        db = await Database.open(name, opts);
        assert.deepEqual(await db.get(Buffer.from([0, 255])), Buffer.from([128, 0]));
        await db.close();
        await assert.rejects(Database.open(path.join(root, 'absent'), opts), {code: 'INVALID_ARGUMENT'});
        const esm = await import('../src/index.mjs');
        db = await esm.Database.create(path.join(root, 'defaults'));
        await db.close();
        await assert.rejects(Database.create(''), {code: 'INVALID_ARGUMENT'});
        await assert.rejects(Database.create('\ud800'), TypeError);
        await assert.rejects(Database.create(name, {createIfMissing: 0}), TypeError);
        await assert.rejects(Database.create(name, null), TypeError);
        await assert.rejects(Database.create(path.join(root, 'missing', 'db')),
            error => error.code === 'IO_ERROR' || error.code === 'NOT_FOUND');
    } finally {
        if (db) await db.close();
        await fs.rm(root, {recursive: true});
    }
});
test('binary data, snapshots, queues, close and reopen', async () => {
    const root = await fs.mkdtemp(path.join(process.env.ML_TEST_ROOT || os.tmpdir(), 'multileveldb-node-данные-'));
    const name = path.join(root, 'db');
    const db = await Database.open(name, {
        createIfMissing: true, compression: 'none',
        errorIfExists: false, paranoidChecks: true, writeBufferSize: 65536,
        maxOpenFiles: 40, blockSize: 4096, blockRestartInterval: 16, cacheSize: 1048576
    });
    const key = Buffer.from([0, 255]), value = Buffer.from([0, 128, 255]);
    await db.put(key, value, {sync: true});
    assert.deepEqual(await db.get(key, {verifyChecksums: true, fillCache: false}), value);
    await assert.rejects(Database.open(name), {code: 'DATABASE_LOCKED'});
    const snapshot = await db.createSnapshot();
    const batch = await db.createBatch();
    await batch.put(key, new Uint8Array(0));
    await batch.put(Buffer.from('z'), value);
    await batch.delete(Buffer.from('z'));
    await db.write(batch);
    await batch.clear();
    await batch.close();
    await batch.close();
    assert.deepEqual(await db.get(key), Buffer.alloc(0));
    assert.deepEqual(await db.get(key, {}, snapshot), value);
    const cursor = await db.iterator({}, snapshot);
    await snapshot.close();
    await cursor.seek(key);
    assert.equal(await cursor.isValid(), true);
    assert.deepEqual(await cursor.value(), value);
    await cursor.seekToFirst();
    await cursor.seekToLast();
    await cursor.previous();
    assert.equal(await cursor.isValid(), false);
    await cursor.seekToFirst();
    assert.deepEqual(await cursor.key(), key);
    await cursor.next();
    assert.equal(await cursor.isValid(), false);
    await cursor.status();
    await assert.rejects(cursor.next(), {code: 'NOT_FOUND'});
    await cursor.close();
    await db.delete(key);
    assert.equal(await db.get(key), null);
    const large = Buffer.alloc(4 * 1024 * 1024, 255);
    await db.put(key, large);
    assert.deepEqual(await db.get(key), large);
    const pending = Array.from({length: 30}, (_, n) => db.put(Buffer.from([n]), Buffer.from([n])));
    const close = db.close();
    await Promise.all(pending);
    await close;
    await db.close();
    await assert.rejects(db.get(key), {code: 'DATABASE_CLOSED'});
    assert.throws(() => db.get('text'), TypeError);
    assert.throws(() => db.put(key, value, {sync: 1}), TypeError);
    const reopened = await Database.open(name);
    assert.deepEqual(await reopened.get(key), large);
    await reopened.close();
    const esm = await import('../src/index.mjs');
    assert.equal(esm.Database, Database);
    await fs.rm(root, {recursive: true});
});
