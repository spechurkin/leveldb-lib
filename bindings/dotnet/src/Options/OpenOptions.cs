using System;

namespace MultiLevelDB
{
    /// <summary>Settings for OpenOptions; operations copy them synchronously.</summary>
    public sealed class OpenOptions : Options
    {
        /// <summary>Allocate settings initialized to engine defaults.</summary>
        public OpenOptions() : base(0) { }
        /// <summary>Set CreateIfMissing. Default: false; parents must exist. Native range failures throw DatabaseException.</summary>
        public OpenOptions CreateIfMissing(bool value) { Set(1, value ? 1UL : 0UL); return this; }
        /// <summary>Set ErrorIfExists. Default: false. Native range failures throw DatabaseException.</summary>
        public OpenOptions ErrorIfExists(bool value) { Set(2, value ? 1UL : 0UL); return this; }
        /// <summary>Set ParanoidChecks. Default: false; extra recovery checks. Native range failures throw DatabaseException.</summary>
        public OpenOptions ParanoidChecks(bool value) { Set(3, value ? 1UL : 0UL); return this; }
        /// <summary>Set WriteBufferSize. Default: 4 MiB; positive byte target, engine may clamp. Native range failures throw DatabaseException.</summary>
        public OpenOptions WriteBufferSize(ulong value) { Set(4, value); return this; }
        /// <summary>Set MaxOpenFiles. Default: 1000; positive C-int file budget, engine may clamp. Native range failures throw DatabaseException.</summary>
        public OpenOptions MaxOpenFiles(ulong value) { Set(5, value); return this; }
        /// <summary>Set BlockSize. Default: 4096; positive uncompressed block byte target. Native range failures throw DatabaseException.</summary>
        public OpenOptions BlockSize(ulong value) { Set(6, value); return this; }
        /// <summary>Set BlockRestartInterval. Default: 16; positive C-int key count. Native range failures throw DatabaseException.</summary>
        public OpenOptions BlockRestartInterval(ulong value) { Set(7, value); return this; }
        /// <summary>Set Compression. Default: Snappy; affects newly written blocks. Native range failures throw DatabaseException.</summary>
        public OpenOptions Compression(Compression value) { Set(8, (ulong)value); return this; }
        /// <summary>Set CacheSize. Default: engine cache policy; zero explicitly selects zero capacity. Native range failures throw DatabaseException.</summary>
        public OpenOptions CacheSize(ulong value) { Set(9, value); return this; }
    }
}
