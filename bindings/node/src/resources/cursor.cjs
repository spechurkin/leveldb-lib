'use strict';
const {Resource, binary, schedule} = require('../internal/resource.cjs');

/**
 * Streaming bidirectional cursor, initially unpositioned. Seek before record
 * access, check status at scan end, and await close to release native references.
 * @memberof module:multileveldb
 * @inner
 * @augments module:multileveldb~Resource
 */
class Cursor extends Resource {
    /** Seek first; an empty view succeeds and is invalid. Rejects closed/engine errors.
     * @returns {Promise<void>} Completion after positioning, including a successful empty view.
     */
    seekToFirst() {
        return schedule([this], 'seekToFirst');
    }

    /** Seek last; an empty view succeeds and is invalid. Rejects closed/engine errors.
     * @returns {Promise<void>} Completion after positioning, including a successful empty view.
     */
    seekToLast() {
        return schedule([this], 'seekToLast');
    }

    /**
     * Seek the first key >= the target in bytewise order. No qualifying key is a
     * successful invalid position, not NOT_FOUND.
     * @param {Uint8Array} key Binary target copied at invocation.
     * @returns {Promise<void>} Completion; rejects closed, encoded-length or engine errors.
     * @throws {TypeError} for a nonbyte argument.
     */
    seek(key) {
        return schedule([this], 'seek', binary(key));
    }

    /** Advance; passing the last record succeeds and invalidates. Rejects NOT_FOUND
     * if already invalid, or closed/engine errors. Check status after a scan.
     * @returns {Promise<void>} Completion after advancement, including successful exhaustion.
     */
    next() {
        return schedule([this], 'next');
    }

    /** Move backward; passing the first record succeeds and invalidates. Rejects
     * NOT_FOUND if already invalid, or closed/engine errors.
     * @returns {Promise<void>} Completion after backward movement, including the beginning boundary.
     */
    previous() {
        return schedule([this], 'previous');
    }

    /** Resolve positioning only: false for unpositioned/exhausted/failed cursors.
     * Does not inspect engine errors; use status. Rejects after close.
     * @returns {Promise<boolean>} Whether this cursor currently refers to a record.
     */
    isValid() {
        return schedule([this], 'isValid');
    }

    /** Resolve a copied current key that survives movement/close. Rejects NOT_FOUND
     * if invalid, or a closed/engine/native allocation error.
     * @returns {Promise<Buffer>} An owned independent copy of the current binary key.
     */
    key() {
        return schedule([this], 'key');
    }

    /** Resolve a copied current value, including an empty Buffer, surviving close.
     * Rejects NOT_FOUND if invalid, or a closed/engine/native allocation error.
     * @returns {Promise<Buffer>} An owned independent copy of the current value, including empty data.
     */
    value() {
        return schedule([this], 'value');
    }

    /** Check recorded engine status without moving. Ordinary exhaustion succeeds;
     * closed cursors and recorded I/O/corruption/native errors reject.
     * @returns {Promise<void>} Success when no engine error is recorded; otherwise rejects that error.
     */
    status() {
        return schedule([this], 'status');
    }
}

module.exports = {Cursor};
