/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>
#include <fcntl.h>
#include <pci/pci.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../platform.h"

struct platform_pci_device {
    struct pci_access *pacc;
    struct pci_dev *pci_dev;
};

#define FATAL                                                                  \
    do {                                                                       \
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,     \
                __FILE__, errno, strerror(errno));                             \
        exit(1);                                                               \
    } while (0)

struct pci_dev *
find_device(struct pci_access *pacc)
{
    struct pci_dev *dev;

    pci_init(pacc);
    pci_scan_bus(pacc);

    for (dev = pacc->devices; dev; dev = dev->next) {
        if (dev->vendor_id == ATI_VENDOR_ID) {
            return dev;
        }
    }

    return NULL;
}

void
platform_pci_get_name(platform_pci_device_t *dev, char *buf, size_t len)
{
    pci_lookup_name(dev->pacc, buf, len, PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
                    dev->pci_dev->vendor_id, dev->pci_dev->device_id);
}

static platform_pci_device_t *
platform_pci_init_internal(void)
{
    platform_pci_device_t *dev = malloc(sizeof(platform_pci_device_t));
    if (!dev)
        FATAL;

    dev->pacc = pci_alloc();
    if (!dev->pacc)
        FATAL;

    dev->pci_dev = find_device(dev->pacc);
    if (!dev->pci_dev)
        FATAL;

    return dev;
}

void
platform_pci_destroy(platform_pci_device_t *dev)
{
    if (!dev)
        return;
    if (dev->pacc)
        pci_cleanup(dev->pacc);
    free(dev);
}

void *
platform_pci_map_bar(platform_pci_device_t *dev, int bar_idx)
{
    struct pci_dev *pci = dev->pci_dev;
    char pci_loc[32];
    sprintf(pci_loc, "%04x:%02x:%02x.%d", pci->domain, pci->bus, pci->dev,
            pci->func);

    char base_path[256];
    sprintf(base_path, "/sys/bus/pci/devices/%s", pci_loc);

    char bar_path[512];
    sprintf(bar_path, "%s/resource%d", base_path, bar_idx);

    int bar_fd = open(bar_path, O_RDWR | O_SYNC);
    if (bar_fd == -1)
        FATAL;

    void *bar = mmap(NULL, pci->size[bar_idx], PROT_READ | PROT_WRITE,
                     MAP_SHARED, bar_fd, 0);
    if (bar == (void *) -1)
        FATAL;

    return bar;
}

void
platform_pci_unmap_bar(platform_pci_device_t *dev, void *addr, int bar_idx)
{
    munmap(addr, dev->pci_dev->size[bar_idx]);
}

size_t
platform_pci_get_bar_size(platform_pci_device_t *dev, int bar_idx)
{
    return dev->pci_dev->size[bar_idx];
};

const uint8_t *
platform_get_fixture(const char *name, size_t *size_out)
{
    char path[512];
    snprintf(path, sizeof(path), "fixtures/%s.bin", name);

    FILE *f = fopen(path, "rb");
    if (!f) {
        *size_out = 0;
        return NULL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(file_size);
    if (!data) {
        fprintf(stderr, "Failed to allocate memory for fixture\n");
        fclose(f);
        *size_out = 0;
        return NULL;
    }

    size_t read = fread(data, 1, file_size, f);
    fclose(f);

    if (read != file_size) {
        fprintf(stderr, "Failed to read complete fixture file\n");
        free(data);
        *size_out = 0;
        return NULL;
    }

    *size_out = file_size;
    return data;
}

void
platform_free_fixture(const uint8_t *data)
{
    free((void *) data);
}

size_t
platform_write_file(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", path,
                strerror(errno));
    }

    size_t written = fwrite(data, 1, size, f);
    if (written != size) {
        fprintf(stderr, "Warning: only wrote %zu of %zu bytes\n", written,
                size);
    }
    fclose(f);

    return written;
}

platform_t *
platform_init(int argc, char **argv)
{
    static platform_t platform;

    // Skip program name (argv[0])
    platform.argc = argc > 1 ? argc - 1 : 0;
    platform.argv = argc > 1 ? &argv[1] : NULL;

    // Initialize PCI device
    platform.pci_dev = platform_pci_init_internal();

    return &platform;
}

void
platform_destroy(platform_t *platform)
{
    // Nothing to do on Linux
    (void) platform;
}
