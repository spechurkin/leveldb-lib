/**
 * Binary-safe LevelDB access for Java and other JVM languages.
 * The entry point is {@link io.multileveldb.Database}. Opening, read and write
 * settings are {@link io.multileveldb.options.OpenOptions},
 * {@link io.multileveldb.options.ReadOptions} and {@link io.multileveldb.options.WriteOptions}.
 * These types, {@link io.multileveldb.Cursor}, {@link io.multileveldb.Snapshot}
 * and {@link io.multileveldb.Batch} are top-level classes and
 * own native resources. All resources are {@link java.lang.AutoCloseable};
 * use try-with-resources for deterministic release.
 *
 * <p>Native failures throw {@link io.multileveldb.errors.DbException} subclasses with
 * a stable {@link io.multileveldb.errors.Code}. Native data never passes through a
 * Java String; only database paths and diagnostic messages have text semantics.
 */
package io.multileveldb;
