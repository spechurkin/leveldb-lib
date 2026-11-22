'use strict';
const path = require('node:path');
const fs = require('node:fs');
const candidates = [process.env.MULTILEVELDB_NODE,
    path.join(__dirname, '..', '..', 'prebuilds', `${process.platform}-${process.arch}`, 'multileveldb.node'),
    path.join(__dirname, '..', '..', '..', '..', 'build', 'bindings', 'node', 'multileveldb.node')].filter(Boolean);
const file = candidates.find(p => fs.existsSync(p));
if (!file) throw new Error(`No multileveldb native addon for ${process.platform}-${process.arch}. Install a matching prebuilt package or set MULTILEVELDB_NODE.`);
const native = require(file);

module.exports = native;
