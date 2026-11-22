import {Database, type Batch, type Cursor, type DbError, type OpenOptions, type Snapshot} from 'multileveldb';

async function exercise() {
    const opts: OpenOptions = {createIfMissing: true, compression: 'none'};
    const db = await Database.create('types-db', opts);
    try {
        await db.put(new Uint8Array([0, 255]), Buffer.from([0, 128]), {sync: true});
        const value: Buffer | null = await db.get(Buffer.alloc(0));
        const snapshot: Snapshot = await db.createSnapshot();
        const batch: Batch = await db.createBatch();
        await batch.put(Buffer.alloc(0), value ?? Buffer.alloc(0));
        await db.write(batch);
        await batch.close();
        const cursor: Cursor = await db.iterator({fillCache: false}, snapshot);
        await snapshot.close();
        await cursor.seekToFirst();
        if (await cursor.isValid()) {
            const key: Buffer = await cursor.key();
            await cursor.seek(key);
        }
        await cursor.close();
    } catch (error) {
        console.error((error as DbError).code);
    } finally {
        await db.close();
    }
}

void exercise;
