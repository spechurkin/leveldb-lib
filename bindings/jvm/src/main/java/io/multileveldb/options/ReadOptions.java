package io.multileveldb.options;

import io.multileveldb.errors.DbException;

/**
 * Per-read settings; pass a snapshot separately to get or iterator.
 */
public final class ReadOptions extends Options {
    /**
     * Allocate defaults: checksum verification off, cache filling on.
     *
     * @throws DbException native allocation failure
     */
    public ReadOptions() {
        super(1);
    }

    /**
     * Control verification of checksums for blocks read from storage.
     *
     * @param v true to verify; default false
     * @return this builder
     * @throws DbException native failure or closed options
     */
    public ReadOptions verifyChecksums(boolean v) {
        set(10, v ? 1 : 0);
        return this;
    }

    /**
     * Control cache population; false still permits using cached blocks.
     *
     * @param v true to populate the cache; default true
     * @return this builder
     * @throws DbException native failure or closed options
     */
    public ReadOptions fillCache(boolean v) {
        set(11, v ? 1 : 0);
        return this;
    }
}
