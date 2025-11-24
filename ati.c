/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ati.h"
#include "common.h"
#include "platform/platform.h"

#define NUM_BARS 8

struct ati_device {
    platform_pci_device_t *pci_dev;
    char name[256];
    void *bar[NUM_BARS];
};

ati_device_t *
ati_device_init(void)
{
    ati_device_t *ati = malloc(sizeof(ati_device_t));
    ati->pci_dev = platform_pci_init();
    ati->bar[0] = platform_pci_map_bar(ati->pci_dev, 0);
    ati->bar[2] = platform_pci_map_bar(ati->pci_dev, 2);
    platform_pci_get_name(ati->pci_dev, ati->name, sizeof(ati->name));
    return ati;
}

void
ati_device_destroy(ati_device_t *dev)
{
    if (!dev)
        return;
    for (int i = 0; i < NUM_BARS; i++) {
        if (dev->bar[i])
            platform_pci_unmap_bar(dev->pci_dev, dev->bar[i], i);
    }
    platform_pci_destroy(dev->pci_dev);
}

static inline uint32_t
reg_read(void *base, uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *) ((char *) base + offset);
    return *reg;
}

static inline void
reg_write(void *base, uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *) ((char *) base + offset);
    *reg = value;
}

uint32_t
ati_reg_read(ati_device_t *dev, uint32_t offset)
{
    return reg_read(dev->bar[2], offset);
}

void
ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value)
{
    reg_write(dev->bar[2], offset, value);
}

uint32_t
ati_vram_read(ati_device_t *dev, uint32_t offset)
{
    return reg_read(dev->bar[0], offset);
}

void
ati_vram_write(ati_device_t *dev, uint32_t offset, uint32_t value)
{
    reg_write(dev->bar[0], offset, value);
}

uint64_t
ati_vram_search(ati_device_t *dev, uint32_t needle)
{
    volatile uint32_t *vram = (volatile uint32_t *) dev->bar[0];
    size_t vram_size = platform_pci_get_bar_size(dev->pci_dev, 0);
    size_t dwords = vram_size / 4;

    for (size_t i = 0; i < dwords; i++) {
        if (vram[i] == needle) {
            return (uint64_t) i * 4;
        }
    }

    return VRAM_NOT_FOUND;
}

void
ati_vram_memcpy(ati_device_t *dev, uint32_t dst_offset, const void *src,
                size_t size)
{
    size_t vram_size = platform_pci_get_bar_size(dev->pci_dev, 0);
    if (size > vram_size) {
        printf("Copying data larger than BAR0\n");
    }

    memcpy(dev->bar[0] + dst_offset, src, size);
}

bool
ati_screen_compare_file(ati_device_t *dev, const char *filename)
{
start: {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        if (errno == ENOENT) {
            printf("Fixture does not yet exist. Create it now? (Y/n) ");
            fflush(stdout);

            char response[10];
            if (fgets(response, sizeof(response), stdin)) {
                if (response[0] == 'Y' || response[0] == 'y' ||
                    response[0] == '\n') {
                    ati_screen_dump(dev, filename);
                    goto start;
                } else {
                    return false;
                }
            }
        }
        fprintf(stderr, "Failed to open fixture %s: %s\n", filename,
                strerror(errno));
        return false;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t screen_size = 640 * 480 * 4;
    if (file_size != screen_size) {
        fprintf(stderr, "Reference file size mismatch: expected %zu, got %zu\n",
                screen_size, file_size);
        fclose(f);
        return false;
    }

    uint8_t *fixture = malloc(screen_size);
    if (!fixture) {
        fprintf(stderr, "Failed to allocate memory for fixture\n");
        fclose(f);
        return false;
    }
    size_t read = fread(fixture, 1, screen_size, f);
    fclose(f);
    if (read != screen_size) {
        fprintf(stderr, "Failed to read complete fixture file\n");
        free(fixture);
        return false;
    }

    // Compare with current framebuffer
    volatile uint8_t *vram = (volatile uint8_t *) dev->bar[0];
    bool match = true;
    int first_mismatch = -1;
    int mismatch_count = 0;
    for (size_t i = 0; i < screen_size; i++) {
        if (vram[i] != fixture[i]) {
            if (first_mismatch == -1) {
                first_mismatch = i;
            }
            mismatch_count++;
        }
    }
    if (mismatch_count > 0) {
        match = false;
        printf("MISMATCH: %d bytes differ\n", mismatch_count);
        if (first_mismatch >= 0) {
            printf("First mismatch at byte offset 0x%x:\n", first_mismatch);
            printf("  Expected: 0x%02x\n", fixture[first_mismatch]);
            printf("  Got:      0x%02x\n", vram[first_mismatch]);

            int pixel_offset = first_mismatch / 4;
            int y = pixel_offset / 640;
            int x = pixel_offset % 640;
            printf("  Pixel at (%d, %d)\n", x, y);
        }
    }
    free(fixture);
    return match;
}
}

void
ati_screen_clear(ati_device_t *dev)
{
    size_t screen_size = 640 * 480 * 4;
    memset(dev->bar[0], 0, screen_size);
}

void
ati_vram_clear(ati_device_t *dev)
{
    size_t vram_size = platform_pci_get_bar_size(dev->pci_dev, 0);
    memset(dev->bar[0], 0, vram_size);
}

void
ati_vram_dump(ati_device_t *dev, const char *filename)
{
    volatile uint32_t *vram = (volatile uint32_t *) dev->bar[0];
    size_t vram_size = platform_pci_get_bar_size(dev->pci_dev, 0);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", filename,
                strerror(errno));
    }

    size_t written = fwrite((void *) vram, 1, vram_size, f);

    if (written != vram_size) {
        fprintf(stderr, "Warning: only wrote %zu of %zu bytes\n", written,
                vram_size);
    }
    fclose(f);
    printf("Dumped %zu bytes of VRAM to %s\n", written, filename);
}

