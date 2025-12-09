/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PLATFORM_BAREMETAL_H
#define PLATFORM_BAREMETAL_H

// IWYU pragma: begin_exports
#include "tinyprintf.h"
// IWYU pragma: end_exports
#include <stdint.h>

/* Minimal libc replacement declarations */
typedef void FILE;
#define stdin ((FILE *) 0)
#define stdout ((FILE *) 1)
#define stderr ((FILE *) 2)

int fprintf(FILE *stream, const char *format, ...);
int fflush(FILE *stream);
char *fgets(char *s, int n, FILE *stream);
void exit(int status);
int snprintf(char *str, size_t size, const char *format, ...);
int strcmp(const char *s1, const char *s2);
int strcasecmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void serial_init(void);
void serial_putc(void *p, char c);

/* Multiboot support */
struct multiboot_info;
void platform_init_args(uint32_t magic, struct multiboot_info *mbi);

#endif
