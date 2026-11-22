'use strict';
const openFields = {
    createIfMissing: 1, errorIfExists: 2, paranoidChecks: 3,
    writeBufferSize: 4, maxOpenFiles: 5, blockSize: 6, blockRestartInterval: 7, compression: 8, cacheSize: 9
};
const readFields = {verifyChecksums: 10, fillCache: 11};
const writeFields = {sync: 12};
const booleans = new Set([1, 2, 3, 10, 11, 12]);

function options(value = {}, fields) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) throw new TypeError('Options must be an object');
    return Object.entries(value).flatMap(([key, v]) => {
        const field = fields[key];
        if (!field) throw new TypeError(`Unknown option: ${key}`);
        if (booleans.has(field)) {
            if (typeof v !== 'boolean') throw new TypeError(`${key} must be boolean`);
            v = Number(v);
        } else if (key === 'compression') {
            if (v !== 'none' && v !== 'snappy') throw new TypeError('compression must be none or snappy');
            v = v === 'snappy' ? 1 : 0;
        } else if (!Number.isSafeInteger(v) || v < 0) throw new TypeError(`${key} must be a nonnegative safe integer`);
        return [field, v];
    });
}

module.exports = {options, openFields, readFields, writeFields};
