/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

#ifdef PLATFORM_BAREMETAL
typedef void FILE;
#define stdin ((FILE *)0)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int fflush(FILE *stream);
char *fgets(char *s, int n, FILE *stream);
void exit(int status);
int snprintf(char *str, size_t size, const char *format, ...);
int strcmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void serial_init(void);
void serial_putc(char c);
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif

#define ATI_VENDOR_ID 0x1002

typedef struct platform_pci_device platform_pci_device_t;

void platform_init(void);
platform_pci_device_t *platform_pci_init(void);
void platform_pci_destroy(platform_pci_device_t *dev);
void platform_pci_get_name(platform_pci_device_t *dev, char *buf, size_t len);

void *platform_pci_map_bar(platform_pci_device_t *dev, int bar_idx);
void platform_pci_unmap_bar(platform_pci_device_t *dev, void *addr,
                            int bar_idx);
size_t platform_pci_get_bar_size(platform_pci_device_t *dev, int bar_idx);

/* File I/O */
void* platform_read_file(const char *path, size_t *size_out);
void platform_free_file(void *data);
size_t platform_write_file(const char *path, const void *data, size_t size);

#endif
