/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

// clang-format off
// I/O Ports
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// PCI Register Offsets
#define PCI_VENDOR_ID        0x00     // Vendor ID (16-bit)
#define PCI_DEVICE_ID        0x02     // Device ID (16-bit)
#define PCI_COMMAND          0x04     // Command register
#define PCI_STATUS           0x06     // Status register
#define PCI_CLASS_REVISION   0x08     // Class code and revision
#define PCI_HEADER_TYPE      0x0E     // Header type
#define PCI_BAR0             0x10     // Base Address Register 0
#define PCI_BAR1             0x14     // Base Address Register 1
#define PCI_BAR2             0x18     // Base Address Register 2

// PCI Command Register Bits
#define PCI_COMMAND_IO       0x01     // Enable I/O Space
#define PCI_COMMAND_MEMORY   0x02     // Enable Memory Space
#define PCI_COMMAND_MASTER   0x04     // Enable Bus Mastering
// clang-format on

#define NUM_BARS 8

struct platform_pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t bar[NUM_BARS];
};

#define FATAL                                                                  \
    do {                                                                       \
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,     \
                __FILE__, errno, strerror(errno));                             \
        exit(1);                                                               \
    } while (0)

static platform_pci_device_t g_pci_dev;

static inline void
outl(uint16_t port, uint32_t val)
{
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t
inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static uint32_t
pci_config_address(uint8_t bus, uint8_t device, uint8_t function,
                   uint8_t offset)
{
    return (1U << 31)                    // Enable bit
           | ((uint32_t) bus << 16)      // Bus number
           | ((uint32_t) device << 11)   // Device number
           | ((uint32_t) function << 8)  // Function number
           | ((uint32_t) offset & 0xFC); // Register offset (aligned to 4 bytes)
}

static uint32_t
pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t
pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t value = pci_config_read32(bus, device, function, offset & 0xFC);
    // Extract the correct 16 bits based on offset
    return (uint16_t) ((value >> ((offset & 2) * 8)) & 0xFFFF);
}

void
pci_config_write32(uint8_t bus, uint8_t device, uint8_t function,
                   uint8_t offset, uint32_t value)
{
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static int
pci_device_exists(uint8_t bus, uint8_t device, uint8_t function)
{
    uint16_t vendor_id =
        pci_config_read16(bus, device, function, PCI_VENDOR_ID);
    // Vendor ID 0xFFFF means no device
    return (vendor_id != 0xFFFF);
}

static int
find_device(platform_pci_device_t *dev)
{
    for (int bus = 0; bus < 4; bus++) {
        for (int device = 0; device < 32; device++) {
            for (int function = 0; function < 8; function++) {
                if (!pci_device_exists(bus, device, function)) {
                    continue;
                }

                uint16_t vendor_id =
                    pci_config_read16(bus, device, function, PCI_VENDOR_ID);
                uint16_t device_id =
                    pci_config_read16(bus, device, function, PCI_DEVICE_ID);

                // Check if this is any ATI device
                // (for now, accept any ATI card)
                if (vendor_id == ATI_VENDOR_ID) {
                    dev->bus = bus;
                    dev->device = device;
                    dev->function = function;
                    dev->vendor_id = vendor_id;
                    dev->device_id = device_id;
                    dev->bar[0] =
                        pci_config_read32(bus, device, function, PCI_BAR0);
                    dev->bar[2] =
                        pci_config_read32(bus, device, function, PCI_BAR2);
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Enable memory and I/O access for a PCI device
void
pci_enable_device(platform_pci_device_t *dev)
{
    uint16_t command =
        pci_config_read16(dev->bus, dev->device, dev->function, PCI_COMMAND);

    command |= PCI_COMMAND_MEMORY | PCI_COMMAND_IO | PCI_COMMAND_MASTER;

    uint32_t cmd_status =
        pci_config_read32(dev->bus, dev->device, dev->function, PCI_COMMAND);
    cmd_status = (cmd_status & 0xFFFF0000) | command;
    pci_config_write32(dev->bus, dev->device, dev->function, PCI_COMMAND,
                       cmd_status);
}

platform_pci_device_t *
platform_pci_init(void)
{
    if (!find_device(&g_pci_dev))
        FATAL;

    pci_enable_device(&g_pci_dev);

    return &g_pci_dev;
}

void
platform_pci_destroy(platform_pci_device_t *dev)
{
    // Noop on baremetal, we're using static allocation
    (void) dev;
    return;
}

void
platform_pci_get_name(platform_pci_device_t *dev, char *buf, size_t len)
{
    // TODO: Actually lookup the device name from a static table
    snprintf(buf, len, "%04x", dev->device_id);
}

void *
platform_pci_map_bar(platform_pci_device_t *dev, int bar_idx)
{
    return (void *) (uintptr_t) (dev->bar[bar_idx] & ~0xful);
}

void
platform_pci_unmap_bar(platform_pci_device_t *dev, void *addr, int bar_idx)
{
    // Noop on baremetal, we're dealing with physical addresses
    (void) dev;
    (void) addr;
    (void) bar_idx;
    return;
}

size_t
platform_pci_get_bar_size(platform_pci_device_t *dev, int bar_idx)
{
    uint8_t reg = PCI_BAR0 + (bar_idx * 4);
    uint32_t orig =
        pci_config_read32(dev->bus, dev->device, dev->function, reg);
    pci_config_write32(dev->bus, dev->device, dev->function, reg, 0xffffffff);
    uint32_t size_mask =
        pci_config_read32(dev->bus, dev->device, dev->function, reg);
    pci_config_write32(dev->bus, dev->device, dev->function, reg, orig);
    size_mask &= ~0xf;
    return ~size_mask + 1;
}

void *
platform_read_file(const char *path, size_t *size_out)
{
    // TODO: How to handle this? Possibly embed fixtures directly in the kernel?
    (void) path;
    *size_out = 0;
    return NULL;
}

void
platform_free_file(void *data)
{
    // TODO: How to handle this? Possibly embed fixtures directly
    //       in the elf file?
    (void) data;
    return;
}

size_t
platform_write_file(const char *path, const void *data, size_t size)
{
    // TODO: How to handle this? Possibly send across serial?
    (void) path;
    (void) data;
    (void) size;
    return 0;
}
