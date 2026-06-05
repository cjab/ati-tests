/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REPL_H
#define REPL_H

#include "../ati/ati.h"
#include <stdint.h>

void repl(ati_device_t *dev);

// Parsing utilities
int parse_hex(const char *s, uint32_t *out);
int parse_int(const char *s, uint32_t *out);
void print_usage_colored(const char *usage);
int parse_reg(ati_device_t *dev, const char *s);

#endif
