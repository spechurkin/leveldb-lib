package io.multileveldb;

import com.sun.jna.Pointer;
import io.multileveldb.errors.ClosedException;
import io.multileveldb.errors.InvalidArgumentException;
import io.multileveldb.errors.LockedException;
import io.multileveldb.internal.NativeCalls;
import io.multileveldb.internal.Resource;
import io.multileveldb.options.Compression;
import io.multileveldb.options.OpenOptions;
import io.multileveldb.options.ReadOptions;
import io.multileveldb.options.WriteOptions;

import java.lang.ref.WeakReference;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;

public final class IntegrationTest {
    public static void main(String[] args) throws Exception {
        cleanup();
        createNew();
        Path path = Files.createTempDirectory(Paths.get(System.getenv().getOrDefault("ML_TEST_ROOT", System.getProperty("java.io.tmpdir"))), "multileveldb-java-данные-").resolve("db");
        byte[] key = {0, -1}, value = {0, -128, -1};
        try (OpenOptions o = new OpenOptions().createIfMissing(true).compression(Compression.NONE)
                .errorIfExists(false).paranoidChecks(true).writeBufferSize(65536).maxOpenFiles(40)
                .blockSize(4096).blockRestartInterval(16).cacheSize(1048576);
             ReadOptions r = new ReadOptions().verifyChecksums(true).fillCache(false);
             WriteOptions w = new WriteOptions().sync(true);
             Database db = Database.open(path, o)) {
            db.put(key, value, w);
            assert Arrays.equals(db.get(key, r, null), value);
            try {
                Database.open(path).close();
                throw new AssertionError("Expected lock error");
            } catch (LockedException expected) {
            }
            try (Snapshot s = db.createSnapshot(); Batch b = db.createBatch()) {
                b.put(key, new byte[0]).put(new byte[]{1}, value).delete(new byte[]{1});
                db.write(b);
                assert db.get(key).length == 0;
                assert Arrays.equals(db.get(key, null, s), value);
                try (Cursor i = db.iterator(null, s)) {
                    s.close();
                    i.seek(key);
                    assert i.isValid();
                    assert Arrays.equals(i.value(), value);
                    i.seekToFirst();
                    i.seekToLast();
                    i.previous();
                    assert !i.isValid();
                    i.seekToFirst();
                    assert Arrays.equals(i.key(), key);
                    i.next();
                    assert !i.isValid();
                    i.status();
                }
                b.clear();
            }
            db.delete(key);
            assert db.get(key) == null;
            byte[] large = new byte[4 * 1024 * 1024];
            Arrays.fill(large, (byte) 255);
            db.put(key, large);
            assert Arrays.equals(db.get(key), large);
        }
        try (Database db = Database.open(path)) {
            assert db.get(key).length == 4 * 1024 * 1024;
            db.close();
            db.close();
        }
    }

    private static WeakReference<Resource> unclosed(AtomicInteger frees) {
        Resource resource = new Resource(new Pointer(1), p -> frees.incrementAndGet()) {
        };
        return new WeakReference<>(resource);
    }

    private static void cleanup() throws Exception {
        AtomicInteger frees = new AtomicInteger();
        Resource resource = new Resource(new Pointer(1), p -> frees.incrementAndGet()) {
        };
        resource.close();
        resource.close();
        assert frees.get() == 1;
        try {
            NativeCalls.optional(resource, p -> null);
            throw new AssertionError("Used a closed handle");
        } catch (ClosedException expected) {
        }
        WeakReference<Resource> abandoned = unclosed(frees);
        for (int i = 0; i < 500 && frees.get() != 2; i++) {
            System.gc();
            Thread.sleep(20);
        }
        assert abandoned.get() == null;
        assert frees.get() == 2 : "Fallback cleanup did not release the handle";
    }

    private static void createNew() throws Exception {
        Path root = Files.createTempDirectory(Paths.get(System.getenv().getOrDefault("ML_TEST_ROOT", System.getProperty("java.io.tmpdir"))), "multileveldb-create-");
        Path path = root.resolve("новая");
        byte[] key = {0, -1}, value = {-128, 0};
        try (OpenOptions options = new OpenOptions().createIfMissing(false).errorIfExists(false).cacheSize(0)) {
            try (Database db = Database.create(path, options); Cursor cursor = db.iterator()) {
                cursor.seekToFirst();
                assert !cursor.isValid();
                db.put(key, value);
                try (Database unexpected = Database.create(path)) {
                    throw new AssertionError("created a locked database");
                } catch (LockedException expected) {
                }
            }
            try (Database unexpected = Database.create(path, options)) {
                throw new AssertionError("created an existing database");
            } catch (InvalidArgumentException expected) {
            }
            try (Database db = Database.open(path, options)) {
                assert Arrays.equals(db.get(key), value);
            }
            try (Database unexpected = Database.open(root.resolve("absent"), options)) {
                throw new AssertionError("changed caller options");
            } catch (InvalidArgumentException expected) {
            }
            try (Database db = Database.create(root.resolve("defaults"))) {
                assert db.get(key) == null;
            }
        }
    }
}
