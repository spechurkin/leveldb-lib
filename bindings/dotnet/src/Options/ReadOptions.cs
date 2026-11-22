using System;

namespace MultiLevelDB
{
    /// <summary>Settings for ReadOptions; operations copy them synchronously.</summary>
    public sealed class ReadOptions : Options
    {
        /// <summary>Allocate settings initialized to engine defaults.</summary>
        public ReadOptions() : base(1) { }
        /// <summary>Set VerifyChecksums. Default: false. Native range failures throw DatabaseException.</summary>
        public ReadOptions VerifyChecksums(bool value) { Set(10, value ? 1UL : 0UL); return this; }
        /// <summary>Set FillCache. Default: true; false still permits existing cached reads. Native range failures throw DatabaseException.</summary>
        public ReadOptions FillCache(bool value) { Set(11, value ? 1UL : 0UL); return this; }
    }
}
