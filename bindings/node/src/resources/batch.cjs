'use strict';
const {Resource, binary, schedule} = require('../internal/resource.cjs');

/**
 * Reusable database-independent puts/deletions for Database.write. Changes stay
 * pending until write atomically applies them. Close the batch when finished.
 * @memberof module:multileveldb
 * @inner
 * @augments module:multileveldb~Resource
 */
class Batch extends Resource {
    /**
     * Append a copied put without writing to a database.
     * @param {Uint8Array} key Binary key, possibly empty, copied at invocation.
     * @param {Uint8Array} value Binary value, possibly empty, copied at invocation.
     * @returns {Promise<void>} Completion, not the batch; rejects DbError if closed, oversized or allocation fails.
     * @throws {TypeError} for nonbyte arguments.
     */
    put(key, value) {
        return schedule([this], 'batchPut', binary(key), binary(value));
    }

    /**
     * Append a copied deletion. An absent key is harmless when applied.
     * @param {Uint8Array} key Binary key, possibly empty, copied at invocation.
     * @returns {Promise<void>} Completion; rejects DbError if closed, oversized or allocation fails.
     * @throws {TypeError} for a nonbyte argument.
     */
    delete(key) {
        return schedule([this], 'batchDelete', binary(key));
    }

    /** Discard all pending operations, keeping the batch reusable. No DB is modified.
     * @returns {Promise<void>} Completion; rejects DbError after close or on native failure.
     */
    clear() {
        return schedule([this], 'batchClear');
    }
}

module.exports = {Batch};