void
ati_screen_dump(ati_device_t *dev, const char *filename)
{
    volatile uint32_t *vram = (volatile uint32_t *) dev->bar[0];
    size_t screen_size = 640 * 480 * 4;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", filename,
                strerror(errno));
        return;
    }

    size_t written = fwrite((void *) vram, 1, screen_size, f);

    if (written != screen_size) {
        fprintf(stderr, "Warning: only wrote %zu of %zu bytes\n", written,
                screen_size);
    }
    fclose(f);
    printf("Dumped %zu bytes of screen to %s\n", written, filename);
}

void
ati_dump_mode(ati_device_t *dev)
{
    DUMP_REGISTERS(dev, CRTC_H_TOTAL_DISP, CRTC_H_SYNC_STRT_WID,
                   CRTC_V_TOTAL_DISP, CRTC_V_SYNC_STRT_WID, CRTC_GEN_CNTL,
                   CRTC_EXT_CNTL, CRTC_OFFSET, CRTC_OFFSET_CNTL, CRTC_PITCH,
                   DAC_CNTL);
}

/* Register accessor functions */

/* Reads */
#define X(func_name, const_name, offset, mode)                                 \
    X_##mode##_READ_IMPL(func_name, const_name)

#define X_RW_READ_IMPL(func_name, const_name)                                  \
    uint32_t rd_##func_name(ati_device_t *dev)                                 \
    {                                                                          \
        return ati_reg_read(dev, const_name);                                  \
    }

#define X_RO_READ_IMPL(func_name, const_name)                                  \
    uint32_t rd_##func_name(ati_device_t *dev)                                 \
    {                                                                          \
        return ati_reg_read(dev, const_name);                                  \
    }

#define X_WO_READ_IMPL(func_name, const_name) /* No read for write-only */

ATI_REGISTERS
#undef X
#undef X_RW_READ_IMPL
#undef X_RO_READ_IMPL
#undef X_WO_READ_IMPL

/* Writes */
#define X(func_name, const_name, offset, mode)                                 \
    X_##mode##_WRITE_IMPL(func_name, const_name)

#define X_RW_WRITE_IMPL(func_name, const_name)                                 \
    void wr_##func_name(ati_device_t *dev, uint32_t val)                       \
    {                                                                          \
        ati_reg_write(dev, const_name, val);                                   \
    }
#define X_WO_WRITE_IMPL(func_name, const_name)                                 \
    void wr_##func_name(ati_device_t *dev, uint32_t val)                       \
    {                                                                          \
        ati_reg_write(dev, const_name, val);                                   \
    }
#define X_RO_WRITE_IMPL(func_name, const_name) /* No write for read-only */

ATI_REGISTERS
#undef X
#undef X_RW_WRITE_IMPL
#undef X_WO_WRITE_IMPL
#undef X_RO_WRITE_IMPL

void
ati_dump_registers(ati_device_t *dev, int count, ...)
{
    va_list args;
    printf("\n============== Register State ==============\n");
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        uint32_t offset = va_arg(args, uint32_t);
        bool found = false;

#define X(func_name, const_name, offset_val, mode)                             \
    X_##mode##_DUMP_IF(func_name, const_name, offset_val, offset, &found)

#define X_RW_DUMP_IF(func_name, const_name, offset_val, target, found_ptr)     \
    if (offset_val == target) {                                                \
        printf(" %-30s " YELLOW "0x%08x\n" RESET, #const_name ":",             \
               rd_##func_name(dev));                                           \
        *found_ptr = true;                                                     \
    }

#define X_RO_DUMP_IF(func_name, const_name, offset_val, target, found_ptr)     \
    if (offset_val == target) {                                                \
        printf(" %-30s " YELLOW "0x%08x\n" RESET, #const_name ":",             \
               rd_##func_name(dev));                                           \
        *found_ptr = true;                                                     \
    }

#define X_WO_DUMP_IF(func_name, const_name, offset_val, target, found_ptr)     \
    if (offset_val == target) {                                                \
        printf(" %-30s " RED "[write-only]\n" RESET, #const_name ":");         \
        *found_ptr = true;                                                     \
    }

        ATI_REGISTERS

#undef X
#undef X_RW_DUMP_IF
#undef X_RO_DUMP_IF
#undef X_WO_DUMP_IF

        if (!found) {
            printf("0x%08x:                   [unknown register]\n", offset);
        }
    }

    va_end(args);
    printf("============================================\n");
}

void
ati_dump_all_registers(ati_device_t *dev)
{
    printf("\n============== Register State ==============\n");

#define X(func_name, const_name, offset, mode)                                 \
    X_##mode##_DUMP(func_name, const_name)

#define X_RW_DUMP(func_name, const_name)                                       \
    printf(" %-30s " YELLOW "0x%08x\n" RESET, #const_name ":",                 \
           rd_##func_name(dev));

#define X_RO_DUMP(func_name, const_name)                                       \
    printf(" %-30s " YELLOW "0x%08x\n" RESET, #const_name ":",                 \
           rd_##func_name(dev));

#define X_WO_DUMP(func_name, const_name)                                       \
    printf(" %-30s " RED "[write-only]\n" RESET, #const_name ":");

    ATI_REGISTERS
#undef X
#undef X_RW_DUMP
#undef X_RO_DUMP
#undef X_WO_DUMP
    printf("============================================\n");
}
