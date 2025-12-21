/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// IWYU pragma: begin_exports
#ifdef PLATFORM_BAREMETAL
#include "baremetal/baremetal.h"
#else
#include "linux/linux.h"
#endif
// IWYU pragma: end_exports

#define ATI_VENDOR_ID 0x1002

typedef struct platform_pci_device platform_pci_device_t;

/* Platform state - everything needed from platform initialization */
typedef struct {
    int argc;
    char **argv;
    platform_pci_device_t *pci_dev;
} platform_t;

platform_t *platform_init(int argc, char **argv);
void platform_destroy(platform_t *platform);
void platform_reboot(void);
void platform_pci_destroy(platform_pci_device_t *dev);
void platform_pci_get_name(platform_pci_device_t *dev, char *buf, size_t len);

void *platform_pci_map_bar(platform_pci_device_t *dev, int bar_idx);
void platform_pci_unmap_bar(platform_pci_device_t *dev, void *addr,
                            int bar_idx);
size_t platform_pci_get_bar_size(platform_pci_device_t *dev, int bar_idx);

/* Timing */
void udelay(unsigned int us);

/* Fixture access - abstracted from filesystem */
const uint8_t *platform_get_fixture(const char *name, size_t *size_out);
void platform_free_fixture(const uint8_t *data);

/* File I/O - for Linux platform only */
size_t platform_write_file(const char *path, const void *data, size_t size);

#endif
