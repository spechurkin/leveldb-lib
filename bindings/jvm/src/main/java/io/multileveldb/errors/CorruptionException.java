package io.multileveldb.errors;

/**
 * Engine-reported malformed or corrupt stored data.
 */
public final class CorruptionException extends DbException {
    /**
     * Construct this failure with the original native diagnostic.
     *
     * @param m diagnostic message
     */
    public CorruptionException(String m) {
        super(Code.CORRUPTION, m);
    }
}
