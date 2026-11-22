import {Resource} from '../internal/resource';
import type {OpenOptions, ReadOptions, WriteOptions} from '../types/options';
import type {Snapshot} from './snapshot';
import type {Batch} from './batch';
import type {Cursor} from './cursor';

/**
 * An owned binary LevelDB handle using C ABI 1 and the bytewise comparator.
 * Blocking calls run on Node-API workers. Calls sharing resources are queued in
 * invocation order. Input Uint8Arrays are copied immediately, returned Buffers
 * are owned copies, and no key/value text conversion occurs.
 */
export class Database extends Resource {
    /**
     * Create a new LevelDB and return its open handle. Parents must exist.
     * @param path Nonempty well-formed Unicode path without NUL; relative to cwd.
     * @param options Copied storage settings; createIfMissing and errorIfExists are forced true.
     * @returns Owned handle. Rejects DbError with INVALID_ARGUMENT for an existing database;
     * lock checks may reject DATABASE_LOCKED first. Other failures match open.
     * @throws TypeError for invalid path or option argument shapes.
     */
    static create(path: string, options?: OpenOptions): Promise<Database>;

    /**
     * Open a standard LevelDB directory; absent options select engine defaults.
     * @param path Nonempty well-formed Unicode path without NUL; relative to cwd.
     * @param options Opening settings; numeric values must be nonnegative safe integers.
     * @returns Owned handle. Rejects TypeError for JS argument shapes, or DbError
     * for invalid/missing/locked paths, existence policy, I/O, corruption or allocation.
     * Parents must exist; creation is disabled by default.
     */
    static open(path: string, options?: OpenOptions): Promise<Database>;

    /**
     * Read exactly one binary key.
     * @param key Buffer/Uint8Array, possibly empty; copied before queuing.
     * @param options Read settings; undefined selects defaults.
     * @param snapshot Read view from this exact database instance, or undefined for current data.
     * @returns Owned Buffer, including length zero, or null if absent. Rejects
     * DbError for closed resources, foreign snapshots, engine lengths, I/O or corruption.
     * @throws TypeError for invalid key, options, or snapshot argument shape.
     */
    get(key: Uint8Array, options?: ReadOptions, snapshot?: Snapshot): Promise<Buffer | null>;

    /**
     * Insert/replace a value; empty keys and values are valid.
     * @param key Binary key copied at invocation.
     * @param value Binary value copied at invocation.
     * @param options Durability settings; undefined means nonsynchronous defaults.
     * @returns Completion; rejects DbError for closed resources, length overflow or write failures.
     * @throws TypeError for nonbyte arguments or malformed options.
     */
    put(key: Uint8Array, value: Uint8Array, options?: WriteOptions): Promise<void>;

    /**
     * Delete a binary key; an already absent key also succeeds.
     * @param key Binary key copied before queuing.
     * @param options Write durability, or undefined for defaults.
     * @returns Completion; rejects DbError for closed resources, invalid length or write failure.
     * @throws TypeError for invalid key/options shapes.
     */
    delete(key: Uint8Array, options?: WriteOptions): Promise<void>;

    /**
     * Allocate an empty, independently owned batch while this database is open.
     * @returns Batch not tied to this DB; rejects DbError if closed or allocation fails.
     */
    createBatch(): Promise<Batch>;

    /**
     * Atomically apply operations in insertion order. The batch is not consumed or
     * cleared and may be reused with another DB. This adds no read transaction.
     * @param batch Live reusable batch; its queue participates in write ordering.
     * @param options Write durability, or undefined for defaults.
     * @returns Completion; rejects DbError for closed participants or engine write failure.
     * @throws TypeError for invalid batch/options shapes.
     */
    write(batch: Batch, options?: WriteOptions): Promise<void>;

    /**
     * Capture an immutable engine read view; it may retain older table versions.
     * @returns Owned snapshot for this DB; rejects DbError if closed or allocation fails.
     */
    createSnapshot(): Promise<Snapshot>;

    /**
     * Create an initially unpositioned streaming cursor; seek before accessing data.
     * @param options Read settings, or undefined for defaults; copied at invocation.
     * @param snapshot Same-instance snapshot, or undefined for a current-view cursor.
     * @returns Owned cursor retaining engine/snapshot after their wrappers close.
     * Rejects TypeError for argument shapes or DbError for foreign/closed resources
     * and engine/native creation failures.
     */
    iterator(options?: ReadOptions, snapshot?: Snapshot): Promise<Cursor>;
}
