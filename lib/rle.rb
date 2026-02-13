# frozen_string_literal: true

# RLE (Run-Length Encoding) utilities
# Format: 0xFF <count> <byte> for runs of 3+, raw bytes otherwise

module RLE
  ESCAPE = 0xFF

  # Encode binary data using RLE compression
  def self.encode(data)
    data.bytes
        .chunk_while { |a, b| a == b }
        .flat_map { |run| encode_run(run) }
        .pack('C*')
  end

  # Decode RLE-compressed data using an enumerator
  def self.decode(data)
    tokenize(data.bytes)
      .flat_map { |token| token[0] == ESCAPE ? [token[2]] * token[1] : token }
      .pack('C*')
  end

  class << self
    private

    # Encode a run of identical bytes, splitting runs > 255
    def encode_run(run)
      byte = run.first
      run.each_slice(255).flat_map do |slice|
        if slice.length >= 3 || byte == ESCAPE
          [ESCAPE, slice.length, byte]
        else
          slice
        end
      end
    end

    def tokenize(bytes)
      Enumerator.new do |tokens|
        enum = bytes.each
        loop do
          byte = enum.next
          if byte == ESCAPE
            tokens << [byte, enum.next, enum.next]
          else
            tokens << [byte]
          end
        end
      end
    end
  end
end
