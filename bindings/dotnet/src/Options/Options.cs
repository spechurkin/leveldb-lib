using System;

namespace MultiLevelDB
{
    /// <summary>Kind-specific native options. Setters copy settings and return their builder.</summary>
    public abstract class Options : Resource
    {
        internal Options(uint kind) : base(Create(kind), Native.ml_options_destroy) { }
        private static IntPtr Create(uint kind)
        {
            lock (Native.Gate)
            {
                Native.Check(Native.ml_options_create(kind, out var pointer, out var error), error);
                return pointer;
            }
        }
        internal void Set(uint field, ulong value)
        { lock (Native.Gate) Native.Check(Native.ml_options_set(Handle, field, value, out var error), error); }
    }
}
