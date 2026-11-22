package io.multileveldb.errors;

/**
 * An identified database-directory lock conflict.
 */
public final class LockedException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public LockedException(String m) {
        super(Code.DATABASE_LOCKED, m);
    }
}
