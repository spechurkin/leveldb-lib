package io.multileveldb.errors;

/**
 * An operation attempted to use a closed managed resource.
 */
public final class ClosedException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public ClosedException(String m) {
        super(Code.DATABASE_CLOSED, m);
    }
}
