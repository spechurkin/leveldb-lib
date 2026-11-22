package io.multileveldb.options;

/**
 * Compression for new table blocks; existing tables are not rewritten.
 */
public enum Compression {
    /**
     * Write uncompressed blocks.
     */
    NONE,
    /**
     * Use official Snappy compression, the engine default.
     */
    SNAPPY
}
