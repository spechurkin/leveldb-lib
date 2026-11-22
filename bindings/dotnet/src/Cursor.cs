using System;

namespace MultiLevelDB
{
    /// <summary>Owned bidirectional streaming cursor. Check Status after scanning.</summary>
    public sealed class Cursor : Resource
    {
        internal Cursor(IntPtr pointer) : base(pointer, Native.ml_iterator_destroy) { }
        /// <summary>Position at the first record; an empty view becomes invalid.</summary>
        public void SeekToFirst()
        { lock (Native.Gate) Native.Check(Native.ml_iterator_seek_first(Handle, out var error), error); }
        /// <summary>Position at the last record; an empty view becomes invalid.</summary>
        public void SeekToLast()
        { lock (Native.Gate) Native.Check(Native.ml_iterator_seek_last(Handle, out var error), error); }
        /// <summary>Advance. Passing the end succeeds; already invalid throws NotFound.</summary>
        public void Next()
        { lock (Native.Gate) Native.Check(Native.ml_iterator_next(Handle, out var error), error); }
        /// <summary>Move backward; already invalid throws NotFound.</summary>
        public void Previous()
        { lock (Native.Gate) Native.Check(Native.ml_iterator_previous(Handle, out var error), error); }
        /// <summary>Throw for a recorded engine error; ordinary exhaustion succeeds.</summary>
        public void Status()
        { lock (Native.Gate) Native.Check(Native.ml_iterator_status(Handle, out var error), error); }

        /// <summary>Seek to the first key greater than or equal to the target; no match becomes invalid.</summary>
        public void Seek(byte[] key)
        {
            var size = Native.Size(key);
            lock (Native.Gate)
                Native.Check(Native.ml_iterator_seek(Handle, key, size, out var error), error);
        }

        /// <summary>Test position without checking the engine status; closed wrappers throw.</summary>
        public bool IsValid { get { lock (Native.Gate) return Native.ml_iterator_valid(Handle) != 0; } }
        /// <summary>Copy the current binary key. Invalid position throws NotFound.</summary>
        public byte[] Key()
        {
            lock (Native.Gate)
            {
                Native.Check(Native.ml_iterator_key(Handle, out var pointer, out var length, out var error), error);
                return Native.Copy(pointer, length);
            }
        }
        /// <summary>Copy the current binary value. Invalid position throws NotFound.</summary>
        public byte[] Value()
        {
            lock (Native.Gate)
            {
                Native.Check(Native.ml_iterator_value(Handle, out var pointer, out var length, out var error), error);
                return Native.Copy(pointer, length);
            }
        }
    }
}
