using System;

namespace MultiLevelDB
{
    /// <summary>Native failure retaining its status and original engine diagnostic.</summary>
    public sealed class DatabaseException : Exception
    {
        /// <summary>Stable native failure code.</summary>
        public ErrorCode Code { get; }
        internal DatabaseException(ErrorCode code, string message) : base(message) { Code = code; }
    }
}
