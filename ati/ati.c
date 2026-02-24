/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdarg.h>

#include "../platform/platform.h"

#include "ati.h"
#include "cce.h"
#include "r128.h"
#include "r100.h"
#include "../tests/test.h"

#define NUM_BARS 8

// ============================================================================
// Chip Detection
// ============================================================================

// Device ID to chip family mapping
static const struct {
    uint16_t device_id;
    ati_chip_family_t family;
    const char *name;
} chip_id_table[] = {
    // Rage 128 Pro
    { 0x5046, CHIP_R128, "Rage 128 Pro PF" },
    { 0x5050, CHIP_R128, "Rage 128 Pro PP" },
    { 0x5052, CHIP_R128, "Rage 128 Pro PR" },
    { 0x5245, CHIP_R128, "Rage 128 RE" },
    { 0x5246, CHIP_R128, "Rage 128 RF" },
    { 0x5247, CHIP_R128, "Rage 128 RG" },
    { 0x524B, CHIP_R128, "Rage 128 RK" },
    { 0x524C, CHIP_R128, "Rage 128 RL" },
    { 0x5345, CHIP_R128, "Rage 128 SE" },
    { 0x5346, CHIP_R128, "Rage 128 SF" },
    { 0x5347, CHIP_R128, "Rage 128 SG" },
    { 0x5348, CHIP_R128, "Rage 128 SH" },
    { 0x534B, CHIP_R128, "Rage 128 SK" },
    { 0x534C, CHIP_R128, "Rage 128 SL" },
    { 0x534D, CHIP_R128, "Rage 128 SM" },
    { 0x534E, CHIP_R128, "Rage 128 SN" },
    { 0x5446, CHIP_R128, "Rage 128 PRO Ultra AGP 4x" },
    // Radeon R100 / RV100 / M6
    { 0x5144, CHIP_R100, "Radeon QD" },
    { 0x5145, CHIP_R100, "Radeon QE" },
    { 0x5146, CHIP_R100, "Radeon QF" },
    { 0x5147, CHIP_R100, "Radeon QG" },
    { 0x5159, CHIP_R100, "Radeon QY (RV100)" },
    { 0x515A, CHIP_R100, "Radeon QZ (RV100)" },
    { 0x4C59, CHIP_R100, "Mobility M6 LY" },
    { 0x4C5A, CHIP_R100, "Mobility M6 LZ" },
    { 0, CHIP_UNKNOWN, NULL }
};

static ati_chip_family_t
detect_chip_family(uint16_t device_id)
{
    for (int i = 0; chip_id_table[i].name != NULL; i++) {
        if (chip_id_table[i].device_id == device_id) {
            return chip_id_table[i].family;
        }
    }
    return CHIP_UNKNOWN;
}

static const char *
get_chip_name(uint16_t device_id)
{
    for (int i = 0; chip_id_table[i].name != NULL; i++) {
        if (chip_id_table[i].device_id == device_id) {
            return chip_id_table[i].name;
        }
    }
    return "Unknown ATI Device";
}

const char *
ati_chip_family_name(ati_chip_family_t family)
{
    switch (family) {
    case CHIP_R128:
        return "R128";
    case CHIP_R100:
        return "R100";
    default:
        return "Unknown";
    }
}

// ============================================================================
// Device Structure
// ============================================================================

struct ati_device {
    platform_pci_device_t *pci_dev;
    ati_chip_family_t family;
    uint16_t device_id;
    char name[256];
    void *bar[NUM_BARS];
};

ati_chip_family_t
ati_get_chip_family(const ati_device_t *dev)
{
    return dev->family;
}

// ============================================================================
// Device Lifecycle
// ============================================================================

