using System;

namespace MultiLevelDB
{
    /// <summary>Deterministic ownership with SafeHandle fallback; use using or F# use.</summary>
    public abstract class Resource : IDisposable
    {
        private readonly OwnedHandle owned;
        internal Resource(IntPtr pointer, Action<IntPtr> release) { owned = new OwnedHandle(pointer, release); }
        internal IntPtr Handle
        {
            get
            {
                if (owned.IsClosed) throw new DatabaseException(ErrorCode.DatabaseClosed, "Resource is closed");
                return owned.DangerousGetHandle();
            }
        }
        internal static IntPtr Optional(Resource? resource) => resource?.Handle ?? IntPtr.Zero;

        /// <summary>Release once. Dependents keep their own engine/snapshot references.</summary>
        public void Dispose() { lock (Native.Gate) owned.Dispose(); }
    }
}
