package io.multileveldb;

import com.sun.jna.Pointer;
import io.multileveldb.errors.ClosedException;
import io.multileveldb.errors.DbException;
import io.multileveldb.errors.NotFoundException;
import io.multileveldb.internal.Resource;
import io.multileveldb.options.ReadOptions;

import java.util.Objects;

import static io.multileveldb.internal.NativeCalls.*;

/**
 * Streaming bidirectional cursor; initially unpositioned and explicitly closeable.
 * Returned arrays are independent copies. Normal exhaustion is not an error;
 * check status after a scan. Calls synchronize against movement and close.
 * Create cursors with {@link Database#iterator()} or
 * {@link Database#iterator(ReadOptions, Snapshot)}.
 */
public final class Cursor extends Resource {
    Cursor(Pointer p) {
        super(p, N::ml_iterator_destroy);
    }

    /**
     * Seek to the first record; empty views succeed with an invalid position.
     *
     * @throws DbException closed cursor or engine/native failure
     */
    public void seekToFirst() {
        use(p -> {
            call(e -> N.ml_iterator_seek_first(p, e));
            return null;
        });
    }

    /**
     * Seek to the last record; empty views succeed with an invalid position.
     *
     * @throws DbException closed cursor or engine/native failure
     */
    public void seekToLast() {
        use(p -> {
            call(e -> N.ml_iterator_seek_last(p, e));
            return null;
        });
    }

    /**
     * Seek to the first key greater than or equal to the supplied binary key.
     * No matching key is a successful invalid position.
     *
     * @param key binary seek target, possibly empty
     * @throws NullPointerException if key is null
     * @throws DbException          closed cursor, invalid engine length or engine failure
     */
    public void seek(byte[] key) {
        Objects.requireNonNull(key);
        use(p -> {
            call(e -> N.ml_iterator_seek(p, key, key.length, e));
            return null;
        });
    }

    /**
     * Advance; moving past the final record succeeds and invalidates the cursor.
     *
     * @throws NotFoundException if already invalid
     * @throws DbException       closed cursor or engine/native failure
     */
    public void next() {
        use(p -> {
            call(e -> N.ml_iterator_next(p, e));
            return null;
        });
    }

    /**
     * Move backward; passing the first record succeeds and invalidates the cursor.
     *
     * @throws NotFoundException if already invalid
     * @throws DbException       closed cursor or engine/native failure
     */
    public void previous() {
        use(p -> {
            call(e -> N.ml_iterator_previous(p, e));
            return null;
        });
    }

    /**
     * Test positioning without inspecting the engine error status.
     *
     * @return true for a current record; false for unpositioned/exhausted/failed cursors
     * @throws ClosedException after close
     * @see #status()
     */
    public boolean isValid() {
        return use(p -> N.ml_iterator_valid(p) != 0);
    }

    /**
     * Check the engine error without moving; ordinary exhaustion succeeds.
     *
     * @throws DbException closed cursor or the recorded engine/native failure
     */
    public void status() {
        use(p -> {
            call(e -> N.ml_iterator_status(p, e));
            return null;
        });
    }

    /**
     * Copy the current binary key; it survives movement and close.
     *
     * @return independent key bytes
     * @throws NotFoundException if invalid
     * @throws DbException       closed cursor, Java capacity limit or engine failure
     */
    public byte[] key() {
        return use(i -> read((p, n, e) -> N.ml_iterator_key(i, p, n, e)));
    }

    /**
     * Copy the current binary value, including a valid zero-byte value.
     *
     * @return independent value bytes, surviving movement/close
     * @throws NotFoundException if invalid
     * @throws DbException       closed cursor, Java capacity limit or engine failure
     */
    public byte[] value() {
        return use(i -> read((p, n, e) -> N.ml_iterator_value(i, p, n, e)));
    }
}
