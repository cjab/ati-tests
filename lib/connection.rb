# frozen_string_literal: true

require 'base64'
require 'socket'
require 'zlib'
require_relative 'rle'

# Segment types yielded by Connection#command
TextSegment = Struct.new(:data)
FileSegment = Struct.new(:filename, :rle_data, :original_size, :crc_ok)
FileProgressSegment = Struct.new(:filename, :bytes_received)

# Connection to ATI Rage 128 test firmware over TCP or serial.
#
# Understands the firmware's protocol: EOT+prompt framing, command echo,
# and file transfer markers. Callers receive parsed TextSegment and
# FileSegment objects rather than raw byte chunks.
class Connection
  FILE_START_MARKER = "===FILE_START===\r\n".b
  FILE_END_MARKER   = "===FILE_END===\r\n".b
  EOT_PROMPT        = "\x04> ".b

  # Maximum bytes to hold back when scanning for split markers.
  # Ensures we catch a marker split across two reads.
  HOLDBACK = [FILE_START_MARKER.bytesize, FILE_END_MARKER.bytesize,
              EOT_PROMPT.bytesize].max - 1

  DEFAULT_TIMEOUT = 30

  def initialize(io, timeout: DEFAULT_TIMEOUT)
    @io = io
    @timeout = timeout
  end

  # Send a command and process the response.
  #
  # With a block: yields TextSegment and FileSegment objects as they
  # arrive, providing low-latency streaming of text output while
  # silently capturing file transfers.
  #
  # Without a block: buffers the entire response and returns
  # [text_string, files_array].
  def command(cmd, timeout: @timeout, &block)
    write("#{cmd}\n")

    if block
      read_response(echo: cmd, timeout: timeout, &block)
    else
      text = ''.b
      files = []
      read_response(echo: cmd, timeout: timeout) do |segment|
        case segment
        when TextSegment then text << segment.data
        when FileSegment then files << segment
        end
      end
      [text, files]
    end
  end

  # Discard everything until the next prompt. Used for initial sync.
  def drain(timeout: @timeout)
    write("\n")
    read_response(timeout: timeout) { |_| }
    nil
  end

  def close
    @io.close
  end

  def self.tcp(host, port, timeout: DEFAULT_TIMEOUT)
    sock = TCPSocket.new(host, port)
    sock.setsockopt(Socket::SOL_SOCKET, Socket::SO_RCVTIMEO,
                    [timeout, 0].pack('l_2'))
    new(sock, timeout: timeout)
  end

  def self.serial(device, baud, timeout: DEFAULT_TIMEOUT)
    system("stty -F #{device} #{baud} cs8 -cstopb -parenb raw -echo " \
           "2>/dev/null") or
      raise "Failed to configure serial port #{device}"

    io = File.open(device, 'r+')
    io.sync = true
    new(io, timeout: timeout)
  end

  private

  def write(data)
    @io.write(data)
  end

  # Read a single chunk from the socket. Returns the bytes read.
  # Raises on timeout or EOF.
  def read_chunk(timeout)
    ready = IO.select([@io], nil, nil, timeout)
    raise 'Timeout waiting for response' unless ready

    @io.read_nonblock(4096)
  rescue EOFError
    raise 'Connection closed'
  end

  # Core response parser. Reads from the socket and yields TextSegment
  # and FileSegment objects to the block.
  def read_response(echo: nil, timeout: @timeout, &block)
    buf = ''.b
    state = :text
    strip_echo = !echo.nil?
    file_buf = ''.b

    loop do
      buf << read_chunk(timeout)

      # Process the buffer, re-entering on state transitions since
      # remaining data may be immediately actionable in the new state.
      loop do
        prev_state = state

        case state
        when :text
          buf, state, strip_echo = scan_text(buf, strip_echo, file_buf, &block)
        when :file
          buf, state = scan_file(buf, file_buf, &block)
        end

        break if state == :done
        break if state == prev_state # stable — need more data
      end

      break if state == :done
    end
  end

  # Process buffer while in :text state.
  #
  # Scans for echo to strip, EOT+prompt, and FILE_START_MARKER.
  # Flushes safe text as TextSegments via the block.
  #
  # Returns [remaining_buf, new_state, strip_echo].
  def scan_text(buf, strip_echo, file_buf, &block)
    # Strip command echo: everything up to and including first \n
    if strip_echo
      newline_idx = buf.index("\n".b)
      if newline_idx
        buf = buf[(newline_idx + 1)..]
        strip_echo = false
      else
        return [buf, :text, true]
      end
    end

    # Check for EOT+prompt — response is complete
    eot_idx = buf.index(EOT_PROMPT)
    if eot_idx
      flush_text(buf[0...eot_idx], &block) if eot_idx.positive?
      return [''.b, :done, false]
    end

    # Check for file transfer start
    start_idx = buf.index(FILE_START_MARKER)
    if start_idx
      flush_text(buf[0...start_idx], &block) if start_idx.positive?
      buf = buf[(start_idx + FILE_START_MARKER.bytesize)..]
      file_buf.clear
      return [buf, :file, false]
    end

    # No markers found. Flush everything except holdback region,
    # which might contain the beginning of a split marker.
    if buf.bytesize > HOLDBACK
      safe_end = buf.bytesize - HOLDBACK
      flush_text(buf[0...safe_end], &block)
      buf = buf[safe_end..]
    end

    [buf, :text, false]
  end

  # Process buffer while in :file state.
  #
  # Accumulates data into file_buf until FILE_END_MARKER is found,
  # then decodes the file and yields a FileSegment. Yields
  # FileProgressSegments as data arrives.
  #
  # Returns [remaining_buf, new_state].
  def scan_file(buf, file_buf, &block)
    file_buf << buf
    end_idx = file_buf.index(FILE_END_MARKER)

    unless end_idx
      # Yield progress if we can extract the filename from the header
      newline_idx = file_buf.index("\n".b)
      if newline_idx
        header = parse_header(file_buf[0...newline_idx])
        filename = header ? header[0] : nil
        yield FileProgressSegment.new(filename, file_buf.bytesize) if filename
      end
      return [''.b, :file]
    end

    file_content = file_buf[0...end_idx]
    remaining = file_buf[(end_idx + FILE_END_MARKER.bytesize)..]
    file_buf.clear

    segment = decode_file(file_content)
    yield segment if segment

    [remaining, :text]
  end

  # Yield text data as a TextSegment, applying CRLF normalization.
  def flush_text(data)
    return if data.nil? || data.empty?

    cleaned = data.gsub("\r\n", "\n").gsub("\r", '')
    yield TextSegment.new(cleaned) unless cleaned.empty?
  end

  # Parse a file transfer block: header line + encoded payload.
  # Returns a FileSegment or nil on error.
  def decode_file(data)
    newline_idx = data.index("\n".b)
    return nil unless newline_idx

    header_line = data[0...newline_idx]
    payload = data[(newline_idx + 1)..]

    filename, encoding, original_size, expected_crc = parse_header(header_line)
    return nil unless filename

    if encoding != 'rle+base64'
      warn "Warning: Unknown encoding '#{encoding}' for #{filename}"
      return nil
    end

    rle_data = Base64.decode64(payload)
    decoded = RLE.decode(rle_data)

    if decoded.bytesize != original_size
      warn "Warning: Size mismatch for #{filename}: " \
           "expected #{original_size}, got #{decoded.bytesize}"
    end

    actual_crc = Zlib.crc32(decoded) & 0xFFFFFFFF
    crc_ok = actual_crc == expected_crc

    if !crc_ok
      warn "ERROR: CRC mismatch for #{filename}: " \
           "expected #{expected_crc.to_s(16).rjust(8, '0')}, " \
           "got #{actual_crc.to_s(16).rjust(8, '0')}"
    end

    FileSegment.new(filename, rle_data, original_size, crc_ok)
  end

  # Parse file header: filename:encoding:original_size:crc32_hex
  def parse_header(line)
    parts = line.strip.split(':')
    return nil unless parts.length == 4

    filename = parts[0]
    encoding = parts[1]
    original_size = Integer(parts[2])
    expected_crc = Integer(parts[3], 16)
    [filename, encoding, original_size, expected_crc]
  rescue ArgumentError
    nil
  end
end
