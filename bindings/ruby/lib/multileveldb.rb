# frozen_string_literal: true

require 'monitor'

# Synchronous binary LevelDB access. All resource operations and finalizers share
# one monitor. Set MULTILEVELDB_LIBRARY before opening the first resource.
module MultiLevelDB
  LOCK = Monitor.new
  private_constant :LOCK
end

require_relative 'multileveldb/error'
require_relative 'multileveldb/native'
require_relative 'multileveldb/resource'
require_relative 'multileveldb/snapshot'
require_relative 'multileveldb/batch'
require_relative 'multileveldb/cursor'
require_relative 'multileveldb/database'
