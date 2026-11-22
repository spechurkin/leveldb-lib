package io.multileveldb.errors;

/**
 * Stable failure categories; messages must not be parsed to classify errors.
 */
public enum Code {
    /**
     * An iterator has no current record. Missing gets return null instead.
     */
    NOT_FOUND,
    /**
     * Invalid input, option, engine length, or foreign snapshot.
     */
    INVALID_ARGUMENT,
    /**
     * Filesystem failure, including errors that cannot be classified further.
     */
    IO_ERROR,
    /**
     * Malformed or corrupt data reported by the engine.
     */
    CORRUPTION,
    /**
     * Identified contention for the database directory lock.
     */
    DATABASE_LOCKED,
    /**
     * A database or other managed resource has been closed.
     */
    DATABASE_CLOSED,
    /**
     * An unavailable feature or a value exceeding Java array capacity.
     */
    UNSUPPORTED,
    /**
     * An unexpected native exception or unrecognized native status.
     */
    INTERNAL_ERROR
}
