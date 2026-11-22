'use strict';
const native = require('./native.cjs');

function binary(value) {
    if (!(value instanceof Uint8Array)) throw new TypeError('Expected Buffer or Uint8Array');
    // Copy at invocation time so caller mutation cannot change queued writes.
    return Buffer.from(value);
}

const states = new WeakMap();

function state(resource) {
    const s = states.get(resource);
    if (!s) throw new TypeError('Invalid resource');
    return s;
}

function schedule(resources, op, key = null, value = null, opts = []) {
    const active = resources.filter(Boolean).map(state);
    const result = Promise.all(active.map(s => s.tail)).then(() =>
        native.invoke(op, active[0]?.handle ?? null, active[1]?.handle ?? null, key, value, opts));
    const settled = result.then(() => undefined, () => undefined);
    for (const s of active) s.tail = settled;
    return result;
}

const token = Symbol('native resource');

/**
 * Owned native resource. Use Database factories rather than direct construction.
 * Explicitly await close; finalizers are only a fallback.
 * @memberof module:multileveldb
 * @inner
 */
class Resource {
    /**
     * Internal factory-only native-handle constructor.
     * @protected
     * @param {*} handle Owned opaque Node-API external.
     * @param {symbol} secret Module-private factory token; other values throw TypeError.
     */
    constructor(handle, secret) {
        if (secret !== token) throw new TypeError('Use a factory method to create resources');
        states.set(this, {handle, tail: Promise.resolve()});
    }

    /**
     * Release this wrapper after its earlier queued operations. Repeated close is
     * harmless. Later operations reject DATABASE_CLOSED; retained cursors may keep
     * the engine/snapshot and directory lock alive independently. Await cleanup;
     * native finalizers are only a nondeterministic fallback.
     * @returns {Promise<void>} A promise resolving after release, or rejecting a worker/native error.
     */
    close() {
        return schedule([this], 'close');
    }
}

module.exports = {Resource, binary, schedule, token};
