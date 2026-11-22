import api, {type Database} from 'multileveldb';

const db: Database = await api.Database.create('esm-types');
await db.put(new Uint8Array([0]), new Uint8Array([255]));
const value: Buffer | null = await db.get(Buffer.alloc(0));
void value;
await db.close();
