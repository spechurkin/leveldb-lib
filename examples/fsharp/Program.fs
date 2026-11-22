open System
open MultiLevelDB

[<EntryPoint>]
let main _ =
    use options = new OpenOptions()
    options.CreateIfMissing(true) |> ignore
    use db = Database.Open("example-db", options)
    let key, value = [|0uy;255uy|], [|0uy;128uy;255uy|]
    db.Put(key, value)
    if db.Get key <> value then failwith "Roundtrip failed"
    use cursor = db.Iterator()
    cursor.SeekToFirst()
    while cursor.IsValid do
        printfn "%s %s" (Convert.ToHexString(cursor.Key())) (Convert.ToHexString(cursor.Value()))
        cursor.Next()
    cursor.Status()
    0
