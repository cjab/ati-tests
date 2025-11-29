#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""
Capture files sent over serial from baremetal tests.

Reads from stdin (or a serial device) and extracts RLE-encoded files
sent using the ===FILE_START===/===FILE_END=== protocol.

Usage:
    # From QEMU serial output:
    qemu-system-x86_64 ... -serial stdio 2>/dev/null | ./scripts/serial_capture.py

    # From a file (for testing):
    ./scripts/serial_capture.py < serial_output.bin

    # Specify output directory:
    ./scripts/serial_capture.py -o /tmp/dumps < serial_output.bin
"""

import argparse
import os
import signal
import sys
import zlib

FILE_START_MARKER = b"===FILE_START===\n"
FILE_END_MARKER = b"===FILE_END===\n"
RLE_ESCAPE = 0xFF


def parse_header(header_line):
    """Parse header line: filename:rle:original_size:crc32_hex"""
    parts = header_line.decode("utf-8").strip().split(":")
    if len(parts) != 4:
        return None, None, None, None
    filename = parts[0]
    encoding = parts[1]
    try:
        original_size = int(parts[2])
        expected_crc = int(parts[3], 16)
    except ValueError:
        return None, None, None, None
    return filename, encoding, original_size, expected_crc


def rle_decode(data):
    """
    Decode RLE-encoded data.
    Format: <0xFF> <count> <byte> for runs, or raw bytes otherwise.
    """
    result = bytearray()
    i = 0
    while i < len(data):
        byte = data[i]
        if byte == RLE_ESCAPE:
            if i + 2 >= len(data):
                # Incomplete escape sequence at end - treat as raw
                result.append(byte)
                i += 1
            else:
                count = data[i + 1]
                value = data[i + 2]
                result.extend([value] * count)
                i += 3
        else:
            result.append(byte)
            i += 1
    return bytes(result)


def capture_files(input_stream, output_dir):
    """
    Read from input_stream, pass through text, capture and decode RLE files.
    """
    buffer = bytearray()
    files_captured = 0

    # Read in binary mode
    stdin_binary = input_stream.buffer if hasattr(input_stream, "buffer") else input_stream

    while True:
        # Use read1() to return immediately with available data rather than
        # blocking until the full buffer size is read. This allows passthrough
        # text to display in real-time.
        chunk = stdin_binary.read1(4096)
        if not chunk:
            break

        buffer.extend(chunk)

        while True:
            # Look for start marker
            start_idx = buffer.find(FILE_START_MARKER)

            if start_idx == -1:
                # No start marker found - output everything except bytes that
                # could be the start of a marker split across chunks.
                # Find how many trailing bytes match a prefix of the marker.
                keep = 0
                for i in range(1, min(len(buffer), len(FILE_START_MARKER)) + 1):
                    if FILE_START_MARKER.startswith(buffer[-i:]):
                        keep = i
                safe_len = len(buffer) - keep
                if safe_len > 0:
                    sys.stdout.buffer.write(buffer[:safe_len])
                    sys.stdout.buffer.flush()
                    del buffer[:safe_len]
                break

            # Output everything before the start marker
            if start_idx > 0:
                sys.stdout.buffer.write(buffer[:start_idx])
                sys.stdout.buffer.flush()
                del buffer[:start_idx]
                start_idx = 0

            # Find the header line (ends with newline after marker)
            header_start = len(FILE_START_MARKER)
            header_end = buffer.find(b"\n", header_start)

            if header_end == -1:
                # Need more data for complete header
                break

            header_line = buffer[header_start:header_end]
            filename, encoding, original_size, expected_crc = parse_header(header_line)

            if filename is None or encoding is None or original_size is None or expected_crc is None:
                # Invalid header - output marker and continue
                sys.stdout.buffer.write(FILE_START_MARKER)
                sys.stdout.buffer.flush()
                del buffer[: len(FILE_START_MARKER)]
                continue

            if encoding != "rle":
                sys.stderr.write(f"Warning: Unknown encoding '{encoding}' for {filename}\n")
                sys.stdout.buffer.write(FILE_START_MARKER)
                sys.stdout.buffer.flush()
                del buffer[: len(FILE_START_MARKER)]
                continue

            # For RLE, we don't know the encoded size upfront, so we need to
            # find the end marker
            data_start = header_end + 1
            end_marker_idx = buffer.find(FILE_END_MARKER, data_start)

            if end_marker_idx == -1:
                # Need more data - haven't seen end marker yet
                break

            # Extract encoded data
            encoded_data = bytes(buffer[data_start:end_marker_idx])

            # Decode RLE
            try:
                decoded_data = rle_decode(encoded_data)
            except Exception as e:
                sys.stderr.write(f"Warning: RLE decode failed for {filename}: {e}\n")
                del buffer[:end_marker_idx + len(FILE_END_MARKER)]
                continue

            # Verify size
            if len(decoded_data) != original_size:
                sys.stderr.write(
                    f"Warning: Size mismatch for {filename}: "
                    f"expected {original_size}, got {len(decoded_data)}\n"
                )

            # Verify CRC
            actual_crc = zlib.crc32(decoded_data) & 0xFFFFFFFF
            if actual_crc != expected_crc:
                sys.stderr.write(
                    f"ERROR: CRC mismatch for {filename}: "
                    f"expected {expected_crc:08x}, got {actual_crc:08x}\n"
                )

            # Write file
            output_path = os.path.join(output_dir, os.path.basename(filename))

            with open(output_path, "wb") as f:
                f.write(decoded_data)

            files_captured += 1
            ratio = len(encoded_data) / len(decoded_data) * 100 if decoded_data else 0
            sys.stderr.write(
                f"Captured: {output_path} "
                f"({len(encoded_data)} bytes encoded -> {len(decoded_data)} bytes, "
                f"{ratio:.1f}% ratio)\n"
            )

            # Remove processed data from buffer
            del buffer[:end_marker_idx + len(FILE_END_MARKER)]

    # Flush any remaining buffer
    if buffer:
        sys.stdout.buffer.write(buffer)
        sys.stdout.buffer.flush()

    return files_captured


def main():
    parser = argparse.ArgumentParser(
        description="Capture files sent over serial from baremetal tests"
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        default=".",
        help="Directory to write captured files (default: current directory)",
    )
    args = parser.parse_args()

    # Create output directory if needed
    os.makedirs(args.output_dir, exist_ok=True)

    files_captured = capture_files(sys.stdin, args.output_dir)

    if files_captured > 0:
        sys.stderr.write(f"\nTotal files captured: {files_captured}\n")


if __name__ == "__main__":
    # Handle SIGPIPE (e.g., when piped to head) - exit silently
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    try:
        main()
    except KeyboardInterrupt:
        # Ctrl+C - exit silently
        sys.stderr.write("\n")
        sys.exit(0)
    except BrokenPipeError:
        # Upstream process died - exit silently
        sys.exit(0)
