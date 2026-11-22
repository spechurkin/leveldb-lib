package io.multileveldb;

import com.sun.jna.Pointer;
import io.multileveldb.errors.*;
import io.multileveldb.internal.NativeApi;
import io.multileveldb.internal.NativeCalls;
import io.multileveldb.internal.Resource;
import io.multileveldb.options.OpenOptions;
import io.multileveldb.options.ReadOptions;
import io.multileveldb.options.WriteOptions;

import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.Objects;

import static io.multileveldb.internal.NativeCalls.*;

/**
 * Binary access to a Google LevelDB directory through native ABI 1.
 * Keys and values are never text-decoded. A missing get returns {@code null};
 * an existing empty value returns a zero-length array. Returned arrays are owned
 * copies. Use try-with-resources for this handle and every dependent resource.
 *
 * <p>Operations serialize per handle, including explicit close and fallback
 * cleanup. Closing the database does not invalidate existing cursors: native
 * cursors/snapshots retain the engine and directory lock until released.
 * The default comparator is bytewise. Native failures throw a typed
 * {@link DbException}; its {@link DbException#code} is stable, its message is
 * the original diagnostic. Null byte-array arguments throw
 * {@link NullPointerException}. Loading requires a matching 64-bit native
 * library, selected by {@code multileveldb.library},
 * {@code MULTILEVELDB_LIBRARY}, or JNA's normal search path.
 * Missing/incompatible native code fails class initialization with a linkage or
 * initializer error, before the ordinary DbException operation contract applies.
 */
public final class Database implements AutoCloseable {
    private static final NativeApi N = NativeCalls.N;

    private final Resource resource;

    private Database(Pointer p) {
        resource = new Resource(p, N::ml_db_destroy) {
        };
    }

    /**
     * Open a database using copied settings. Relative paths use the working directory.
     *
     * @param path    nonempty Unicode path without NUL or unpaired surrogates
     * @param options opening settings, or null for engine defaults
     * @return an owned database; close explicitly
     * @throws NullPointerException     if path is null
     * @throws InvalidArgumentException invalid path/options, missing DB without creation,
     *                                  or existing DB when errorIfExists applies
     * @throws LockedException          identified directory-lock contention
     * @throws DbException              I/O, corruption or another native failure
     */
    public static Database open(Path path, OpenOptions options) {
        return open(path, options, false);
    }

    private static Database open(Path path, OpenOptions options, boolean createNew) {
        Objects.requireNonNull(path);
        String text = path.toString();
        // Java's UTF-8 encoder replaces unpaired surrogates; reject those paths.
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        if (!new String(bytes, StandardCharsets.UTF_8).equals(text))
            throw new InvalidArgumentException("Path contains unpaired surrogates");
        return optional(options, o -> new Database(NativeCalls.create((p, e) -> createNew
                ? N.ml_db_create(bytes, bytes.length, o, p, e) : N.ml_db_open(bytes, bytes.length, o, p, e))));
    }

    /**
     * Open an existing database with engine defaults; creation is disabled.
     *
     * @param path Unicode database directory
     * @return an owned database
     * @throws NullPointerException if path is null
     * @throws DbException          invalid path, missing/locked DB, I/O or corruption
     * @see #open(Path, OpenOptions)
     */
    public static Database open(Path path) {
        return open(path, null);
    }

    /**
     * Create a new database and return its open handle. Parent directories must exist.
     * Copies settings, forcing createIfMissing and errorIfExists true without modifying options.
     *
     * @param path    nonempty Unicode database directory without NUL or unpaired surrogates
     * @param options storage settings, or null for defaults
     * @return an owned database; close explicitly
     * @throws NullPointerException     if path is null
     * @throws InvalidArgumentException existing database or invalid path/options
     * @throws LockedException          directory contention, which may be checked before existence
     * @throws DbException              I/O, corruption or another native failure
     */
    public static Database create(Path path, OpenOptions options) {
        return open(path, options, true);
    }

    /**
     * Create a new database using default storage settings.
     *
     * @param path Unicode database directory; parent directories must exist
     * @return an owned database
     * @throws NullPointerException if path is null
     * @throws DbException          existing/locked database, invalid path or engine failure
     * @see #create(Path, OpenOptions)
     */
    public static Database create(Path path) {
        return create(path, null);
    }

    /**
     * Read the current view with default read settings.
     *
     * @param key binary key, possibly empty
     * @return owned bytes, including an empty array, or null if absent
     * @throws NullPointerException if key is null
     * @throws DbException          closed DB, invalid engine length or engine failure
     * @see #get(byte[], ReadOptions, Snapshot)
     */
    public byte[] get(byte[] key) {
        return get(key, null, null);
    }

