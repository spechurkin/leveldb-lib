using System;
using System.Text;

namespace MultiLevelDB
{
    /// <summary>A binary LevelDB directory handle. All calls are synchronous and serialized.</summary>
    public sealed class Database : Resource
    {
        private Database(IntPtr pointer) : base(pointer, Native.ml_db_destroy) { }

        /// <summary>Create a new LevelDB, failing if it exists; settings are copied without mutation.</summary>
        public static Database Create(string path, OpenOptions? options = null)
        {
            if (path == null) throw new ArgumentNullException(nameof(path));
            var bytes = new UTF8Encoding(false, true).GetBytes(path);
            lock (Native.Gate)
            {
                Native.Check(Native.ml_db_create(bytes, Native.Size(bytes), Optional(options), out var pointer, out var error), error);
                return new Database(pointer);
            }
        }

        /// <summary>Open a strict UTF-8 path; creation is disabled by default. Parents must exist.</summary>
        public static Database Open(string path, OpenOptions? options = null)
        {
            if (path == null) throw new ArgumentNullException(nameof(path));
            var bytes = new UTF8Encoding(false, true).GetBytes(path);
            lock (Native.Gate)
            {
                Native.Check(Native.ml_db_open(bytes, Native.Size(bytes), Optional(options), out var pointer, out var error), error);
                return new Database(pointer);
            }
        }

        /// <summary>Return a copied byte array, or null if missing. Empty values have length zero.</summary>
        public byte[]? Get(byte[] key, ReadOptions? options = null, Snapshot? snapshot = null)
        {
            var size = Native.Size(key);
            lock (Native.Gate)
            {
                var status = Native.ml_db_get(Handle, key, size, Optional(options), Optional(snapshot), out var pointer, out var length, out var error);
                if (status == 1) { Native.ml_error_free(error); return null; }
                Native.Check(status, error);
                return Native.Copy(pointer, length);
            }
        }

        /// <summary>Insert/replace exact bytes. Default writes do not request synchronous flush.</summary>
        public void Put(byte[] key, byte[] value, WriteOptions? options = null)
        {
            var keySize = Native.Size(key); var valueSize = Native.Size(value);
            lock (Native.Gate)
                Native.Check(Native.ml_db_put(Handle, key, keySize, value, valueSize, Optional(options), out var error), error);
        }

        /// <summary>Delete a binary key; absent keys also succeed.</summary>
        public void Delete(byte[] key, WriteOptions? options = null)
        {
            var size = Native.Size(key);
            lock (Native.Gate)
                Native.Check(Native.ml_db_delete(Handle, key, size, Optional(options), out var error), error);
        }

        /// <summary>Atomically apply the live batch in order, without consuming or clearing it.</summary>
        public void Write(Batch batch, WriteOptions? options = null)
        {
            if (batch == null) throw new ArgumentNullException(nameof(batch));
            lock (Native.Gate)
                Native.Check(Native.ml_batch_write(Handle, batch.Handle, Optional(options), out var error), error);
        }

        /// <summary>Capture a native read view belonging to this exact DB instance.</summary>
        public Snapshot CreateSnapshot()
        {
            lock (Native.Gate)
            {
                Native.Check(Native.ml_snapshot_create(Handle, out var pointer, out var error), error);
                return new Snapshot(pointer);
            }
        }

        /// <summary>Create an unpositioned cursor retaining its engine and optional same-instance snapshot.</summary>
        public Cursor Iterator(ReadOptions? options = null, Snapshot? snapshot = null)
        {
            lock (Native.Gate)
            {
                Native.Check(Native.ml_iterator_create(Handle, Optional(options), Optional(snapshot), out var pointer, out var error), error);
                return new Cursor(pointer);
            }
        }
    }
}
