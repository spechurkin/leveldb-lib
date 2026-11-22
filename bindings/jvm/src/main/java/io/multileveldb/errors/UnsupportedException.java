package io.multileveldb.errors;

/**
 * A requested native feature or Java representation is unavailable.
 */
public final class UnsupportedException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public UnsupportedException(String m) {
        super(Code.UNSUPPORTED, m);
    }
}
