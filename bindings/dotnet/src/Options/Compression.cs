namespace MultiLevelDB
{
    /// <summary>Compression for newly written blocks.</summary>
    public enum Compression
    {
        /// <summary>Write uncompressed blocks.</summary>
        None = 0,
        /// <summary>Use the bundled Snappy codec (default).</summary>
        Snappy = 1
    }
}
