/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

#include "tinyprintf.h"

// Record framing: single ASCII control characters for unambiguous parsing.
// File record:  \x1C <header> \x1E <payload> \x1C
// Error record: \x1D <text> \x1D  (reserved for future use)
#define RECORD_FILE_SEP '\x1C'  // File Separator — delimits file records
#define RECORD_FIELD_SEP '\x1E' // Record Separator — header/payload boundary

void serial_init(void);
void serial_putc(void *p, char c);
void serial_puts(const char *s);
int serial_data_ready(void);
char serial_getc(void);
int serial_gets(char *buf, int maxlen);
size_t rle_encode_to_serial(const uint8_t *data, size_t len);
size_t send_file_to_serial(const char *path, const void *data, size_t size);

#endif
