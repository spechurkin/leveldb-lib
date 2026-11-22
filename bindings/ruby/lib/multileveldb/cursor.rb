# frozen_string_literal: true

module MultiLevelDB
  # Bidirectional streaming cursor. Check #status after a scan finishes.
  class Cursor < Resource
    # Seek to first record; an empty view becomes invalid without an error.
    def seek_to_first
      LOCK.synchronize { Native.call('iterator_seek_first', handle) }
    end

    # Seek to last record; an empty view becomes invalid without an error.
    def seek_to_last
      LOCK.synchronize { Native.call('iterator_seek_last', handle) }
    end

    # Seek to the first key >= target, or an invalid end position.
    def seek(key)
      key = Native.bytes(key)
      LOCK.synchronize { Native.call('iterator_seek', handle, key, key.bytesize) }
    end

    # Advance. Passing the end succeeds; moving an invalid cursor raises NOT_FOUND.
    def next
      LOCK.synchronize { Native.call('iterator_next', handle) }
    end

    # Move backward. An already invalid cursor raises NOT_FOUND.
    def previous
      LOCK.synchronize { Native.call('iterator_previous', handle) }
    end

    # Test position. This does not check the engine's recorded status.
    def valid?
      LOCK.synchronize { !Native.ml_iterator_valid(handle).zero? }
    end

    # Return an independent binary key; invalid position raises NOT_FOUND.
    def key
      LOCK.synchronize { Native.read('iterator_key', handle) }
    end

    # Return an independent binary value, including ''; invalid position raises.
    def value
      LOCK.synchronize { Native.read('iterator_value', handle) }
    end

    # Raise on a recorded engine failure; ordinary scan exhaustion succeeds.
    def status
      LOCK.synchronize { Native.call('iterator_status', handle) }
    end
  end
end
