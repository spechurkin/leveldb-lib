using System;
using System.Runtime.InteropServices;

namespace MultiLevelDB
{
    internal sealed class OwnedHandle : SafeHandle
    {
        private readonly Action<IntPtr> release;
        internal OwnedHandle(IntPtr pointer, Action<IntPtr> release) : base(IntPtr.Zero, true)
        { this.release = release; SetHandle(pointer); }
        public override bool IsInvalid => handle == IntPtr.Zero;
        protected override bool ReleaseHandle()
        { lock (Native.Gate) { release(handle); return true; } }
    }
}
