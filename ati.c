/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdarg.h>

#include "platform/platform.h"

#include "ati.h"
#include "common.h"

#define NUM_BARS 8

struct ati_device {
    platform_pci_device_t *pci_dev;
    char name[256];
    void *bar[NUM_BARS];
};

ati_device_t *
ati_device_init(platform_pci_device_t *pci_dev)
{
    static ati_device_t ati_dev;
    ati_device_t *ati = &ati_dev;
    ati->pci_dev = pci_dev;
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
ati_screen_async_compare_fixture(ati_device_t *dev, const char *fixture_name)
{
    size_t fixture_size;
    const uint8_t *fixture = platform_get_fixture(fixture_name, &fixture_size);

    if (!fixture) {
        fprintf(stderr, "Fixture '%s' not found\n", fixture_name);
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
    size_t written = platform_write_file(filename, (void *) vram, vram_size);
    printf("Dumped %zu bytes of VRAM to %s\n", written, filename);
}

void
ati_screen_dump(ati_device_t *dev, const char *filename)
{
    volatile uint32_t *vram = (volatile uint32_t *) dev->bar[0];
    size_t screen_size = 640 * 480 * 4;
    size_t written = platform_write_file(filename, (void *) vram, screen_size);
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

void
ati_set_display_mode(ati_device_t *dev)
{
    // Disable display while programming CRTC registers
    uint32_t crtc_ext_cntl = rd_crtc_ext_cntl(dev);
    wr_crtc_ext_cntl(dev, crtc_ext_cntl | CRTC_HSYNC_DIS | CRTC_VSYNC_DIS |
                              CRTC_DISPLAY_DIS | VGA_ATI_LINEAR |
                              VGA_XCRT_CNT_EN | CRTC_CRT_ON);

    // Clear common registers that could interfere with CRTC settings
    wr_ovr_clr(dev, 0x0);
    wr_ovr_wid_left_right(dev, 0x0);
    wr_ovr_wid_top_bottom(dev, 0x0);
    wr_ov0_scale_cntl(dev, 0x0);
    wr_mpp_tb_config(dev, 0x0);
    wr_mpp_gp_config(dev, 0x0);
    wr_subpic_cntl(dev, 0x0);
    wr_viph_control(dev, 0x0);
    wr_i2c_cntl_1(dev, 0x0);
    wr_gen_int_cntl(dev, 0x0);
    wr_cap0_trig_cntl(dev, 0x0);
    wr_cap1_trig_cntl(dev, 0x0);

    uint32_t dac_cntl = rd_dac_cntl(dev);
    wr_dac_cntl(dev,
                (dac_cntl & ~DAC_TVO_EN & ~DAC_VGA_ADR_EN & ~DAC_MASK_MASK) |
                    DAC_8BIT_EN | (0xff << DAC_MASK_SHIFT));

    // Enable acceleration
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    wr_crtc_gen_cntl(dev, (crtc_gen_cntl & ~CRTC_PIX_WIDTH_MASK & ~CRTC_CUR_EN &
                           ~CRTC_C_SYNC_EN) |
                              CRTC_EXT_DISP_EN | CRTC_EN |
                              (CRTC_PIX_WIDTH_32BPP << CRTC_PIX_WIDTH_SHIFT));

    // VESA 640x480@60Hz standard timings
    // Horizontal: 640 visible + 16 front porch + 96 sync + 48 back porch = 800
    // total Vertical: 480 visible + 10 front porch + 2 sync + 33 back porch =
    // 525 total Pixel clock: 25.175 MHz

    // Horizontal timing (in 8-pixel characters)
    uint32_t h_disp = ((640 / 8) - 1);  // 79 (640 pixels / 8 - 1)
    uint32_t h_total = ((800 / 8) - 1); // 99 (800 pixels / 8 - 1)
    wr_crtc_h_total_disp(dev, (h_disp << CRTC_H_DISP_SHIFT) |
                                  (h_total << CRTC_H_TOTAL_SHIFT));

    // Horizontal sync: starts at 640 + 16 = 656, width = 96 pixels
    uint32_t h_sync_strt = (640 + 16) / 8; // 82 characters
    uint32_t h_sync_wid = 96 / 8;          // 12 characters
    // Negative sync polarity for 640x480@60Hz
    wr_crtc_h_sync_strt_wid(dev, (h_sync_strt << CRTC_H_SYNC_STRT_CHAR_SHIFT) |
                                     (h_sync_wid << CRTC_H_SYNC_WID_SHIFT) |
                                     CRTC_H_SYNC_POL);

    // Vertical timing (in lines)
    uint32_t v_disp = 480 - 1;  // 479
    uint32_t v_total = 525 - 1; // 524
    wr_crtc_v_total_disp(dev, (v_disp << CRTC_V_DISP_SHIFT) |
                                  (v_total << CRTC_V_TOTAL_SHIFT));

    // Vertical sync: starts at 480 + 10 = 490, width = 2 lines
    uint32_t v_sync_strt = 480 + 10; // 490
    uint32_t v_sync_wid = 2;         // 2 lines
    // Negative sync polarity for 640x480@60Hz
    wr_crtc_v_sync_strt_wid(dev, (v_sync_strt << CRTC_V_SYNC_STRT_SHIFT) |
                                     (v_sync_wid << CRTC_V_SYNC_WID_SHIFT) |
                                     CRTC_V_SYNC_POL);

    wr_crtc_offset(dev, 0x0);
    wr_crtc_offset_cntl(dev, 0x0);
    wr_crtc_pitch(dev, X_RES / 8);

    // Program DDA registers for display FIFO arbitration
    // These values are calculated for:
    //   XCLK = 134 MHz (memory clock from BIOS)
    //   VCLK = 67 MHz (BIOS-configured pixel clock, not 25MHz)
    //   32bpp, 640x480, FIFO depth=32, FIFO width=128 bits
    // Using Linux's known-working values for now:
    wr_dda_config(dev, 0x01060220);
    wr_dda_on_off(dev, 0x05e03b80);

    // Initialize linear palette for 32bpp gamma correction
    // In 32bpp mode, each color component (R,G,B) is looked up through
    // the palette. Entry N should map to (N,N,N) for correct colors.
    wr_palette_index(dev, 0); // Start at entry 0
    for (int i = 0; i < 256; i++) {
        uint32_t val = (i << 16) | (i << 8) | i; // R=G=B=i
        wr_palette_data(dev, val);
    }

    // Re-enable the display
    crtc_ext_cntl = rd_crtc_ext_cntl(dev);
    wr_crtc_ext_cntl(dev, (crtc_ext_cntl & ~CRTC_HSYNC_DIS & ~CRTC_VSYNC_DIS &
                           ~CRTC_DISPLAY_DIS) |
                              VGA_ATI_LINEAR | VGA_XCRT_CNT_EN | CRTC_CRT_ON);
}

void
ati_init_gui_engine(ati_device_t *dev)
{
    // Disable 3D scaling
    wr_scale_3d_cntl(dev, 0x0);

    // Reset the engine
    ati_engine_reset(dev);

    // Wait for engine to be idle after reset
    ati_wait_for_idle(dev);

    // Set default offset and pitch
    wr_default_offset(dev, 0x0);
    wr_default_pitch(dev, X_RES / 8);

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
        dev, 0x0 | (BRUSH_SOLIDCOLOR << GMC_BRUSH_DATATYPE_SHIFT) |
                 (ati_get_dst_datatype(BPP) << GMC_DST_DATATYPE_SHIFT) |
                 (SRC_DST_COLOR << GMC_SRC_DATATYPE_SHIFT) |
                 GMC_BYTE_PIX_ORDER | // LSB to MSB
                 (ROP3_SRCCOPY << GMC_ROP3_SHIFT) |
                 (SOURCE_MEMORY << GMC_SRC_SOURCE_SHIFT) |
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
    // Flush the pixel cache
    wr_pc_ngui_ctlstat(dev, PC_FLUSH_ALL);

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
    ati_engine_reset(dev);
    ati_wait_for_idle(dev);
    ati_screen_clear(dev);
    ati_init_gui_engine(dev);
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

/* CCE microcode (from ATI) */
static uint32_t r128_cce_microcode[] = {
    0,  276838400,   0,  268449792,  2,  142,         2,  145,
    0,  1076765731,  0,  1617039951, 0,  774592877,   0,  1987540286,
    0,  2307490946U, 0,  599558925,  0,  589505315,   0,  596487092,
    0,  589505315,   1,  11544576,   1,  206848,      1,  311296,
    1,  198656,      2,  912273422,  11, 262144,      0,  0,
    1,  33559837,    1,  7438,       1,  14809,       1,  6615,
    12, 28,          1,  6614,       12, 28,          2,  23,
    11, 18874368,    0,  16790922,   1,  409600,      9,  30,
    1,  147854772,   16, 420483072,  3,  8192,        0,  10240,
    1,  198656,      1,  15630,      1,  51200,       10, 34858,
    9,  42,          1,  33559823,   2,  10276,       1,  15717,
    1,  15718,       2,  43,         1,  15936948,    1,  570480831,
    1,  14715071,    12, 322123831,  1,  33953125,    12, 55,
    1,  33559908,    1,  15718,      2,  46,          4,  2099258,
    1,  526336,      1,  442623,     4,  4194365,     1,  509952,
    1,  459007,      3,  0,          12, 92,          2,  46,
    12, 176,         1,  15734,      1,  206848,      1,  18432,
    1,  133120,      1,  100670734,  1,  149504,      1,  165888,
    1,  15975928,    1,  1048576,    6,  3145806,     1,  15715,
    16, 2150645232U, 2,  268449859,  2,  10307,       12, 176,
    1,  15734,       1,  15735,      1,  15630,       1,  15631,
    1,  5253120,     6,  3145810,    16, 2150645232U, 1,  15864,
    2,  82,          1,  343310,     1,  1064207,     2,  3145813,
    1,  15728,       1,  7817,       1,  15729,       3,  15730,
    12, 92,          2,  98,         1,  16168,       1,  16167,
    1,  16002,       1,  16008,      1,  15974,       1,  15975,
    1,  15990,       1,  15976,      1,  15977,       1,  15980,
    0,  15981,       1,  10240,      1,  5253120,     1,  15720,
    1,  198656,      6,  110,        1,  180224,      1,  103824738,
    2,  112,         2,  3145839,    0,  536885440,   1,  114880,
    14, 125,         12, 206975,     1,  33559995,    12, 198784,
    0,  33570236,    1,  15803,      0,  15804,       3,  294912,
    1,  294912,      3,  442370,     1,  11544576,    0,  811612160,
    1,  12593152,    1,  11536384,   1,  14024704,    7,  310382726,
    0,  10240,       1,  14796,      1,  14797,       1,  14793,
    1,  14794,       0,  14795,      1,  268679168,   1,  9437184,
    1,  268449792,   1,  198656,     1,  9452827,     1,  1075854602,
    1,  1075854603,  1,  557056,     1,  114880,      14, 159,
    12, 198784,      1,  1109409213, 12, 198783,      1,  1107312059,
    12, 198784,      1,  1109409212, 2,  162,         1,  1075854781,
    1,  1073757627,  1,  1075854780, 1,  540672,      1,  10485760,
    6,  3145894,     16, 274741248,  9,  168,         3,  4194304,
    3,  4209949,     0,  0,          0,  256,         14, 174,
    1,  114857,      1,  33560007,   12, 176,         0,  10240,
    1,  114858,      1,  33560018,   1,  114857,      3,  33560007,
    1,  16008,       1,  114874,     1,  33560360,    1,  114875,
    1,  33560154,    0,  15963,      0,  256,         0,  4096,
    1,  409611,      9,  188,        0,  10240,       0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0,
    0,  0,           0,  0,          0,  0,           0,  0};

void
ati_init_cce_engine(ati_device_t *dev)
{
    ati_wait_for_idle(dev);
    // Load CCE microcode
    wr_pm4_microcode_addr(dev, 0);
    for (int i = 0; i < 256; i += 1) {
        wr_pm4_microcode_datah(dev, r128_cce_microcode[i * 2]);
        wr_pm4_microcode_datal(dev, r128_cce_microcode[i * 2 + 1]);
    }
    // Set to PIO-based CCE mode with 192 entries
    wr_pm4_buffer_cntl(dev, PM4_192PIO << PM4_BUFFER_MODE_SHIFT);
    // Start the CCE engine
    wr_pm4_micro_cntl(dev, PM4_MICRO_FREERUN);
}

void
ati_stop_cce_engine(ati_device_t *dev)
{
    ati_wait_for_idle(dev);
    // Stop the CCE engine
    wr_pm4_micro_cntl(dev, 0);
    // Disable CCE Mode
    wr_pm4_buffer_cntl(dev, PM4_NONPM4 << PM4_BUFFER_MODE_SHIFT);
}
