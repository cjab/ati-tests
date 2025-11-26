/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stddef.h>
#include <stdint.h>

#include "platform.h"
#include "tinyprintf.h"

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
#define SERIAL_PORT 0x3f8

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
        while (1)                                                              \
            ;                                                                  \
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

static inline void
outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t
inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Serial
void
serial_init(void)
{
    outb(SERIAL_PORT + 1, 0x00); // Disable interrupts
    outb(SERIAL_PORT + 3, 0x80); // Enable DLAB
    outb(SERIAL_PORT + 0, 0x03); // Divisor low (38400 baud)
    outb(SERIAL_PORT + 1, 0x00); // Divisor high
    outb(SERIAL_PORT + 3, 0x03); // 8N1
    outb(SERIAL_PORT + 2, 0xC7); // Enable FIFO
}

void
serial_putc(void *p, char c)
{
    (void) p;
    while (!(inb(SERIAL_PORT + 5) & 0x20))
        ; // Wait for ready
    outb(SERIAL_PORT, c);
}

void
serial_puts(const char *s)
{
    while (*s) {
        serial_putc(NULL, *s++);
    }
}

// PCI
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

static uint16_t
pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t value = pci_config_read32(bus, device, function, offset & 0xFC);
    // Extract the correct 16 bits based on offset
    return (uint16_t) ((value >> ((offset & 2) * 8)) & 0xFFFF);
}

static void
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

static platform_pci_device_t *
platform_pci_init_internal(void)
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
    (void) dev;
    (void) len;
    // TODO: Actually lookup the device name from a static table
    if (len >= 4) {
        buf[0] = 'A';
        buf[1] = 'T';
        buf[2] = 'I';
        buf[3] = '\0';
    }
}

void *
platform_pci_map_bar(platform_pci_device_t *dev, int bar_idx)
{
    // TODO: I think technically we should be masking these just like X.org
    //
    // From the Rage 128 programmers guide:
    //
    // """
    // The memory aperture base address value at offset 0x10 within the PCI
    // configuration space is in bits [31:26] of its DWORD. Therefore, to
    // isolate the proper bits, the value should be logically ANDed with
    // 0xFC000000.
    //
    // For the I/O base aperture, the actual value is within bits [31:8] of its
    // DWORD (at offset 0x14). Therefore, to isolate the proper bits, the value
    // should be logically ANDed with 0xFFFFFF00.
    //
    // The register aperture base value resides in bits [31:14] of its DWORD (at
    // offset 0x18). Therefore, to isolate the proper bits, the value should be
    // logically ANDed with 0xFFFFC000.
    // """
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

int
fprintf(FILE *stream, const char *format, ...)
{
    (void) stream;
    va_list va;
    va_start(va, format);
    tfp_format(NULL, serial_putc, format, va);
    va_end(va);

    return 0;
}

int
fflush(FILE *stream)
{
    // TODO: stubbed
    (void) stream;
    return 0;
}

void
exit(int status)
{
    (void) status;
    while (1) {
        __asm__ volatile("hlt");
    } // Halt CPU
}

void *
memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *
memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char) c;
    }
    return s;
}

int
strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *) s1 - *(unsigned char *) s2;
}

char *
fgets(char *s, int size, FILE *stream)
{
    // TODO: stubbed
    (void) s;
    (void) size;
    (void) stream;
    return NULL;
}

// Multiboot v1 information structure
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    // And more... See:
    // https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Boot-information-format
};

// Storage for parsed command line arguments
static char *g_argv[32];
static int g_argc;

// Simple tokenizer - splits on whitespace, null-terminates each token
static int
parse_cmdline(char *cmdline, char **argv, int max_args)
{
    int argc = 0;
    char *p = cmdline;

    while (*p && argc < max_args) {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (!*p) {
            break;
        }

        // Found start of argument
        argv[argc++] = p;

        // Find end of argument (next whitespace or null)
        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }

        // Null-terminate this argument
        if (*p) {
            *p++ = '\0';
        }
    }

    return argc;
}

platform_t *
platform_init(int argc, char **argv)
{
    static platform_t platform;

    // argc/argv are ignored on baremetal, we get args from multiboot
    (void) argc;
    (void) argv;

    serial_init();
    init_printf(NULL, serial_putc);

    // Args already parsed by platform_init_args() called from boot.S
    platform.argc = g_argc - 1; // Remove kernel name to match linux argc count
    platform.argv = g_argv;

    // Initialize PCI device
    platform.pci_dev = platform_pci_init_internal();

    return &platform;
}

void
platform_init_args(uint32_t magic, struct multiboot_info *mbi)
{
    g_argc = 0;

    if (magic != 0x2BADB002) {
        serial_puts("Invalid multiboot magic\n");
        return;
    }

    // Check if command line arguments were provided (bit 2 of flags)
    if ((mbi->flags & (1 << 2)) && mbi->cmdline) {
        char *cmdline = (char *) (uintptr_t) mbi->cmdline;
        g_argc = parse_cmdline(cmdline, g_argv, 32);
    }
}

void
platform_destroy(platform_t *platform)
{
    (void) platform;

    printf("\n\nTests complete. Halting.\n");

    // Halt the CPU
    while (1) {
        __asm__ volatile("hlt");
    }
}
