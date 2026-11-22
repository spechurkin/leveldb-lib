require 'multileveldb'

MultiLevelDB::Database.open('example-db', create_if_missing: true) do |db|
  db.put("\0\xff".b, "\0\x80\xff".b)
  raise 'roundtrip failed' unless db.get("\0\xff".b) == "\0\x80\xff".b
  db.iterator.use do |cursor|
    cursor.seek_to_first
    while cursor.valid?
      puts "#{cursor.key.unpack1('H*')} #{cursor.value.unpack1('H*')}"
      cursor.next
    end
    cursor.status
  end
end