    /**
     * Read one binary key without text conversion.
     *
     * @param key      binary key, possibly empty
     * @param options  read settings, or null for defaults
     * @param snapshot read view from this exact database instance, or null for current data
     * @return independent bytes; null only for a missing key
     * @throws NullPointerException     if key is null
     * @throws InvalidArgumentException foreign snapshot or engine length overflow
     * @throws UnsupportedException     if the value exceeds Java byte-array capacity
     * @throws DbException              a closed participating resource or engine/native failure
     */
    public byte[] get(byte[] key, ReadOptions options, Snapshot snapshot) {
        Objects.requireNonNull(key);
        try {
            return optional(resource, db -> optional(options, o -> optional(snapshot, s -> read((p, n, e) -> N.ml_db_get(db, key, key.length, o, s, p, n, e)))));
        } catch (NotFoundException e) {
            return null;
        }
    }

    /**
     * Insert/replace a binary value using default, nonsynchronous write settings.
     *
     * @param key   binary key, possibly empty
     * @param value binary value, possibly empty
     * @throws NullPointerException if either array is null
     * @throws DbException          closed DB, invalid engine length or write failure
     */
    public void put(byte[] key, byte[] value) {
        put(key, value, null);
    }

    /**
     * Insert/replace exactly the supplied bytes; arrays are borrowed for the call.
     *
     * @param key     binary key, possibly empty
     * @param value   binary value, possibly empty
     * @param options write durability, or null for defaults
     * @throws NullPointerException if either array is null
     * @throws DbException          closed resource, invalid engine length or write failure
     */
    public void put(byte[] key, byte[] value, WriteOptions options) {
        Objects.requireNonNull(key);
        Objects.requireNonNull(value);
        optional(resource, db -> optional(options, o -> {
            call(e -> N.ml_db_put(db, key, key.length, value, value.length, o, e));
            return null;
        }));
    }

    /**
     * Delete a key using default settings; an already absent key also succeeds.
     *
     * @param key binary key, possibly empty
     * @throws NullPointerException if key is null
     * @throws DbException          closed DB, invalid engine length or write failure
     */
    public void delete(byte[] key) {
        delete(key, null);
    }

    /**
     * Delete a key with the supplied durability; no existence result is returned.
     *
     * @param key     binary key, possibly empty
     * @param options write settings, or null for defaults
     * @throws NullPointerException if key is null
     * @throws DbException          closed resource, invalid engine length or write failure
     */
    public void delete(byte[] key, WriteOptions options) {
        Objects.requireNonNull(key);
        optional(resource, db -> optional(options, o -> {
            call(e -> N.ml_db_delete(db, key, key.length, o, e));
            return null;
        }));
    }

    /**
     * Allocate an empty, database-independent batch while this handle is open.
     *
     * @return owned batch; close after use
     * @throws DbException closed DB or native allocation failure
     */
    public Batch createBatch() {
        return optional(resource, db -> new Batch());
    }

    /**
     * Atomically apply a batch with default write durability, without clearing it.
     *
     * @param batch reusable operation sequence
     * @throws NullPointerException if batch is null
     * @throws DbException          closed resource or engine write failure
     */
    public void write(Batch batch) {
        write(batch, null);
    }

    /**
     * Apply operations atomically in insertion order; the batch remains reusable.
     * This does not supply read transactions or conflict detection.
     *
     * @param batch   operation sequence, not consumed or cleared
     * @param options write settings, or null for defaults
     * @throws NullPointerException if batch is null
     * @throws DbException          closed participating resource or engine write failure
     */
    public void write(Batch batch, WriteOptions options) {
        Objects.requireNonNull(batch);
        optional(resource, db -> optional(batch, b -> optional(options, o -> {
            call(e -> N.ml_batch_write(db, b, o, e));
            return null;
        })));
    }

    /**
     * Capture an immutable native read view; it may retain older table versions.
     *
     * @return owned snapshot for this database instance
     * @throws DbException closed DB or native failure
     */
    public Snapshot createSnapshot() {
        return optional(resource, db -> new Snapshot(NativeCalls.create((p, e) -> N.ml_snapshot_create(db, p, e))));
    }

    /**
     * Create an unpositioned streaming cursor with default current-view reads.
     *
     * @return owned cursor; seek before accessing records
     * @throws DbException closed DB or engine/native failure
     */
    public Cursor iterator() {
        return iterator(null, null);
    }

    /**
     * Create an unpositioned streaming cursor, ordered by the bytewise comparator.
     *
     * @param options  read settings, or null for defaults; copied by this call
     * @param snapshot same-instance snapshot, or null for a current-view cursor
     * @return owned cursor retaining engine/snapshot even after their wrappers close
     * @throws InvalidArgumentException if snapshot belongs to another database
     * @throws DbException              closed resource or engine/native failure
     */
    public Cursor iterator(ReadOptions options, Snapshot snapshot) {
        return optional(resource, db -> optional(options, o -> optional(snapshot, s -> new Cursor(NativeCalls.create((p, e) -> N.ml_iterator_create(db, o, s, p, e))))));
    }

    /**
     * Close this handle once; repeated calls do nothing and no data is deleted.
     * Subsequent DB operations throw ClosedException. Existing cursors/snapshots
     * retain the engine and may keep its directory locked until they close.
     */
    @Override
    public void close() {
        resource.close();
    }
}
