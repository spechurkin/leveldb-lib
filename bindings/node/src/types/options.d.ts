/** Opening/engine tuning, copied at invocation. Unknown fields are rejected. */
export interface OpenOptions {
    /** Create a missing database; parents must exist. Default false. */
    createIfMissing?: boolean;
    /** Fail if a database already exists; lock acquisition may fail first. Default false. */
    errorIfExists?: boolean;
    /** Enable extra opening/recovery consistency checks. Default false. */
    paranoidChecks?: boolean;
    /** Positive write-buffer byte target, default 4 MiB; engine bounds may clamp it. */
    writeBufferSize?: number;
    /** Positive C-int open-file budget, default 1000; engine bounds may clamp it. */
    maxOpenFiles?: number;
    /** Positive uncompressed block byte target, default 4096; not a record-size limit. */
    blockSize?: number;
    /** Positive C-int key count between block restart points; default 16. */
    blockRestartInterval?: number;
    /** Compression of newly written blocks; existing tables are unchanged. Default 'snappy'. */
    compression?: 'none' | 'snappy';
    /** Owned LRU byte capacity; zero gives zero capacity. Unset uses engine cache policy. */
    cacheSize?: number;
}

/** Settings copied by a get or cursor-creation call. Pass a snapshot separately. */
export interface ReadOptions {
    /** Verify checksums of blocks read from storage. Default false. */
    verifyChecksums?: boolean;
    /** Populate the block cache. False still permits cached reads. Default true. */
    fillCache?: boolean;
}

/** Settings copied by put, delete, or atomic batch write. */
export interface WriteOptions {
    /** Request LevelDB's synchronous storage flush before success. Default false. */
    sync?: boolean;
}
