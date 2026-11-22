namespace MultiLevelDB
{
    /// <summary>Stable native status identifiers. Missing Get alone returns null.</summary>
    public enum ErrorCode
    {
        /// <summary>Native Ok status (0).</summary>
        Ok = 0,
        /// <summary>Native NotFound status (1).</summary>
        NotFound = 1,
        /// <summary>Native InvalidArgument status (2).</summary>
        InvalidArgument = 2,
        /// <summary>Native IoError status (3).</summary>
        IoError = 3,
        /// <summary>Native Corruption status (4).</summary>
        Corruption = 4,
        /// <summary>Native DatabaseLocked status (5).</summary>
        DatabaseLocked = 5,
        /// <summary>Native DatabaseClosed status (6).</summary>
        DatabaseClosed = 6,
        /// <summary>Native Unsupported status (7).</summary>
        Unsupported = 7,
        /// <summary>Native InternalError status (8).</summary>
        InternalError = 8,
    }
}
