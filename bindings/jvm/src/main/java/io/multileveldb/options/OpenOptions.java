package io.multileveldb.options;

import io.multileveldb.errors.DbException;
import io.multileveldb.errors.InvalidArgumentException;
import io.multileveldb.errors.UnsupportedException;

/**
 * Opening settings. Read/snapshot settings and write durability are separate.
 */
public final class OpenOptions extends Options {
    /**
     * Allocate defaults: creation off, Snappy compression, engine-owned cache.
     *
     * @throws DbException native allocation failure
     */
    public OpenOptions() {
        super(0);
    }

    /**
     * Control creation of a missing database; parent directories must exist.
     *
     * @param v true to create; default false
     * @return this builder
     * @throws DbException native failure or closed options
     */
    public OpenOptions createIfMissing(boolean v) {
        set(1, v ? 1 : 0);
        return this;
    }

    /**
     * Reject an existing database when opening it; lock checks can fail first.
     *
     * @param v true to reject existing databases; default false
     * @return this builder
     * @throws DbException native failure or closed options
     */
    public OpenOptions errorIfExists(boolean v) {
        set(2, v ? 1 : 0);
        return this;
    }

    /**
     * Enable extra engine consistency checks during opening/recovery.
     *
     * @param v true for extra checks; default false
     * @return this builder
     * @throws DbException native failure or closed options
     */
    public OpenOptions paranoidChecks(boolean v) {
        set(3, v ? 1 : 0);
        return this;
    }

    /**
     * Set the in-memory write-buffer target; the engine may clamp it.
     *
     * @param v positive target in bytes; default 4 MiB
     * @return this builder
     * @throws InvalidArgumentException if not positive
     * @throws DbException              native failure or closed options
     */
    public OpenOptions writeBufferSize(long v) {
        set(4, v);
        return this;
    }

    /**
     * Set the engine's open-file budget, subject to its minimum/maximum bounds.
     *
     * @param v positive file count; default 1000
     * @return this builder
     * @throws InvalidArgumentException if not positive
     * @throws DbException              native failure or closed options
     */
    public OpenOptions maxOpenFiles(int v) {
        set(5, v);
        return this;
    }

    /**
     * Set the uncompressed table-block target, not a maximum record size.
     *
     * @param v positive byte target; default 4096
     * @return this builder
     * @throws InvalidArgumentException if not positive
     * @throws DbException              native failure or closed options
     */
    public OpenOptions blockSize(long v) {
        set(6, v);
        return this;
    }

    /**
     * Set the number of keys between block restart points.
     *
     * @param v positive key count; default 16
     * @return this builder
     * @throws InvalidArgumentException if not positive
     * @throws DbException              native failure or closed options
     */
    public OpenOptions blockRestartInterval(int v) {
        set(7, v);
        return this;
    }

    /**
     * Select compression for newly written table blocks.
     *
     * @param v compression setting; default SNAPPY
     * @return this builder
     * @throws NullPointerException if v is null
     * @throws UnsupportedException if Snappy is unavailable
     * @throws DbException          native failure or closed options
     */
    public OpenOptions compression(Compression v) {
        set(8, v.ordinal());
        return this;
    }

    /**
     * Install an owned LRU block cache; zero means zero capacity.
     * Without this setter, the engine's default cache policy applies.
     *
     * @param v nonnegative capacity in bytes
     * @return this builder
     * @throws InvalidArgumentException if negative
     * @throws DbException              native failure or closed options
     */
    public OpenOptions cacheSize(long v) {
        set(9, v);
        return this;
    }
}
