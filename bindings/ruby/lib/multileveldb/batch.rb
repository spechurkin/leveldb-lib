# frozen_string_literal: true

module MultiLevelDB
  # Database-independent reusable sequence of atomic write operations.
  class Batch < Resource
    # Allocate an empty batch. Use #use for block-scoped ownership.
    def initialize
      LOCK.synchronize { super(Native.create('batch_create'), 'batch_destroy') }
    end

    # Append copied binary key/value data and return self for chaining.
    def put(key, value)
      key, value = Native.bytes(key), Native.bytes(value)
      LOCK.synchronize { Native.call('batch_put', handle, key, key.bytesize, value, value.bytesize) }
      self
    end

    # Append a copied deletion and return self.
    def delete(key)
      key = Native.bytes(key)
      LOCK.synchronize { Native.call('batch_delete', handle, key, key.bytesize) }
      self
    end

    # Discard all pending operations; no database is changed.
    def clear
      LOCK.synchronize { Native.call('batch_clear', handle) }
    end
  end
end
