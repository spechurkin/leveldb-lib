using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using MultiLevelDB;

static class Program
{
    static void Assert(bool condition) { if (!condition) throw new Exception("Assertion failed"); }
    static void Equal(byte[]? actual, byte[] expected) { Assert(actual != null && actual.SequenceEqual(expected)); }
    static byte[] Decode(string text)
    {
        if (text == "-") return Array.Empty<byte>();
        return Enumerable.Range(0, text.Length / 2).Select(i => Convert.ToByte(text.Substring(i * 2, 2), 16)).ToArray();
    }
    static void Fails(Action action, ErrorCode code)
    {
        try { action(); }
        catch (DatabaseException error) { Assert(error.Code == code); return; }
        throw new Exception($"Expected {code}");
    }

    static void Main(string[] args)
    {
        if (args.Contains("--interop")) { Interop(); return; }
        string directory = Path.Combine(Environment.GetEnvironmentVariable("ML_TEST_ROOT") ?? Path.GetTempPath(), "dotnet-данные-数据库-😀-" + Guid.NewGuid());
        Directory.CreateDirectory(directory);
        string path = Path.Combine(directory, "db");
        byte[] key = {0, 255, 128};
        byte[] value = Enumerable.Range(0, 1048576).Select(i => (byte)i).ToArray();
        try
        {
            using (var options = new OpenOptions().CreateIfMissing(false).ErrorIfExists(false).ParanoidChecks(true)
                       .WriteBufferSize(65536).MaxOpenFiles(64).BlockSize(1024).BlockRestartInterval(8)
                       .Compression(Compression.Snappy).CacheSize(0))
            using (var reads = new ReadOptions().VerifyChecksums(true).FillCache(false))
            using (var writes = new WriteOptions().Sync(true))
            using (var db = Database.Create(path, options))
            {
                Assert(db.Get(new byte[] {1}) == null);
                db.Put(Array.Empty<byte>(), Array.Empty<byte>());
                Equal(db.Get(Array.Empty<byte>()), Array.Empty<byte>());
                db.Put(key, value, writes);
                Equal(db.Get(key, reads), value);
                using var snapshot = db.CreateSnapshot();
                using var batch = new Batch();
                batch.Put(key, new byte[] {42}).Put(new byte[] {127}, new byte[] {1}).Delete(Array.Empty<byte>());
                db.Write(batch, writes);
                Equal(db.Get(key), new byte[] {42});
                Equal(db.Get(key, null, snapshot), value);
                batch.Clear(); db.Write(batch);
                batch.Dispose(); Fails(() => db.Write(batch), ErrorCode.DatabaseClosed);
                using var cursor = db.Iterator(reads, snapshot);
                Assert(!cursor.IsValid);
                Fails(() => cursor.Key(), ErrorCode.NotFound);
                snapshot.Dispose();
                cursor.SeekToFirst(); Equal(cursor.Key(), Array.Empty<byte>());
                cursor.Next(); Equal(cursor.Key(), key);
                var copied = cursor.Value();
                cursor.SeekToLast(); cursor.Previous(); Equal(cursor.Key(), Array.Empty<byte>());
                cursor.Seek(key); Equal(cursor.Value(), value);
                cursor.Next(); Assert(!cursor.IsValid); cursor.Status(); Equal(copied, value);
                using var other = Database.Create(Path.Combine(directory, "other"));
                using var foreign = other.CreateSnapshot();
                Fails(() => db.Get(key, null, foreign), ErrorCode.InvalidArgument);
                Parallel.For(0, 4, i => { for (int n = 0; n < 50; n++) db.Put(new byte[] {100, (byte)i, (byte)n}, key); });
                db.Dispose(); db.Dispose();
                Fails(() => db.Get(key), ErrorCode.DatabaseClosed);
                cursor.SeekToFirst(); Assert(cursor.IsValid);
            }
            using (var db = Database.Open(path))
            {
                Equal(db.Get(key), new byte[] {42});
                db.Delete(key); db.Delete(key); Assert(db.Get(key) == null);
            }
            Fails(() => Database.Create(path).Dispose(), ErrorCode.InvalidArgument);
            Fails(() => Database.Create("bad\0path").Dispose(), ErrorCode.InvalidArgument);
            using var wrong = new WriteOptions();
            wrong.Dispose();
            Fails(() => wrong.Sync(false), ErrorCode.DatabaseClosed);
            Console.WriteLine("C#: binary CRUD/options/batches/snapshots/cursors/ownership/create passed");
        }
        finally { Directory.Delete(directory, true); }
    }

    static void Interop()
    {
        using var db = Database.Open(Environment.GetEnvironmentVariable("ML_INTEROP_PATH")!);
        foreach (string line in File.ReadLines(Environment.GetEnvironmentVariable("ML_FIXTURE")!))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            var fields = line.Split('\t');
            byte[] key = Decode(fields[0]);
            byte[] value = Decode(fields[1]);
            Equal(db.Get(key), value.Concat(new byte[] {0, 254}).ToArray());
        }
        Equal(db.Get(new byte[] {114,117,98,121,0,255}), new byte[] {0,128,255});
        db.Put(new byte[] {99,115,104,97,114,112,0,255}, new byte[] {0,128,255});
    }
}
