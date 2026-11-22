# frozen_string_literal: true

module MultiLevelDB
  # Native failure retaining its numeric status, stable code and diagnostic.
  class Error < StandardError
    # Numeric native status identifier.
    attr_reader :status
    # Stable status name, such as INVALID_ARGUMENT or DATABASE_CLOSED.
    attr_reader :code

    # Constructed by the adapter after copying and freeing a native error.
    def initialize(status, message)
      @status = status
      @code = %w[OK NOT_FOUND INVALID_ARGUMENT IO_ERROR CORRUPTION DATABASE_LOCKED DATABASE_CLOSED UNSUPPORTED INTERNAL_ERROR][status] || 'INTERNAL_ERROR'
      super(message)
    end
  end
end
