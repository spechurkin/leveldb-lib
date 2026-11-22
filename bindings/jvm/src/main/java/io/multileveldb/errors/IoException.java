package io.multileveldb.errors;

/**
 * Engine filesystem I/O failure with the original diagnostic.
 */
public final class IoException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public IoException(String m) {
        super(Code.IO_ERROR, m);
    }
}
