import {Resource} from '../internal/resource';

/** Reusable, database-independent ordered puts/deletions; close when finished. */
export class Batch extends Resource {
    /**
     * Append a copied put without writing to a database.
     * @param key Binary key, possibly empty, copied at invocation.
     * @param value Binary value, possibly empty, copied at invocation.
     * @returns Completion, not the batch; rejects DbError if closed, oversized or allocation fails.
     * @throws TypeError for nonbyte arguments.
     */
    put(key: Uint8Array, value: Uint8Array): Promise<void>;

    /**
     * Append a copied deletion. An absent key is harmless when applied.
     * @param key Binary key, possibly empty, copied at invocation.
     * @returns Completion; rejects DbError if closed, oversized or allocation fails.
     * @throws TypeError for a nonbyte argument.
     */
    delete(key: Uint8Array): Promise<void>;

    /** Discard all pending operations, keeping the batch reusable. No DB is modified.
     * @returns Completion; rejects DbError after close or on native failure.
     */
    clear(): Promise<void>;
}
