using System;

namespace MultiLevelDB
{
    /// <summary>Settings for WriteOptions; operations copy them synchronously.</summary>
    public sealed class WriteOptions : Options
    {
        /// <summary>Allocate settings initialized to engine defaults.</summary>
        public WriteOptions() : base(2) { }
        /// <summary>Set Sync. Default: false; true requests the engine's synchronous flush. Native range failures throw DatabaseException.</summary>
        public WriteOptions Sync(bool value) { Set(12, value ? 1UL : 0UL); return this; }
    }
}
