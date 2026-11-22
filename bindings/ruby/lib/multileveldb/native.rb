# frozen_string_literal: true

require 'fiddle/import'

module MultiLevelDB
  module Native # :nodoc:
    extend Fiddle::Importer

    def self.load!
      return if @loaded

      windows = RUBY_PLATFORM.match?(/mswin|mingw/)
      library = ENV.fetch('MULTILEVELDB_LIBRARY') do
        windows ? 'multileveldb.dll' : RUBY_PLATFORM.include?('darwin') ? 'libmultileveldb.dylib' : 'libmultileveldb.so'
      end
      # Ruby's Windows loader excludes PATH for dependent DLLs. Limit temporary
      # search registration to the explicitly selected library's own directory.
      if windows && File.file?(library)
        kernel = Fiddle.dlopen('kernel32.dll')
        add = Fiddle::Function.new(kernel['AddDllDirectory'], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_VOIDP)
        remove = Fiddle::Function.new(kernel['RemoveDllDirectory'], [Fiddle::TYPE_VOIDP], Fiddle::TYPE_INT)
        directory = File.dirname(File.expand_path(library)).encode('UTF-16LE').b + "\0\0".b
        cookie = add.call(directory)
        begin
          dlload library
        ensure
          remove.call(cookie) unless cookie.to_i.zero?
        end
      else
        dlload library
      end
      extern 'unsigned int ml_abi_version()'
      extern 'char* ml_error_message(void*)'
      extern 'void ml_error_free(void*)'
      extern 'void ml_buffer_free(void*)'
      extern 'int ml_options_create(unsigned int, void*, void*)'
      extern 'int ml_options_set(void*, unsigned int, unsigned long long, void*)'
      extern 'void ml_options_destroy(void*)'
      extern 'int ml_db_open(void*, size_t, void*, void*, void*)'
      extern 'int ml_db_create(void*, size_t, void*, void*, void*)'
      extern 'void ml_db_destroy(void*)'
      extern 'int ml_db_get(void*, void*, size_t, void*, void*, void*, void*, void*)'
      extern 'int ml_db_put(void*, void*, size_t, void*, size_t, void*, void*)'
      extern 'int ml_db_delete(void*, void*, size_t, void*, void*)'
      extern 'int ml_iterator_create(void*, void*, void*, void*, void*)'
      extern 'void ml_iterator_destroy(void*)'
      extern 'unsigned int ml_iterator_valid(void*)'
      extern 'int ml_iterator_seek(void*, void*, size_t, void*)'
      %w[seek_first seek_last next previous status].each { |op| extern "int ml_iterator_#{op}(void*, void*)" }
      %w[key value].each { |op| extern "int ml_iterator_#{op}(void*, void*, void*, void*)" }
      extern 'int ml_batch_create(void*, void*)'
      extern 'void ml_batch_destroy(void*)'
      extern 'int ml_batch_clear(void*, void*)'
      extern 'int ml_batch_put(void*, void*, size_t, void*, size_t, void*)'
      extern 'int ml_batch_delete(void*, void*, size_t, void*)'
      extern 'int ml_batch_write(void*, void*, void*, void*)'
      extern 'int ml_snapshot_create(void*, void*, void*)'
      extern 'void ml_snapshot_release(void*)'
      raise LoadError, 'Incompatible MultiLevelDB ABI' unless ml_abi_version >> 16 == 1

      @loaded = true
    end

    def self.slot
      pointer = Fiddle::Pointer.malloc(Fiddle::SIZEOF_VOIDP, Fiddle::RUBY_FREE)
      pointer[0, Fiddle::SIZEOF_VOIDP] = [0].pack('J')
      pointer
    end

    def self.number(slot)
      slot[0, Fiddle::SIZEOF_VOIDP].unpack1('J')
    end

    def self.call(name, *args)
      load!
      error = slot
      status = public_send("ml_#{name}", *args, error)
      pointer = number(error)
      begin
        raise Error.new(status, pointer.zero? ? '' : ml_error_message(pointer).to_s) unless status.zero?
      ensure
        ml_error_free(pointer)
      end
    end

    def self.create(name, *args)
      out = slot
      call(name, *args, out)
      number(out)
    end

    def self.read(name, *args)
      out, length = slot, slot
      begin
        call(name, *args, out, length)
        size = number(length)
        size.zero? ? ''.b : Fiddle::Pointer.new(number(out))[0, size].b
      ensure
        ml_buffer_free(number(out))
      end
    end

    def self.bytes(value)
      raise TypeError, 'Expected a String of binary bytes' unless value.is_a?(String)

      value.b
    end

    FIELDS = {
      create_if_missing: [0, 1], error_if_exists: [0, 2], paranoid_checks: [0, 3],
      write_buffer_size: [0, 4], max_open_files: [0, 5], block_size: [0, 6],
      block_restart_interval: [0, 7], compression: [0, 8], cache_size: [0, 9],
      verify_checksums: [1, 10], fill_cache: [1, 11], sync: [2, 12]
    }.freeze

    def self.options(kind, values)
      raise TypeError, 'Expected an options Hash' unless values.is_a?(Hash)

      pointer = create('options_create', kind)
      begin
        values.each do |name, value|
          field = FIELDS[name]
          raise ArgumentError, "Invalid option for this operation: #{name}" unless field && field[0] == kind

          if name == :compression
            raise ArgumentError, 'compression must be :none or :snappy' unless %i[none snappy].include?(value)

            value = value == :snappy ? 1 : 0
          end
          value = value ? 1 : 0 if value == true || value == false
          raise ArgumentError, "#{name} must be uint64" unless value.is_a?(Integer) && value.between?(0, 0xffffffffffffffff)

          call('options_set', pointer, field[1], value)
        end
        yield pointer
      ensure
        ml_options_destroy(pointer)
      end
    end

    def self.snapshot(value)
      return 0 if value.nil?
      raise TypeError, 'Expected Snapshot or nil' unless value.is_a?(Snapshot)

      value.send(:handle)
    end
  end
  private_constant :Native
end
