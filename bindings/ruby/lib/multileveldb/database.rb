# frozen_string_literal: true

module MultiLevelDB
  # A database directory. Binary get returns a String or nil for missing keys.
  class Database < Resource
    # Create a new LevelDB, failing if it exists. Parents must exist. Storage
    # settings are copied; the native API forces both creation/existence flags.
    # With a block, closes automatically and returns the block's result.
    def self.create(path, **open_options)
      raise TypeError, 'Expected a path String' unless path.is_a?(String)

      path = path.encode(Encoding::UTF_8)
      raise ArgumentError, 'Invalid UTF-8 path' unless path.valid_encoding?

      db = LOCK.synchronize do
        Native.options(0, open_options) { |options| new(Native.create('db_create', path, path.bytesize, options), 'db_destroy') }
      end
      block_given? ? db.use { |resource| yield resource } : db
    end

    # Open a Unicode path. Parent directories must exist; creation is opt-in.
    # With a block, always closes and returns the block's result.
    def self.open(path, **open_options)
      raise TypeError, 'Expected a path String' unless path.is_a?(String)

      path = path.encode(Encoding::UTF_8)
      raise ArgumentError, 'Invalid UTF-8 path' unless path.valid_encoding?

      db = LOCK.synchronize do
        Native.options(0, open_options) { |options| new(Native.create('db_open', path, path.bytesize, options), 'db_destroy') }
      end
      block_given? ? db.use { |resource| yield resource } : db
    end

    # Return an independent binary String, including '', or nil for an absent key.
    def get(key, read_options: {}, snapshot: nil)
      key = Native.bytes(key)
      LOCK.synchronize do
        Native.options(1, read_options) do |options|
          Native.read('db_get', handle, key, key.bytesize, options, Native.snapshot(snapshot))
        end
      end
    rescue Error => e
      raise unless e.status == 1

      nil
    end

    # Insert/replace exact bytes. sync: true requests the engine's flush.
    def put(key, value, **write_options)
      key, value = Native.bytes(key), Native.bytes(value)
      LOCK.synchronize do
        Native.options(2, write_options) { |options| Native.call('db_put', handle, key, key.bytesize, value, value.bytesize, options) }
      end
      nil
    end

    # Delete the binary key; absent keys also succeed.
    def delete(key, **write_options)
      key = Native.bytes(key)
      LOCK.synchronize do
        Native.options(2, write_options) { |options| Native.call('db_delete', handle, key, key.bytesize, options) }
      end
      nil
    end

    # Apply a live Batch atomically, without consuming or clearing it.
    def write(batch, **write_options)
      raise TypeError, 'Expected Batch' unless batch.is_a?(Batch)

      LOCK.synchronize do
        Native.options(2, write_options) { |options| Native.call('batch_write', handle, batch.send(:handle), options) }
      end
      nil
    end

    # Capture an immutable same-instance native read view.
    def create_snapshot
      LOCK.synchronize { Snapshot.new(Native.create('snapshot_create', handle), 'snapshot_release') }
    end

    # Create an unpositioned streaming cursor; snapshot must belong to this DB.
    def iterator(read_options: {}, snapshot: nil)
      LOCK.synchronize do
        Native.options(1, read_options) do |options|
          Cursor.new(Native.create('iterator_create', handle, options, Native.snapshot(snapshot)), 'iterator_destroy')
        end
      end
    end
  end
end
