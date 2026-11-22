using System;
using System.Runtime.InteropServices;

namespace MultiLevelDB
{
    internal static class Native
    {
        internal static readonly object Gate = new object();
        private const string Library = "multileveldb";

        static Native()
        {
#if NETFRAMEWORK
            string? file = Environment.GetEnvironmentVariable("MULTILEVELDB_LIBRARY");
            if (file != null && LoadLibraryExW(System.IO.Path.GetFullPath(file), IntPtr.Zero, 8) == IntPtr.Zero)
                throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error(), "Cannot load MultiLevelDB");
            // Keep the module loaded for the process lifetime, as DllImport does.
#else
            NativeLibrary.SetDllImportResolver(typeof(Native).Assembly, (name, assembly, path) =>
                name == Library && Environment.GetEnvironmentVariable("MULTILEVELDB_LIBRARY") is string file
                    ? NativeLibrary.Load(file) : IntPtr.Zero);
#endif
            if (ml_abi_version() >> 16 != 1)
                throw new NotSupportedException("Incompatible MultiLevelDB ABI");
        }

        internal static void Check(int status, IntPtr error)
        {
            try
            {
                if (status != 0)
                    throw new DatabaseException((ErrorCode)status,
                        ErrorMessage(ml_error_message(error)));
            }
            finally { ml_error_free(error); }
        }

        private static string ErrorMessage(IntPtr pointer)
        {
            if (pointer == IntPtr.Zero) return "";
#if NETFRAMEWORK
            int length = 0;
            while (Marshal.ReadByte(pointer, length) != 0) length = checked(length + 1);
            var bytes = new byte[length];
            Marshal.Copy(pointer, bytes, 0, length);
            return System.Text.Encoding.UTF8.GetString(bytes);
#else
            return Marshal.PtrToStringUTF8(pointer) ?? "";
#endif
        }

#if NETFRAMEWORK
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true, ExactSpelling = true)]
        private static extern IntPtr LoadLibraryExW(string file, IntPtr reserved, uint flags);
#endif

        internal static byte[] Copy(IntPtr pointer, UIntPtr length)
        {
            try
            {
                var bytes = new byte[checked((int)length.ToUInt64())];
                Marshal.Copy(pointer, bytes, 0, bytes.Length);
                return bytes;
            }
            finally { ml_buffer_free(pointer); }
        }

        internal static UIntPtr Size(byte[] bytes)
        {
            if (bytes == null) throw new ArgumentNullException(nameof(bytes));
            return (UIntPtr)(uint)bytes.Length;
        }
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint ml_abi_version();
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ml_library_version();
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ml_api_version();
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ml_leveldb_version();
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint ml_has_feature(uint feature);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_error_code(IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ml_error_message(IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_error_free(IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_buffer_free(IntPtr buffer);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_options_create(uint kind, out IntPtr @out, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_options_set(IntPtr options, uint field, ulong value, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_options_destroy(IntPtr options);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_db_open([In] byte[] path, UIntPtr length, IntPtr options, out IntPtr @out, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_db_create([In] byte[] path, UIntPtr length, IntPtr options, out IntPtr @out, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_db_close(IntPtr db, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_db_destroy(IntPtr db);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_db_get(IntPtr db, [In] byte[] key, UIntPtr key_len, IntPtr read_options, IntPtr snapshot, out IntPtr value, out UIntPtr value_len, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_db_put(IntPtr db, [In] byte[] key, UIntPtr key_len, [In] byte[] value, UIntPtr value_len, IntPtr write_options, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_db_delete(IntPtr db, [In] byte[] key, UIntPtr key_len, IntPtr write_options, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_create(IntPtr db, IntPtr read_options, IntPtr snapshot, out IntPtr @out, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_iterator_destroy(IntPtr iterator);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_seek_first(IntPtr iterator, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_seek_last(IntPtr iterator, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_seek(IntPtr iterator, [In] byte[] key, UIntPtr key_len, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_next(IntPtr iterator, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_previous(IntPtr iterator, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint ml_iterator_valid(IntPtr iterator);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_key(IntPtr iterator, out IntPtr @out, out UIntPtr length, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_value(IntPtr iterator, out IntPtr @out, out UIntPtr length, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_iterator_status(IntPtr iterator, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_batch_create(out IntPtr @out, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_batch_destroy(IntPtr batch);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_batch_clear(IntPtr batch, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_batch_put(IntPtr batch, [In] byte[] key, UIntPtr key_len, [In] byte[] value, UIntPtr value_len, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_batch_delete(IntPtr batch, [In] byte[] key, UIntPtr key_len, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_batch_write(IntPtr db, IntPtr batch, IntPtr write_options, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ml_snapshot_create(IntPtr db, out IntPtr @out, out IntPtr error);
        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ml_snapshot_release(IntPtr snapshot);
    }
}
