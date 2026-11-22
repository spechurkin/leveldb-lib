'use strict';
const native = require('../internal/native.cjs');
const {Resource, binary, schedule, token} = require('../internal/resource.cjs');
const {options, openFields, readFields, writeFields} = require('../internal/options.cjs');
const {Batch} = require('./batch.cjs');
const {Snapshot} = require('./snapshot.cjs');
const {Cursor} = require('./cursor.cjs');

/**
 * Binary-safe Google LevelDB handle. Blocking operations use Node-API workers;
 * overlapping resource calls are queued. Byte inputs are copied immediately.
 * @memberof module:multileveldb
 * @inner
 * @augments module:multileveldb~Resource
 */
class Database extends Resource {
    /**
     * Create a new LevelDB and return its open handle. Parents must exist.
     * @param {string} databasePath Nonempty well-formed Unicode path without NUL; relative to cwd.
     * @param {OpenOptions} [opts] Copied storage settings; createIfMissing and errorIfExists are forced true.
     * @returns {Promise<Database>} Owned handle. Rejects DbError with INVALID_ARGUMENT for an
     * existing database; lock checks may reject DATABASE_LOCKED first. Other failures match open.
     * @throws {TypeError} for invalid path or option argument shapes.
     */
    static async create(databasePath, opts) {
        if (typeof databasePath !== 'string' || databasePath !== databasePath.toWellFormed()) throw new TypeError('Path must be well-formed Unicode');
        const h = await native.invoke('create', null, null, Buffer.from(databasePath, 'utf8'), null, options(opts, openFields));
        return new Database(h, token);
    }

    /**
     * Open a standard LevelDB directory; absent options select engine defaults.
     * @param {string} databasePath Nonempty well-formed Unicode path without NUL; relative to cwd.
     * @param {OpenOptions} [opts] Opening settings; numeric values must be nonnegative safe integers.
     * @returns {Promise<Database>} Owned handle. Rejects TypeError for JS argument shapes, or DbError
     * for invalid/missing/locked paths, existence policy, I/O, corruption or allocation.
     * Parents must exist; creation is disabled by default.
     */
    static async open(databasePath, opts) {
        if (typeof databasePath !== 'string' || databasePath !== databasePath.toWellFormed()) throw new TypeError('Path must be well-formed Unicode');
        const h = await native.invoke('open', null, null, Buffer.from(databasePath, 'utf8'), null, options(opts, openFields));
        return new Database(h, token);
    }

    /**
     * Read exactly one binary key.
     * @param {Uint8Array} key Buffer/Uint8Array, possibly empty; copied before queuing.
     * @param {ReadOptions} [opts] Read settings; undefined selects defaults.
     * @param {Snapshot} [snapshot] Read view from this exact database instance, or undefined for current data.
     * @returns {Promise<(Buffer|null)>} Owned Buffer, including length zero, or null if absent. Rejects
     * DbError for closed resources, foreign snapshots, engine lengths, I/O or corruption.
     * @throws {TypeError} for invalid key, options, or snapshot argument shape.
     */
    get(key, opts, snapshot) {
        if (snapshot && !(snapshot instanceof Snapshot)) throw new TypeError('Expected Snapshot');
        return schedule([this, snapshot], 'get', binary(key), null, options(opts, readFields));
    }

    /**
     * Insert/replace a value; empty keys and values are valid.
     * @param {Uint8Array} key Binary key copied at invocation.
     * @param {Uint8Array} value Binary value copied at invocation.
     * @param {WriteOptions} [opts] Durability settings; undefined means nonsynchronous defaults.
     * @returns {Promise<void>} Completion; rejects DbError for closed resources, length overflow or write failures.
     * @throws {TypeError} for nonbyte arguments or malformed options.
     */
    put(key, value, opts) {
        return schedule([this], 'put', binary(key), binary(value), options(opts, writeFields));
    }

    /**
     * Delete a binary key; an already absent key also succeeds.
     * @param {Uint8Array} key Binary key copied before queuing.
     * @param {WriteOptions} [opts] Write durability, or undefined for defaults.
     * @returns {Promise<void>} Completion; rejects DbError for closed resources, invalid length or write failure.
     * @throws {TypeError} for invalid key/options shapes.
     */
    delete(key, opts) {
        return schedule([this], 'delete', binary(key), null, options(opts, writeFields));
    }

    /**
     * Allocate an empty, independently owned batch while this database is open.
     * @returns {Promise<Batch>} Batch not tied to this DB; rejects DbError if closed or allocation fails.
     */
    async createBatch() {
        return new Batch(await schedule([this], 'batch'), token);
    }

    /**
     * Atomically apply operations in insertion order. The batch is not consumed or
     * cleared and may be reused with another DB. This adds no read transaction.
     * @param {Batch} batch Live reusable batch; its queue participates in write ordering.
     * @param {WriteOptions} [opts] Write durability, or undefined for defaults.
     * @returns {Promise<void>} Completion; rejects DbError for closed participants or engine write failure.
     * @throws {TypeError} for invalid batch/options shapes.
     */
    write(batch, opts) {
        if (!(batch instanceof Batch)) throw new TypeError('Expected Batch');
        return schedule([this, batch], 'write', null, null, options(opts, writeFields));
    }

    /**
     * Capture an immutable engine read view; it may retain older table versions.
     * @returns {Promise<Snapshot>} Owned snapshot for this DB; rejects DbError if closed or allocation fails.
     */
    async createSnapshot() {
        return new Snapshot(await schedule([this], 'snapshot'), token);
    }

    /**
     * Create an initially unpositioned streaming cursor; seek before accessing data.
     * @param {ReadOptions} [opts] Read settings, or undefined for defaults; copied at invocation.
     * @param {Snapshot} [snapshot] Same-instance snapshot, or undefined for a current-view cursor.
     * @returns {Promise<Cursor>} Owned cursor retaining engine/snapshot after their wrappers close.
     * Rejects TypeError for argument shapes or DbError for foreign/closed resources
     * and engine/native creation failures.
     */
    async iterator(opts, snapshot) {
        if (snapshot && !(snapshot instanceof Snapshot)) throw new TypeError('Expected Snapshot');
        return new Cursor(await schedule([this, snapshot], 'iterator', null, null, options(opts, readFields)), token);
    }
}

module.exports = {Database};
