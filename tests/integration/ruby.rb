require 'multileveldb'

MultiLevelDB::Database.open(ENV.fetch('ML_INTEROP_PATH')) do |db|
  File.foreach(ENV.fetch('ML_FIXTURE')) do |line|
    next if line.start_with?('#') || line.strip.empty?

    key, value = line.strip.split("\t").map { |field| field == '-' ? ''.b : [field].pack('H*') }
    raise 'Ruby fixture mismatch' unless db.get(key) == value + "\0\xfe".b
  end
  raise 'Python/Ruby mismatch' unless db.get("python\0\xff".b) == "\0\x80\xff".b
  db.put("ruby\0\xff".b, "\0\x80\xff".b)
end