ati_device_t *
ati_device_init(platform_pci_device_t *pci_dev)
{
    static ati_device_t ati_dev;
    ati_device_t *ati = &ati_dev;

    ati->pci_dev = pci_dev;
    ati->device_id = platform_pci_get_device_id(pci_dev);
    ati->family = detect_chip_family(ati->device_id);
    ati->bar[0] = platform_pci_map_bar(ati->pci_dev, 0);
    ati->bar[2] = platform_pci_map_bar(ati->pci_dev, 2);
    platform_pci_get_name(ati->pci_dev, ati->name, sizeof(ati->name));

    // Print device info
    const char *color;
    switch (ati->family) {
    case CHIP_R128:
        color = "\033[36m";  // cyan
        break;
    case CHIP_R100:
        color = "\033[35m";  // magenta
        break;
    default:
        color = "\033[31m";  // red
        break;
    }
    printf("Detected: %s%s\033[0m [%04x] (%s%s\033[0m family)\n",
           color, get_chip_name(ati->device_id),
           ati->device_id,
           color, ati_chip_family_name(ati->family));

    if (ati->family == CHIP_UNKNOWN) {
        printf("WARNING: Unknown chip family, behavior may be unpredictable\n");
    }

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

// ============================================================================
// Register and VRAM Access
// ============================================================================

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
ati_screen_async_compare_fixture(ati_device_t *dev, const char *fixture_name)
{
    size_t fixture_size;
    const uint8_t *fixture = platform_get_fixture(fixture_name, &fixture_size);

    if (!fixture) {
        fprintf(stderr, "Fixture '%s' not found\n", fixture_name);
        char path[256];
        snprintf(path, sizeof(path), "fixtures/%s.rle", fixture_name);
        ati_screen_dump(dev, path);
        return false;
    }

    size_t screen_size = 640 * 480 * 4;
    if (fixture_size != screen_size) {
        fprintf(stderr, "Fixture size mismatch: expected %zu, got %zu\n",
                screen_size, fixture_size);
        platform_free_fixture(fixture);
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
        ati_screen_dump(dev, "FAILED.rle");
    }
    platform_free_fixture(fixture);
    return match;
}

bool
ati_screen_compare_fixture(ati_device_t *dev, const char *fixture_name)
{
    ati_wait_for_idle(dev);
    return ati_screen_async_compare_fixture(dev, fixture_name);
}

void
ati_screen_clear(ati_device_t *dev, uint32_t color)
{
    size_t screen_size = 640 * 480 * 4;
    memset(dev->bar[0], color, screen_size);
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
    platform_write_file(filename, (void *) vram, vram_size);
}

void
ati_screen_dump(ati_device_t *dev, const char *filename)
{
    volatile uint32_t *vram = (volatile uint32_t *) dev->bar[0];
    size_t screen_size = 640 * 480 * 4;
    platform_write_file(filename, (void *) vram, screen_size);
}

void
ati_dump_mode(ati_device_t *dev)
{
    // Note: CRTC_PITCH is chip-specific but at the same offset (0x022c) for both
    DUMP_REGISTERS(dev, CRTC_H_TOTAL_DISP, CRTC_H_SYNC_STRT_WID,
                   CRTC_V_TOTAL_DISP, CRTC_V_SYNC_STRT_WID, CRTC_GEN_CNTL,
                   CRTC_EXT_CNTL, CRTC_OFFSET, CRTC_OFFSET_CNTL, R128_CRTC_PITCH,
                   DAC_CNTL);
}

void
ati_print_info(ati_device_t *dev)
{
    const char *color;
    switch (dev->family) {
    case CHIP_R128:
        color = "\033[36m";  // cyan
        break;
    case CHIP_R100:
        color = "\033[35m";  // magenta
        break;
    default:
        color = "\033[31m";  // red
        break;
    }

    size_t vram_size = platform_pci_get_bar_size(dev->pci_dev, 0);
    size_t mmio_size = platform_pci_get_bar_size(dev->pci_dev, 2);

    printf("=== ATI Device ===\n");
    printf("Name:    %s%s\033[0m\n", color, get_chip_name(dev->device_id));
    printf("ID:      0x%04x\n", dev->device_id);
    printf("Family:  %s%s\033[0m\n", color, ati_chip_family_name(dev->family));
    printf("VRAM:    %p (%zu MB)\n", dev->bar[0], vram_size / (1024 * 1024));
    printf("MMIO:    %p (%zu KB)\n", dev->bar[2], mmio_size / 1024);
}

// ============================================================================
// Register Accessor Functions - Common Registers
// ============================================================================

/* Reads for common registers */
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    uint32_t rd_##func_name(ati_device_t *dev)                                 \
    {                                                                          \
        return ati_reg_read(dev, const_name);                                  \
    }
COMMON_REGISTERS
#undef X

/* Writes for common registers */
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    void wr_##func_name(ati_device_t *dev, uint32_t val)                       \
    {                                                                          \
        ati_reg_write(dev, const_name, val);                                   \
    }
COMMON_REGISTERS
#undef X

// ============================================================================
// Register Accessor Functions - R128-Specific Registers
// ============================================================================

#ifdef R128_REGISTERS
/* Reads for R128-specific registers (with chip guard) */
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    uint32_t rd_##func_name(ati_device_t *dev)                                 \
    {                                                                          \
        if (dev->family != CHIP_R128) {                                        \
            printf("ERROR: " #func_name " is R128-only (current: %s)\n",       \
                   ati_chip_family_name(dev->family));                         \
            return 0;                                                          \
        }                                                                      \
        return ati_reg_read(dev, const_name);                                  \
    }
R128_REGISTERS
#undef X

/* Writes for R128-specific registers (with chip guard) */
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    void wr_##func_name(ati_device_t *dev, uint32_t val)                       \
    {                                                                          \
        if (dev->family != CHIP_R128) {                                        \
            printf("ERROR: " #func_name " is R128-only (current: %s)\n",       \
                   ati_chip_family_name(dev->family));                         \
            return;                                                            \
        }                                                                      \
        ati_reg_write(dev, const_name, val);                                   \
    }
R128_REGISTERS
#undef X
#endif

// ============================================================================
// Register Accessor Functions - R100-Specific Registers
// ============================================================================

#ifdef R100_REGISTERS
/* Reads for R100-specific registers (with chip guard) */
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    uint32_t rd_##func_name(ati_device_t *dev)                                 \
    {                                                                          \
        if (dev->family != CHIP_R100) {                                        \
            printf("ERROR: " #func_name " is R100-only (current: %s)\n",       \
                   ati_chip_family_name(dev->family));                         \
            return 0;                                                          \
        }                                                                      \
        return ati_reg_read(dev, const_name);                                  \
    }
R100_REGISTERS
#undef X

/* Writes for R100-specific registers (with chip guard) */
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    void wr_##func_name(ati_device_t *dev, uint32_t val)                       \
    {                                                                          \
        if (dev->family != CHIP_R100) {                                        \
            printf("ERROR: " #func_name " is R100-only (current: %s)\n",       \
                   ati_chip_family_name(dev->family));                         \
            return;                                                            \
        }                                                                      \
        ati_reg_write(dev, const_name, val);                                   \
    }
R100_REGISTERS
#undef X
#endif

// ============================================================================
// Register Dump Functions
// ============================================================================

void
ati_dump_registers(ati_device_t *dev, int count, ...)
{
    va_list args;
    printf("\n============== Register State ==============\n");
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        uint32_t offset = va_arg(args, uint32_t);
        bool found = false;

#define X(func_name, const_name, offset_val, flags, fields, aliases)           \
    if (offset_val == offset) {                                                \
        if ((flags) & FLAG_NO_READ) {                                          \
            printf(" %-30s " RED "[write-only]\n" RESET, #const_name ":");     \
        } else {                                                               \
            printf(" %-30s " YELLOW "0x%08x\n" RESET, #const_name ":",         \
                   rd_##func_name(dev));                                       \
        }                                                                      \
        found = true;                                                          \
    }

        COMMON_REGISTERS

#undef X

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

#define X(func_name, const_name, offset, flags, fields, aliases)               \
    if ((flags) & FLAG_NO_READ) {                                              \
        printf(" %-30s " RED "[write-only]\n" RESET, #const_name ":");         \
    } else {                                                                   \
        printf(" %-30s " YELLOW "0x%08x\n" RESET, #const_name ":",             \
               rd_##func_name(dev));                                           \
    }

    COMMON_REGISTERS
#undef X
    printf("============================================\n");
}

// ============================================================================
// Display Mode Setup
// ============================================================================

void
ati_set_display_mode(ati_device_t *dev)
{
    switch (dev->family) {
    case CHIP_R128:
        r128_set_display_mode(dev);
        break;
    case CHIP_R100:
        r100_set_display_mode(dev);
        break;
    default:
        r128_set_display_mode(dev);
        break;
    }
}

// ============================================================================
// GUI Engine Control
// ============================================================================

void
ati_init_gui_engine(ati_device_t *dev)
{
    // Disable 3D scaling
    wr_scale_3d_cntl(dev, 0x0);

    // Reset the engine
    ati_engine_reset(dev);

    // Wait for engine to be idle after reset
    ati_wait_for_idle(dev);

    // Set default offset and pitch - chip-specific register layouts
    if (dev->family == CHIP_R100) {
        // R100: Combined register with pitch in bits 29:22, offset in bits 21:0
        // Pitch is in 64-byte units: (640 * 4) / 64 = 40
        // Offset is in 1KB units: 0
        uint32_t pitch_64 = (X_RES * BYPP) / 64;
        wr_r100_default_pitch_offset(dev, pitch_64 << 22);
    } else {
        // R128 or unknown: Separate registers
        wr_r128_default_offset(dev, 0x0);
        wr_r128_default_pitch(dev, X_RES / 8);
    }

    // Disable auxiliary scissor
    wr_aux_sc_cntl(dev, 0x0);

    // Set scissor clipping to the max.
    // Range is -8192 to +8191 which is why these aren't just set to the mask.
    wr_default_sc_bottom_right(dev, (0x1fff << DEFAULT_SC_RIGHT_SHIFT) |
                                        (0x1fff << DEFAULT_SC_BOTTOM_SHIFT));
    // The docs say to set DEFAULT_SC_TOP_LEFT... That doesn't exist as far as I
    // can tell. So, set the actual scissor top left just to be safe.
    wr_sc_top_left(dev, 0x00000000);
    wr_sc_bottom_right(dev, (0x1fff << DEFAULT_SC_RIGHT_SHIFT) |
                                (0x1fff << DEFAULT_SC_BOTTOM_SHIFT));

    // Set blit direction to left-to-right, top-to-bottom
    wr_dp_cntl(dev, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);

    // Set GUI master control
    wr_dp_gui_master_cntl(
        dev, GMC_BRUSH_DATATYPE_SOLIDCOLOR |
                 ati_get_dst_datatype(BPP) |
                 GMC_SRC_DATATYPE_DST_COLOR |
                 GMC_BYTE_PIX_ORDER | // LSB to MSB
                 GMC_ROP3_SRCCOPY |
                 GMC_SRC_SOURCE_MEMORY |
                 GMC_CLR_CMP_CNTL_DIS | GMC_AUX_CLIP_DIS | GMC_WR_MSK_DIS);

    // Clear the line drawing registers
    wr_dst_bres_err(dev, 0);
    wr_dst_bres_inc(dev, 0);
    wr_dst_bres_dec(dev, 0);

    // Set brush colors
    wr_dp_brush_frgd_clr(dev, 0xffffffff);
    wr_dp_brush_bkgd_clr(dev, 0x00000000);

    // Set source colors
    wr_dp_src_frgd_clr(dev, 0xffffffff);
    wr_dp_src_bkgd_clr(dev, 0x00000000);

    // Set write mask
    wr_dp_write_msk(dev, 0xffffffff);

    // Wait for idle to ensure initialization is complete
    ati_wait_for_idle(dev);
}

void
ati_engine_flush(ati_device_t *dev)
{
    // Flush the pixel cache (write 0xff to flush all)
    wr_pc_ngui_ctlstat(dev, PC_FLUSH_ALL_MASK);

    // Wait for flush to complete (PC_BUSY bit to clear)
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t status = rd_pc_ngui_ctlstat(dev);
        if ((status & PC_BUSY) == 0) {
            return;
        }
    }
    printf("ati_engine_flush timed out! Pixel cache still busy.\n");
}

void
ati_engine_reset(ati_device_t *dev)
{
    // Flush pixel cache first
    ati_engine_flush(dev);

    // Read current GEN_RESET_CNTL value
    uint32_t gen_reset_cntl = rd_gen_reset_cntl(dev);

    // Assert soft reset
    wr_gen_reset_cntl(dev, gen_reset_cntl | SOFT_RESET_GUI);
    // Read back to ensure write completes
    rd_gen_reset_cntl(dev);

    // Deassert soft reset
    wr_gen_reset_cntl(dev, gen_reset_cntl & ~SOFT_RESET_GUI);
    // Read back to ensure write completes
    rd_gen_reset_cntl(dev);

    // Restore original value
    wr_gen_reset_cntl(dev, gen_reset_cntl);
}

void
ati_reset_for_test(ati_device_t *dev)
{
    // Stop CCE engine if it was running (restores standard PIO mode)
    ati_stop_cce_engine(dev);

    ati_engine_reset(dev);
    ati_wait_for_idle(dev);
    ati_screen_clear(dev, 0);
    ati_init_gui_engine(dev);
}

void
ati_wait_for_reg_value(ati_device_t *dev, uint32_t reg, uint32_t value)
{
    uint32_t prev = ati_reg_read(dev, reg);
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t next = ati_reg_read(dev, reg);
        if (next != prev) {
            printf("0x%x => 0x%x\n", prev, next);
        }
        if (next == value) {
            return;
        }
    }
    // Timed out
    printf("ati_wait_for_value timed out! (waiting for 0x%x on reg 0x%x)\n", value, reg);
}

void
ati_wait_for_fifo(ati_device_t *dev, uint32_t entries)
{
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t slots = rd_gui_stat(dev) & GUI_FIFO_CNT_MASK;
        if (slots >= entries) {
            return;
        }
    }
    printf("ati_wait_for_fifo timed out! (waiting for %d entries)\n", entries);
    // TODO: I'm not sure what should happen on a timeout here.
    //       It looks like the r128 driver resets the engine.
    //       We'll see if we can get away with not worrying about it...
}

void
ati_wait_for_idle(ati_device_t *dev)
{
    // Wait for FIFO to be completely empty
    ati_wait_for_fifo(dev, FIFO_MAX);

    // Wait for engine to be idle
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t status = rd_gui_stat(dev);
        if ((status & GUI_ACTIVE) == 0) {
            break;
        }
    }
    if (timeout == 0) {
        printf("ati_wait_for_idle timed out! GUI still active.\n");
    }

    // Flush pixel cache
    ati_engine_flush(dev);
}
