package io.multileveldb.options;

import io.multileveldb.errors.DbException;

/**
 * Per-write durability settings for put, delete and atomic batch application.
 */
public final class WriteOptions extends Options {
    /**
     * Allocate defaults with synchronous flushing disabled.
     *
     * @throws DbException native allocation failure
     */
    public WriteOptions() {
        super(2);
    }

    /**
     * Request LevelDB's synchronous storage flush before a write succeeds.
     *
     * @param v true for synchronous writes; default false
     * @return this builder
     * @throws DbException native failure or closed options
     */
    public WriteOptions sync(boolean v) {
        set(12, v ? 1 : 0);
        return this;
    }
}
