using System;

namespace MultiLevelDB
{
    /// <summary>Immutable read view retaining its engine. Dispose after use; existing cursors remain valid.</summary>
    public sealed class Snapshot : Resource
    { internal Snapshot(IntPtr pointer) : base(pointer, Native.ml_snapshot_release) { } }
}
