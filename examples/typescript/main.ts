import {Database} from 'multileveldb';

const db = await Database.open('example-db', {createIfMissing: true});
try {
    const key = new Uint8Array([0, 255]);
    await db.put(key, new Uint8Array([0, 128, 255]));
    const value: Buffer | null = await db.get(key);
    console.log(value);
} finally {
    await db.close();
}
