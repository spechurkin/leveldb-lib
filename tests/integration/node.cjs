const fs = require('node:fs');
const assert = require('node:assert/strict');
const {Database} = require('../../bindings/node/src/index.cjs');
(async () => {
    const db = await Database.open(process.env.ML_INTEROP_PATH);
    try {
        const decode = s => s === '-' ? Buffer.alloc(0) : Buffer.from(s, 'hex');
        const lines = fs.readFileSync(process.env.ML_FIXTURE, 'utf8').split(/\r?\n/).filter(s => s && !s.startsWith('#'));
        for (const line of lines) {
            const [k, v] = line.split('\t');
            assert.deepEqual(await db.get(decode(k)), decode(v));
            await db.put(decode(k), Buffer.concat([decode(v), Buffer.from([0, 254])]));
        }
    } finally {
        await db.close();
    }
})().catch(e => {
    console.error(e);
    process.exitCode = 1;
});
