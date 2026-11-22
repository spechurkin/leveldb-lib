/** Owned native resource. Instances are produced by factories, never directly. */
export declare class Resource {
    /** Internal native-handle construction; use Database factory methods. */
    protected constructor();

    /**
     * Release this wrapper after its earlier queued operations. Repeated close is
     * harmless. Later operations reject DATABASE_CLOSED; retained cursors may keep
     * the engine/snapshot and directory lock alive independently. Await cleanup;
     * native finalizers are only a nondeterministic fallback.
     * @returns A promise resolving after release, or rejecting a worker/native error.
     */
    close(): Promise<void>;
}
