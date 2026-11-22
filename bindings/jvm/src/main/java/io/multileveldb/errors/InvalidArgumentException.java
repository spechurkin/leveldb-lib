package io.multileveldb.errors;

/**
 * Invalid option, path, encoded length, or snapshot/database pairing.
 */
public final class InvalidArgumentException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public InvalidArgumentException(String m) {
        super(Code.INVALID_ARGUMENT, m);
    }
}
