/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REPL_H
#define REPL_H

#include "../ati/ati.h"
#include <stdint.h>

void repl(ati_device_t *dev);

// Parsing utilities
int parse_hex(const char *s, uint32_t *out);
int parse_int(const char *s, uint32_t *out);

#endif
