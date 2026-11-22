const {Database} = require('../../bindings/node/src/index.cjs');
(async () => {
    const db = await Database.open('example-db', {createIfMissing: true});
    try {
        const key = Buffer.from([0, 255]);
        await db.put(key, Buffer.from([0, 128, 255]));
        console.log(await db.get(key));
    } finally {
        await db.close();
    }
})().catch(error => {
    console.error(error);
    process.exitCode = 1;
});
