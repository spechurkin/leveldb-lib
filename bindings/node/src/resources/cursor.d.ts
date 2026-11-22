import {Resource} from '../internal/resource';

/** Streaming bidirectional cursor. All access/movement/close calls are queued. */
export class Cursor extends Resource {
    /** Seek first; an empty view succeeds and is invalid. Rejects closed/engine errors. */
    seekToFirst(): Promise<void>;

    /** Seek last; an empty view succeeds and is invalid. Rejects closed/engine errors. */
    seekToLast(): Promise<void>;

    /**
     * Seek the first key >= the target in bytewise order. No qualifying key is a
     * successful invalid position, not NOT_FOUND.
     * @param key Binary target copied at invocation.
     * @returns Completion; rejects closed, encoded-length or engine errors.
     * @throws TypeError for a nonbyte argument.
     */
    seek(key: Uint8Array): Promise<void>;

    /** Advance; passing the last record succeeds and invalidates. Rejects NOT_FOUND
     * if already invalid, or closed/engine errors. Check status after a scan.
     */
    next(): Promise<void>;

    /** Move backward; passing the first record succeeds and invalidates. Rejects
     * NOT_FOUND if already invalid, or closed/engine errors.
     */
    previous(): Promise<void>;

    /** Resolve positioning only: false for unpositioned/exhausted/failed cursors.
     * Does not inspect engine errors; use status. Rejects after close.
     */
    isValid(): Promise<boolean>;

    /** Resolve a copied current key that survives movement/close. Rejects NOT_FOUND
     * if invalid, or a closed/engine/native allocation error.
     */
    key(): Promise<Buffer>;

    /** Resolve a copied current value, including an empty Buffer, surviving close.
     * Rejects NOT_FOUND if invalid, or a closed/engine/native allocation error.
     */
    value(): Promise<Buffer>;

    /** Check recorded engine status without moving. Ordinary exhaustion succeeds;
     * closed cursors and recorded I/O/corruption/native errors reject.
     */
    status(): Promise<void>;
}
