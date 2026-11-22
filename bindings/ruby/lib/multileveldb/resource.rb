# frozen_string_literal: true

module MultiLevelDB
  # Shared deterministic ownership and block cleanup for all native resources.
  class Resource
    def initialize(pointer, free)
      # :nodoc:
      @state = [pointer, free]
      ObjectSpace.define_finalizer(self, self.class.send(:cleanup, @state))
    end

    def self.cleanup(state)
      # :nodoc:
      proc do
        LOCK.synchronize do
          pointer, free = state
          state[0] = 0
          Native.public_send("ml_#{free}", pointer) unless pointer.zero?
        end
      end
    end

    private_class_method :cleanup

    # Release once. Existing dependent cursors/snapshots keep the engine alive.
    def close
      LOCK.synchronize { self.class.send(:cleanup, @state).call }
      nil
    end

    # Yield this resource and ensure close, including when the block raises.
    def use
      yield self
    ensure
      close
    end

    private

    def handle
      raise Error.new(6, 'Resource is closed') if @state[0].zero?

      @state[0]
    end
  end
end
