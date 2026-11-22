using System;
using System.Linq;
using MultiLevelDB;

using var options = new OpenOptions().CreateIfMissing(true);
using var db = Database.Open("example-db", options);
byte[] key = {0, 255}, value = {0, 128, 255};
db.Put(key, value);
if (!(db.Get(key)?.SequenceEqual(value) ?? false)) throw new Exception("Roundtrip failed");
using var cursor = db.Iterator();
cursor.SeekToFirst();
while (cursor.IsValid)
{
    Console.WriteLine($"{Convert.ToHexString(cursor.Key())} {Convert.ToHexString(cursor.Value())}");
    cursor.Next();
}
cursor.Status();
