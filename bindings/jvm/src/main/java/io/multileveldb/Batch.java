package io.multileveldb;

import io.multileveldb.errors.DbException;
import io.multileveldb.internal.Resource;

import java.util.Objects;

import static io.multileveldb.internal.NativeCalls.*;

/**
 * Reusable, database-independent ordered operations for atomic write.
 * Mutations copy input arrays but do not touch a database until write is called.
 */
public final class Batch extends Resource {
    /**
     * Allocate an empty batch, independent of any database.
     *
     * @throws DbException native allocation failure
     */
    public Batch() {
        super(create(N::ml_batch_create), N::ml_batch_destroy);
    }

    /**
     * Append a copied binary put without applying it.
     *
     * @param key   binary key, possibly empty
     * @param value binary value, possibly empty
     * @return this batch
     * @throws NullPointerException if either array is null
     * @throws DbException          closed batch, engine length overflow or allocation failure
     */
    public Batch put(byte[] key, byte[] value) {
        Objects.requireNonNull(key);
        Objects.requireNonNull(value);
        use(p -> {
            call(e -> N.ml_batch_put(p, key, key.length, value, value.length, e));
            return null;
        });
        return this;
    }

    /**
     * Append a copied deletion; absence is harmless when applied.
     *
     * @param key binary key, possibly empty
     * @return this batch
     * @throws NullPointerException if key is null
     * @throws DbException          closed batch, engine length overflow or allocation failure
     */
    public Batch delete(byte[] key) {
        Objects.requireNonNull(key);
        use(p -> {
            call(e -> N.ml_batch_delete(p, key, key.length, e));
            return null;
        });
        return this;
    }

    /**
     * Remove all queued operations without changing a database.
     * The same batch remains reusable.
     *
     * @throws DbException closed batch or native failure
     */
    public void clear() {
        use(p -> {
            call(e -> N.ml_batch_clear(p, e));
            return null;
        });
    }
}
