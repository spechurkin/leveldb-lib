from multileveldb import Database

with Database.open("example-db", create_if_missing=True) as db:
    db.put(b"\0\xff", b"\0\x80\xff")
    assert db.get(b"\0\xff") == b"\0\x80\xff"
    with db.iterator() as cursor:
        cursor.seek_to_first()
        while cursor.is_valid():
            print(cursor.key().hex(), cursor.value().hex())
            cursor.next()
        cursor.status()
