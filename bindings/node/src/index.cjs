/**
 * Binary-safe asynchronous LevelDB access. All native resources require awaited
 * close; keys/values are bytes, paths are well-formed Unicode.
 * Loading needs a matching native addon and companion shared library. Module
 * loading/ABI failures are ordinary loader Errors, before any promise exists;
 * the DbError code contract applies to native worker failures after loading.
 * @module multileveldb
 */
'use strict';
/**
 * Opening settings; every numeric value must be a nonnegative safe integer.
 * @typedef {Object} OpenOptions
 * @property {boolean} [createIfMissing=false] Create a missing DB; parents must exist.
 * @property {boolean} [errorIfExists=false] Reject an existing DB; lock checks may fail first.
 * @property {boolean} [paranoidChecks=false] Extra engine opening/recovery checks.
 * @property {number} [writeBufferSize=4194304] Positive byte target; engine may clamp it.
 * @property {number} [maxOpenFiles=1000] Positive C-int file budget; engine may clamp it.
 * @property {number} [blockSize=4096] Positive uncompressed byte target, not a record-size limit.
 * @property {number} [blockRestartInterval=16] Positive C-int keys between restart points.
 * @property {('none'|'snappy')} [compression='snappy'] Compression for newly written blocks.
 * @property {number} [cacheSize] LRU byte capacity; zero gives zero capacity, unset uses engine policy.
 */
/**
 * Per-read settings; snapshots are passed as a separate argument.
 * @typedef {Object} ReadOptions
 * @property {boolean} [verifyChecksums=false] Verify fetched storage-block checksums.
 * @property {boolean} [fillCache=true] Populate the block cache; false still permits cached reads.
 */
/**
 * Per-write durability, copied when an operation is invoked.
 * @typedef {Object} WriteOptions
 * @property {boolean} [sync=false] Request the engine's synchronous storage flush before success.
 */
/**
 * Stable native failure category. Missing gets return null; invalid cursor access
 * uses NOT_FOUND. INVALID_ARGUMENT covers native input/options/snapshot pairing;
 * IO_ERROR preserves unclassified I/O failures; CORRUPTION is engine-detected;
 * DATABASE_LOCKED is identified directory contention; DATABASE_CLOSED is use
 * after resource close; UNSUPPORTED is an unavailable setting/capability;
 * INTERNAL_ERROR is an unexpected native exception or unknown native status.
 * @typedef {('NOT_FOUND'|'INVALID_ARGUMENT'|'IO_ERROR'|'CORRUPTION'|'DATABASE_LOCKED'|'DATABASE_CLOSED'|'UNSUPPORTED'|'INTERNAL_ERROR')} ErrorCode
 */
/**
 * A native worker's standard Error with the original diagnostic message.
 * @typedef {Error} DbError
 * @property {ErrorCode} code Stable category; never classify by parsing message text.
 */
const {Database} = require('./resources/database.cjs');
const {Batch} = require('./resources/batch.cjs');
const {Snapshot} = require('./resources/snapshot.cjs');
const {Cursor} = require('./resources/cursor.cjs');
module.exports = {Database, Batch, Snapshot, Cursor};
