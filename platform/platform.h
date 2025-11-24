/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

#define ATI_VENDOR_ID 0x1002

typedef struct platform_pci_device platform_pci_device_t;

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
