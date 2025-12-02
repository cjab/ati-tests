/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

#include "tinyprintf.h"

#define FILE_START_MARKER "===FILE_START==="
#define FILE_END_MARKER "===FILE_END==="

void serial_init(void);
void serial_putc(void *p, char c);
void serial_puts(const char *s);
int serial_data_ready(void);
char serial_getc(void);
int serial_gets(char *buf, int maxlen);
size_t rle_encode_to_serial(const uint8_t *data, size_t len);
size_t send_file_to_serial(const char *path, const void *data, size_t size);

#endif
