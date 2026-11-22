package io.multileveldb.internal;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;

/**
 * Internal direct mapping of the C ABI for the supported 64-bit platforms.
 * Java long maps size_t/uint64_t here; Windows NativeLong would be only 32 bits.
 * Handle and output/error ownership follow the documented native header;
 * NativeCalls copies/frees native buffers and errors before exposing Java values.
 * These functions are implementation plumbing, not a second public Java API.
 *
 * @hidden
 */
public interface NativeApi extends Library {
    NativeApi INSTANCE = load();

    static NativeApi load() {
        if (Native.POINTER_SIZE != 8) throw new UnsupportedOperationException("64-bit JVM required");
        String path = System.getProperty("multileveldb.library", System.getenv("MULTILEVELDB_LIBRARY"));
        NativeApi api = Native.load(path == null ? "multileveldb" : path, NativeApi.class);
        if ((api.ml_abi_version() >>> 16) != 1) throw new UnsatisfiedLinkError("Incompatible multileveldb ABI");
        return api;
    }

    int ml_abi_version();

    Pointer ml_error_message(Pointer e);

    void ml_error_free(Pointer e);

    void ml_buffer_free(Pointer p);

    int ml_options_create(int kind, PointerByReference out, PointerByReference e);

    int ml_options_set(Pointer o, int field, long value, PointerByReference e);

    void ml_options_destroy(Pointer o);

    int ml_db_open(byte[] p, long n, Pointer o, PointerByReference out, PointerByReference e);

    int ml_db_create(byte[] p, long n, Pointer o, PointerByReference out, PointerByReference e);

    int ml_db_close(Pointer db, PointerByReference e);

    void ml_db_destroy(Pointer db);

    int ml_db_get(Pointer db, byte[] k, long n, Pointer o, Pointer s, PointerByReference out, LongByReference len, PointerByReference e);

    int ml_db_put(Pointer db, byte[] k, long n, byte[] v, long z, Pointer o, PointerByReference e);

    int ml_db_delete(Pointer db, byte[] k, long n, Pointer o, PointerByReference e);

    int ml_iterator_create(Pointer db, Pointer o, Pointer s, PointerByReference out, PointerByReference e);

    void ml_iterator_destroy(Pointer i);

    int ml_iterator_seek_first(Pointer i, PointerByReference e);

    int ml_iterator_seek_last(Pointer i, PointerByReference e);

    int ml_iterator_seek(Pointer i, byte[] k, long n, PointerByReference e);

    int ml_iterator_next(Pointer i, PointerByReference e);

    int ml_iterator_previous(Pointer i, PointerByReference e);

    int ml_iterator_valid(Pointer i);

    int ml_iterator_key(Pointer i, PointerByReference out, LongByReference n, PointerByReference e);

    int ml_iterator_value(Pointer i, PointerByReference out, LongByReference n, PointerByReference e);

    int ml_iterator_status(Pointer i, PointerByReference e);

    int ml_batch_create(PointerByReference out, PointerByReference e);

    void ml_batch_destroy(Pointer b);

    int ml_batch_clear(Pointer b, PointerByReference e);

    int ml_batch_put(Pointer b, byte[] k, long n, byte[] v, long z, PointerByReference e);

    int ml_batch_delete(Pointer b, byte[] k, long n, PointerByReference e);

    int ml_batch_write(Pointer db, Pointer b, Pointer o, PointerByReference e);

    int ml_snapshot_create(Pointer db, PointerByReference out, PointerByReference e);

    void ml_snapshot_release(Pointer s);
}
