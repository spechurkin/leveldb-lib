package io.multileveldb.internal;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;
import io.multileveldb.errors.*;

import java.util.function.Function;

/**
 * Internal native invocation helpers shared by the managed resource wrappers.
 * Copies diagnostics and returned buffers before freeing their native storage.
 *
 * @hidden
 */
public final class NativeCalls {
    public static final NativeApi N = NativeApi.INSTANCE;

    private NativeCalls() {
    }

    private static void check(int code, PointerByReference error) {
        Pointer p = error.getValue();
        String message = "";
        try {
            if (p != null) message = N.ml_error_message(p).getString(0, "UTF-8");
        } finally {
            N.ml_error_free(p);
        }
        switch (code) {
            case 0:
                return;
            case 1:
                throw new NotFoundException(message);
            case 2:
                throw new InvalidArgumentException(message);
            case 3:
                throw new IoException(message);
            case 4:
                throw new CorruptionException(message);
            case 5:
                throw new LockedException(message);
            case 6:
                throw new ClosedException(message);
            case 7:
                throw new UnsupportedException(message);
            default:
                throw new DbException(Code.INTERNAL_ERROR, message);
        }
    }

    public static void call(Call fn) {
        PointerByReference e = new PointerByReference();
        check(fn.run(e), e);
    }

    public static Pointer create(Create fn) {
        PointerByReference p = new PointerByReference();
        call(e -> fn.run(p, e));
        return p.getValue();
    }

    public static byte[] read(Read fn) {
        PointerByReference p = new PointerByReference();
        LongByReference n = new LongByReference();
        call(e -> fn.run(p, n, e));
        try {
            if (n.getValue() > Integer.MAX_VALUE) throw new UnsupportedException("Value exceeds JVM byte[] capacity");
            return p.getValue().getByteArray(0, (int) n.getValue());
        } finally {
            N.ml_buffer_free(p.getValue());
        }
    }

    public static <T> T optional(Resource r, Function<Pointer, T> fn) {
        return r == null ? fn.apply(null) : r.use(fn);
    }

    @FunctionalInterface
    public interface Call {
        int run(PointerByReference e);
    }

    @FunctionalInterface
    public interface Create {
        int run(PointerByReference p, PointerByReference e);
    }

    @FunctionalInterface
    public interface Read {
        int run(PointerByReference p, LongByReference n, PointerByReference e);
    }
}
