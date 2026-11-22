"""Exercise the real native engine, including managed ownership and ABI errors."""
import gc
import os
import tempfile
import threading
import unittest
from multileveldb import Batch, Database, Error
from pathlib import Path


class DatabaseTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="python-данные-数据库-", dir=os.environ.get("ML_TEST_ROOT"))
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "db"

    def test_new_database_preserves_options_and_data(self):
        path = self.path.with_name("новая")
        options = {"create_if_missing": False, "error_if_exists": False, "cache_size": 0}
        with Database.create(path, **options) as db:
            with db.iterator() as cursor:
                cursor.seek_to_first()
                self.assertFalse(cursor.is_valid())
            db.put(b"\0\xff", b"\x80\0")
            with self.assertRaises(Error) as locked:
                Database.create(path)
            self.assertEqual(locked.exception.code, "DATABASE_LOCKED")
        with self.assertRaises(Error) as existing:
            Database.create(path, **options)
        self.assertEqual(existing.exception.code, "INVALID_ARGUMENT")
        with Database.open(path, **options) as db:
            self.assertEqual(db.get(b"\0\xff"), b"\x80\0")
        with self.assertRaises(Error) as missing:
            Database.open(path.with_name("absent"), **options)
        self.assertEqual(missing.exception.code, "INVALID_ARGUMENT")
        with Database.create(path.with_name("defaults")) as db:
            self.assertIsNone(db.get(b"absent"))

    def test_create_rejects_invalid_path(self):
        with self.assertRaises(Error) as empty:
            Database.create("")
        self.assertEqual(empty.exception.code, "INVALID_ARGUMENT")
        with self.assertRaises(TypeError):
            Database.create(b"bytes-path")
        with self.assertRaises(UnicodeEncodeError):
            Database.create("\ud800")

    def test_binary_options_batch_snapshot_cursor_and_reopen(self):
        with Database.create(self.path, create_if_missing=False, error_if_exists=False,
                             paranoid_checks=True, write_buffer_size=65536,
                             max_open_files=64, block_size=1024, block_restart_interval=8,
                             compression="snappy", cache_size=0) as db:
            self.assertIsNone(db.get(b"missing"))
            db.put(b"", b"")
            self.assertEqual(db.get(b""), b"")
            key, value = b"\0\xff\x80", bytes(range(256)) * 4096
            db.put(bytearray(key), memoryview(value), {"sync": True})
            self.assertEqual(db.get(key, {"verify_checksums": True, "fill_cache": False}), value)
            snapshot = db.create_snapshot()
            self.addCleanup(snapshot.close)
            with Batch() as batch:
                batch.put(key, b"new").put(b"z", b"last").delete(b"")
                db.write(batch, {"sync": True})
                self.assertEqual(db.get(key), b"new")
                self.assertEqual(db.get(key, snapshot=snapshot), value)
                batch.clear()
                db.write(batch)
                batch.close()
                with self.assertRaises(Error) as raised:
                    db.write(batch)
                self.assertEqual(raised.exception.code, "DATABASE_CLOSED")
            with db.iterator(snapshot=snapshot) as cursor:
                self.assertFalse(cursor.is_valid())
                with self.assertRaises(Error) as raised:
                    cursor.key()
                self.assertEqual(raised.exception.code, "NOT_FOUND")
                snapshot.close()
                cursor.seek_to_first()
                self.assertEqual(cursor.key(), b"")
                cursor.next()
                self.assertEqual(cursor.key(), key)
                copied = cursor.value()
                cursor.seek_to_last()
                cursor.previous()
                self.assertEqual(cursor.key(), b"")
                cursor.seek(key)
                self.assertEqual(cursor.value(), value)
                cursor.next()
                self.assertFalse(cursor.is_valid())
                cursor.status()
                self.assertEqual(copied, value)
        with Database.open(self.path) as db:
            self.assertEqual(db.get(key), b"new")
            db.delete(key)
            db.delete(key)
            self.assertIsNone(db.get(key))
        with self.assertRaises(Error) as raised:
            Database.create(self.path)
        self.assertEqual(raised.exception.code, "INVALID_ARGUMENT")

    def test_foreign_closed_handles_and_retained_engine(self):
        db = Database.create(self.path)
        self.addCleanup(db.close)
        db.put(b"a", b"before")
        with Database.create(Path(self.directory.name) / "other") as other, other.create_snapshot() as foreign:
            with self.assertRaises(Error) as raised:
                db.get(b"a", snapshot=foreign)
            self.assertEqual(raised.exception.code, "INVALID_ARGUMENT")
        cursor = db.iterator()
        self.addCleanup(cursor.close)
        db.close()
        db.close()
        with self.assertRaises(Error) as raised:
            db.get(b"a")
        self.assertEqual(raised.exception.code, "DATABASE_CLOSED")
        cursor.seek_to_first()
        self.assertEqual(cursor.value(), b"before")
        with self.assertRaises(Error):
            Database.open(self.path)
        cursor.close()
        with Database.open(self.path):
            pass

    def test_validation_and_context_cleanup(self):
        with self.assertRaises(Error):
            Database.open(self.path)
        for path in ("", "bad\0path"):
            with self.assertRaises(Error):
                Database.create(path)
        with Database.create(self.path) as db:
            for value in ("text", 1, None):
                with self.assertRaises(TypeError):
                    db.put(value, b"x")
            for opts in ({"sync": 2}, {"create_if_missing": 1}, {"sync": -1}, {"sync": 1 << 64}):
                with self.assertRaises((Error, ValueError)):
                    db.put(b"a", b"b", opts)
            with self.assertRaises(TypeError):
                db.get(b"a", snapshot=db)

        def abandon():
            return Database.open(self.path)

        abandoned = abandon()
        del abandoned
        gc.collect()
        with Database.open(self.path):
            pass

    def test_shared_threads_and_close(self):
        with Database.create(self.path) as db:
            failures = []

            def writer(index):
                try:
                    for n in range(100):
                        db.put(bytes([index, n]), b"\xff")
                except BaseException as error:
                    failures.append(error)

            threads = [threading.Thread(target=writer, args=(i,)) for i in range(4)]
            for thread in threads:
                thread.start()
            for thread in threads:
                thread.join()
            self.assertEqual(failures, [])
            self.assertEqual(db.get(b"\x03\x63"), b"\xff")


if __name__ == "__main__":
    unittest.main()
