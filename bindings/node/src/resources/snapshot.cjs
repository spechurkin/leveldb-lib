'use strict';
const {Resource} = require('../internal/resource.cjs');

/**
 * Immutable engine view from Database.createSnapshot. Use only with its original
 * DB. Close releases this reference; snapshot cursors retain their references.
 * @memberof module:multileveldb
 * @inner
 * @augments module:multileveldb~Resource
 */
class Snapshot extends Resource {
}

module.exports = {Snapshot};
