# frozen_string_literal: true

module MultiLevelDB
  # Native read view. Obtain from Database#create_snapshot, then close or use.
  class Snapshot < Resource; end
end
