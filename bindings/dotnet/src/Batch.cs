using System;

namespace MultiLevelDB
{
    /// <summary>Reusable database-independent ordered write sequence.</summary>
    public sealed class Batch : Resource
    {
        /// <summary>Allocate an empty batch without writing a database.</summary>
        public Batch() : base(Create(), Native.ml_batch_destroy) { }
        private static IntPtr Create()
        {
            lock (Native.Gate)
            {
                Native.Check(Native.ml_batch_create(out var pointer, out var error), error);
                return pointer;
            }
        }

        /// <summary>Append copied binary data and return this batch for chaining.</summary>
        public Batch Put(byte[] key, byte[] value)
        {
            var keySize = Native.Size(key); var valueSize = Native.Size(value);
            lock (Native.Gate)
                Native.Check(Native.ml_batch_put(Handle, key, keySize, value, valueSize, out var error), error);
            return this;
        }

        /// <summary>Append a copied deletion and return this batch.</summary>
        public Batch Delete(byte[] key)
        {
            var size = Native.Size(key);
            lock (Native.Gate)
                Native.Check(Native.ml_batch_delete(Handle, key, size, out var error), error);
            return this;
        }

        /// <summary>Discard pending operations without changing any database.</summary>
        public void Clear()
        { lock (Native.Gate) Native.Check(Native.ml_batch_clear(Handle, out var error), error); }
    }
}
