/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stddef.h>
#include <stdint.h>

#include "serial.h"

#define SERIAL_PORT 0x3F8
#define RLE_ESCAPE 0xFF

// CRC-32 (IEEE 802.3 polynomial, same as zlib)
static uint32_t
crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return crc ^ 0xFFFFFFFF;
}

static inline void
outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t
inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void
serial_init(void)
{
    // Drain pending data
    while (inb(SERIAL_PORT + 5) & 0x01)
        inb(SERIAL_PORT);

    // Disable and reset FIFO
    outb(SERIAL_PORT + 2, 0x00); // Disable FIFO
    outb(SERIAL_PORT + 2, 0x07); // Enable and clear FIFO

    // Clear interrupts
    inb(SERIAL_PORT + 2); // Read IIR
    inb(SERIAL_PORT + 5); // Read LSR
    inb(SERIAL_PORT + 6); // Read MSR
    inb(SERIAL_PORT);     // Read RBR

    outb(SERIAL_PORT + 1, 0x00); // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80); // Enable DLAB
    outb(SERIAL_PORT + 0, 0x01); // Divisor low (115200 baud)
    outb(SERIAL_PORT + 1, 0x00); // Divisor high
    outb(SERIAL_PORT + 3, 0x03); // 8N1
    outb(SERIAL_PORT + 2, 0xC7); // Enable FIFO
    outb(SERIAL_PORT + 4, 0x03); // DTR + RTS (maybe needed for null modem?)
}

void
serial_raw_putc(void *p, char c)
{
    (void) p;
    while (!(inb(SERIAL_PORT + 5) & 0x20))
        ; // Wait for ready
    outb(SERIAL_PORT, c);
}

void
serial_putc(void *p, char c)
{
    (void) p;
    if (c == '\n') {
        // Translate \n to \r\n
        serial_raw_putc(p, '\r');
    }
    serial_raw_putc(p, c);
}

void
serial_puts(const char *s)
{
    while (*s) {
        serial_putc(NULL, *s++);
    }
}

int
serial_data_ready(void)
{
    return inb(SERIAL_PORT + 5) & 0x01;
}

char
serial_getc(void)
{
    while (!serial_data_ready())
        ;
    return inb(SERIAL_PORT);
}

int
serial_gets(char *buf, int maxlen)
{
    int i = 0;
    while (i < maxlen - 1) {
        char c = serial_getc();
        if (c == '\r' || c == '\n') {
            serial_putc(NULL, '\n');
            break;
        }
        if (c == 127 || c == '\b') {
            if (i > 0) {
                i--;
                serial_puts("\b \b");
            }
            continue;
        }
        serial_putc(NULL, c);
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

// RLE encode data and stream directly to serial.
// Format: runs of 3+ identical bytes become <0xFF> <count> <byte>
//         0xFF itself becomes <0xFF> <0x01> <0xFF>
//         other bytes are output directly
// Returns number of bytes written to serial.
size_t
rle_encode_to_serial(const uint8_t *data, size_t len)
{
    size_t encoded_size = 0;
    size_t i = 0;

    while (i < len) {
        uint8_t byte = data[i];
        uint8_t count = 1;

        // Count run length (max 255)
        while (i + count < len && data[i + count] == byte && count < 255) {
            count++;
        }

        if (count >= 3 || byte == RLE_ESCAPE) {
            // Encode as: <escape> <count> <byte>
            serial_raw_putc(NULL, RLE_ESCAPE);
            serial_raw_putc(NULL, count);
            serial_raw_putc(NULL, byte);
            encoded_size += 3;
        } else {
            // Output bytes directly
            for (uint8_t j = 0; j < count; j++) {
                serial_raw_putc(NULL, byte);
            }
            encoded_size += count;
        }
        i += count;
    }

    return encoded_size;
}

size_t
send_file_to_serial(const char *path, const void *data, size_t size)
{
    uint32_t checksum = crc32((const uint8_t *) data, size);

    // Send start marker and header
    // Format: path:rle:original_size:crc32_hex
    serial_puts(FILE_START_MARKER);
    serial_putc(NULL, '\n');
    printf("%s:rle:%zu:%08x\n", path, size, checksum);

    // Stream RLE-encoded data directly to serial
    size_t encoded_size = rle_encode_to_serial((const uint8_t *) data, size);

    // Send end marker
    serial_puts(FILE_END_MARKER);
    serial_putc(NULL, '\n');

    // Use integer math to avoid FPU - compute ratio as percentage * 10 for one
    // decimal
    unsigned ratio_x10 = (unsigned) (encoded_size * 1000 / size);
    printf("Sent %s (%zu bytes encoded, %zu bytes original, %u.%u%% ratio)\n",
           path, encoded_size, size, ratio_x10 / 10, ratio_x10 % 10);

    return size;
}
