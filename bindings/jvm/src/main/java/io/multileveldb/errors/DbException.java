package io.multileveldb.errors;

/**
 * Base unchecked native failure, carrying its stable code and diagnostic.
 */
public class DbException extends RuntimeException {
    /**
     * Machine-readable category independent of the human-readable message.
     */
    public final Code code;

    /**
     * Construct an adapter/native failure with an owned Java message.
     *
     * @param c stable category
     * @param m diagnostic message
     */
    public DbException(Code c, String m) {
        super(m);
        code = c;
    }
}
