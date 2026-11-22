package io.multileveldb.errors;

/**
 * Cursor access/movement attempted without a valid current record.
 */
public final class NotFoundException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public NotFoundException(String m) {
        super(Code.NOT_FOUND, m);
    }
}
