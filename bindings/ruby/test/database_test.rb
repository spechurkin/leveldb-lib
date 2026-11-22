# frozen_string_literal: true
require 'tmpdir'
require 'fileutils'
require 'multileveldb'

def assert(value)
  raise 'assertion failed' unless value
end

def expect_error(code)
  begin
    yield
  rescue MultiLevelDB::Error => error
    assert(error.code == code)
    return
  end
  raise "expected #{code}"
end

Dir.mktmpdir('ruby-данные-数据库-', ENV['ML_TEST_ROOT']) do |directory|
  path = File.join(directory, 'db')
  key, value = "\0\xff\x80".b, (0..255).to_a.pack('C*') * 4096
  db = MultiLevelDB::Database.create(path, create_if_missing: false, error_if_exists: false,
                                     paranoid_checks: true, write_buffer_size: 65536, max_open_files: 64,
                                     block_size: 1024, block_restart_interval: 8, compression: :snappy, cache_size: 0)
  db.use do
    assert(db.get('missing').nil?)
    db.put('', '')
    assert(db.get('') == ''.b)
    db.put(key, value, sync: true)
    assert(db.get(key, read_options: { verify_checksums: true, fill_cache: false }) == value)
    snapshot = db.create_snapshot
    snapshot.use do
      MultiLevelDB::Batch.new.use do |batch|
        batch.put(key, 'new').put('z', 'last').delete('')
        db.write(batch, sync: true)
        batch.clear
        db.write(batch)
        assert(db.get(key) == 'new')
        assert(db.get(key, snapshot: snapshot) == value)
        batch.close
        expect_error('DATABASE_CLOSED') { db.write(batch) }
      end
      db.iterator(snapshot: snapshot).use do |cursor|
        assert(!cursor.valid?)
        expect_error('NOT_FOUND') { cursor.key }
        snapshot.close
        cursor.seek_to_first
        assert(cursor.key == '')
        cursor.next
        assert(cursor.key == key)
        copied = cursor.value
        cursor.seek_to_last
        cursor.previous
        assert(cursor.key == '')
        cursor.seek(key)
        assert(cursor.value == value)
        cursor.next
        assert(!cursor.valid?)
        cursor.status
        assert(copied == value)
      end
    end
    MultiLevelDB::Database.create(File.join(directory, 'other')) do |other|
      other.create_snapshot.use do |foreign|
        expect_error('INVALID_ARGUMENT') { db.get(key, snapshot: foreign) }
      end
    end
    cursor = db.iterator
    cursor.use do
      db.close
      db.close
      expect_error('DATABASE_CLOSED') { db.get(key) }
      cursor.seek_to_first
      assert(cursor.valid?)
      begin
        MultiLevelDB::Database.open(path).close
        raise 'retained cursor must keep directory locked'
      rescue MultiLevelDB::Error
        # The lock diagnostic may be classified as IO_ERROR by the engine.
      end
    end
  end
  expect_error('INVALID_ARGUMENT') { MultiLevelDB::Database.create(path) }
  MultiLevelDB::Database.open(path) do |reopened|
    assert(reopened.get(key) == 'new')
    reopened.delete(key)
    reopened.delete(key)
    assert(reopened.get(key).nil?)
  end
  expect_error('INVALID_ARGUMENT') { MultiLevelDB::Database.create("bad\0path") }
end
puts 'Ruby: binary CRUD/options/batches/snapshots/cursors/ownership/create passed'
