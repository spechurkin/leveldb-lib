open System
open System.IO
open MultiLevelDB

let check condition = if not condition then failwith "F# assertion failed"
let bytes (value: byte array) = Option.ofObj value

[<EntryPoint>]
let main args =
    if Array.contains "--interop" args then
        use db = Database.Open(Environment.GetEnvironmentVariable "ML_INTEROP_PATH")
        for line in File.ReadLines(Environment.GetEnvironmentVariable "ML_FIXTURE") do
            if line <> "" && not (line.StartsWith "#") then
                let decode (field: string) =
                    if field = "-" then [||]
                    else Array.init (field.Length / 2) (fun i -> Convert.ToByte(field.Substring(i * 2, 2), 16))
                let fields = line.Split '\t'
                check (db.Get(decode fields[0]) = Array.append (decode fields[1]) [|0uy;254uy|])
        check (db.Get([|99uy;115uy;104uy;97uy;114uy;112uy;0uy;255uy|]) = [|0uy;128uy;255uy|])
        db.Put([|102uy;115uy;104uy;97uy;114uy;112uy;0uy;255uy|], [|0uy;128uy;255uy|])
    else
        let parent = Environment.GetEnvironmentVariable "ML_TEST_ROOT" |> Option.ofObj |> Option.defaultValue (Path.GetTempPath())
        let directory = Path.Combine(parent, "fsharp-данные-数据库-" + Guid.NewGuid().ToString())
        Directory.CreateDirectory directory |> ignore
        let path = Path.Combine(directory, "db")
        try
            do
                use options = new OpenOptions()
                options.CreateIfMissing(false).ErrorIfExists(false) |> ignore
                use db = Database.Create(path, options)
                let key, value = [|0uy;255uy|], [|0uy;128uy;255uy|]
                check (bytes (db.Get key) = None)
                db.Put(key, [||])
                check (bytes (db.Get key) = Some [||])
                use snapshot = db.CreateSnapshot()
                use batch = new Batch()
                batch.Put(key, value).Delete([|1uy|]) |> ignore
                use writes = new WriteOptions()
                writes.Sync true |> ignore
                db.Write(batch, writes)
                batch.Clear()
                check (db.Get key = value)
                check (db.Get(key, null, snapshot) = [||])
                use reads = new ReadOptions()
                reads.VerifyChecksums(true).FillCache(false) |> ignore
                use cursor = db.Iterator(reads, snapshot)
                snapshot.Dispose()
                db.Dispose()
                cursor.SeekToFirst()
                check (cursor.IsValid && cursor.Key() = key && cursor.Value() = [||])
                cursor.SeekToLast()
                cursor.Seek key
                cursor.Next()
                check (not cursor.IsValid)
                cursor.Status()
            do
                use db = Database.Open path
                check (db.Get [|0uy;255uy|] = [|0uy;128uy;255uy|])
                db.Delete [|0uy;255uy|]
                db.Delete [|0uy;255uy|]
                check (bytes (db.Get [|0uy;255uy|]) = None)
            let rejected =
                try
                    use db = Database.Create path
                    false
                with :? DatabaseException as error -> error.Code = ErrorCode.InvalidArgument
            check rejected
            printfn "F#: binary CRUD/batch/snapshot/cursor/ownership/create passed"
        finally Directory.Delete(directory, true)
    0
