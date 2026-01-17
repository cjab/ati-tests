/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "cce_cmd.h"
#include "../ati/cce.h"
#include "../tests/test.h"
#include "repl.h"

typedef enum {
    CCE_CMD_INIT,
    CCE_CMD_START,
    CCE_CMD_STOP,
    CCE_CMD_MODE,
    CCE_CMD_RELOAD,
    CCE_CMD_DUMP,
    CCE_CMD_R,
    CCE_CMD_W,
    CCE_CMD_PAINT,
    CCE_CMD_STATMUX,
    CCE_CMD_STEP,
    CCE_CMD_UNKNOWN
} cce_cmd_t;

// Max rectangles for paint command
#define MAX_PAINT_RECTS 4
#define MAX_PAINT_DWORDS (4 + MAX_PAINT_RECTS * 2)

// clang-format off
static const struct {
    const char *name;
    cce_cmd_t cmd;
    const char *usage;
    const char *desc;
} cce_cmd_table[] = {
    {"init",    CCE_CMD_INIT,    NULL,              "full CCE init (load + mode + start)"},
    {"start",   CCE_CMD_START,   NULL,              "start microengine"},
    {"stop",    CCE_CMD_STOP,    NULL,              "stop microengine"},
    {"mode",    CCE_CMD_MODE,    NULL,              "set PM4 PIO mode (no microcode load)"},
    {"reload",  CCE_CMD_RELOAD,  NULL,              "load microcode (no start/stop)"},
    {"dump",    CCE_CMD_DUMP,    NULL,              "dump all 256 instructions"},
    {"r",       CCE_CMD_R,       "<addr> [count]",  "read instruction(s) (0-255)"},
    {"w",       CCE_CMD_W,       "<addr> <h> <l>",  "write instruction"},
    {"paint",   CCE_CMD_PAINT,   "<x y w h>... <color>", "paint rectangle(s) via CCE"},
    {"statmux", CCE_CMD_STATMUX, NULL,              "dump all 32 ME_STAT_MUX values"},
    {"step",    CCE_CMD_STEP,    "[n]",             "single-step n instructions (default 1)"},
    {NULL,      CCE_CMD_UNKNOWN, NULL,              NULL}
};
// clang-format on

static cce_cmd_t
lookup_cce_cmd(const char *name)
{
    for (int i = 0; cce_cmd_table[i].name != NULL; i++) {
        if (strcmp(name, cce_cmd_table[i].name) == 0)
            return cce_cmd_table[i].cmd;
    }
    return CCE_CMD_UNKNOWN;
}

// Hexyl-style byte coloring: gray for 0x00, yellow for non-zero
static void
print_byte(uint8_t b)
{
    if (b == 0x00)
        printf("\x1b[90m%02x\x1b[0m", b);
    else
        printf("\x1b[33m%02x\x1b[0m", b);
}

// Red for mismatched bytes
static void
print_byte_mismatch(uint8_t got, uint8_t expected)
{
    if (got == expected)
        print_byte(got);
    else
        printf("\x1b[31m%02x\x1b[0m", got);
}

// Print 32-bit word as 4 space-separated bytes
static void
print_word(uint32_t w)
{
    print_byte((w >> 24) & 0xff);
    printf(" ");
    print_byte((w >> 16) & 0xff);
    printf(" ");
    print_byte((w >> 8) & 0xff);
    printf(" ");
    print_byte(w & 0xff);
}

// Print word with per-byte mismatch highlighting
static void
print_word_mismatch(uint32_t got, uint32_t expected)
{
    print_byte_mismatch((got >> 24) & 0xff, (expected >> 24) & 0xff);
    printf(" ");
    print_byte_mismatch((got >> 16) & 0xff, (expected >> 16) & 0xff);
    printf(" ");
    print_byte_mismatch((got >> 8) & 0xff, (expected >> 8) & 0xff);
    printf(" ");
    print_byte_mismatch(got & 0xff, expected & 0xff);
}

// Print full 64-bit instruction (high + low words)
static void
print_instruction(uint32_t addr, uint32_t high, uint32_t low)
{
    printf("\x1b[90m[%03d]\x1b[0m ", addr);
    print_word(high);
    printf("  ");
    print_word(low);
    printf("\n");
}

// Subcommand handlers

static void
cce_init(ati_device_t *dev)
{
    ati_init_cce_engine(dev);
    printf("CCE initialized\n");
}

static void
cce_start(ati_device_t *dev)
{
    wr_pm4_micro_cntl(dev, PM4_MICRO_FREERUN);
    printf("CCE microengine started\n");
}

static void
cce_stop(ati_device_t *dev)
{
    ati_wait_for_idle(dev);
    wr_pm4_micro_cntl(dev, 0);
    printf("CCE microengine stopped\n");
}

static void
cce_mode(ati_device_t *dev)
{
    wr_pm4_buffer_cntl(dev, PM4_BUFFER_MODE_192PIO | PM4_BUFFER_CNTL_NOUPDATE);
    (void) rd_pm4_buffer_addr(dev);
    printf("PM4 PIO mode set\n");
}

static void
cce_reload(ati_device_t *dev)
{
    ati_wait_for_idle(dev);
    ati_cce_load_microcode(dev);
    printf("CCE microcode loaded\n");
}

static void
cce_dump(ati_device_t *dev)
{
    wr_pm4_microcode_raddr(dev, 0);
    for (uint32_t addr = 0; addr < 256; addr++) {
        uint32_t high = rd_pm4_microcode_datah(dev);
        uint32_t low = rd_pm4_microcode_datal(dev);
        print_instruction(addr, high, low);
    }
}

static void
cce_read(ati_device_t *dev, int argc, char **args)
{
    uint32_t addr, count = 1;

    if (argc < 3 || parse_int(args[2], &addr) != 0 || addr >= 256) {
        printf("Usage: cce r <addr> [count] (addr 0-255)\n");
        return;
    }

    if (argc >= 4)
        parse_int(args[3], &count);

    // Clamp count to not exceed 256 instructions
    if (addr + count > 256)
        count = 256 - addr;

    wr_pm4_microcode_raddr(dev, addr);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t high = rd_pm4_microcode_datah(dev);
        uint32_t low = rd_pm4_microcode_datal(dev);
        print_instruction(addr + i, high, low);
    }
}

static void
cce_write(ati_device_t *dev, int argc, char **args)
{
    uint32_t addr, high, low;

    if (argc < 5 || parse_int(args[2], &addr) != 0 || addr >= 256 ||
        parse_int(args[3], &high) != 0 || parse_int(args[4], &low) != 0) {
        printf("Usage: cce w <addr> <high> <low> (addr 0-255)\n");
        return;
    }

    // Must wait for idle before writing microcode
    ati_wait_for_idle(dev);
    wr_pm4_microcode_addr(dev, addr);
    wr_pm4_microcode_datah(dev, high);
    wr_pm4_microcode_datal(dev, low);

    // Read back to verify
    wr_pm4_microcode_raddr(dev, addr);
    uint32_t read_high = rd_pm4_microcode_datah(dev);
    uint32_t read_low = rd_pm4_microcode_datal(dev);

    // Check for mismatch
    if (high != read_high || low != read_low) {
        // Show expected vs actual with red on mismatches
        printf("\x1b[90m[%03d]\x1b[0m ", addr);
        print_word(high);
        printf("  ");
        print_word(low);
        printf(" \xe2\x89\xa0 ");  // ≠ in UTF-8
        print_word_mismatch(read_high, high);
        printf("  ");
        print_word_mismatch(read_low, low);
        printf("\n");
    } else {
        print_instruction(addr, high, low);
    }
}

// Get pre-shifted GMC_DST_DATATYPE from current display mode
static uint32_t
get_dst_datatype(ati_device_t *dev)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width = crtc_gen_cntl & CRTC_PIX_WIDTH_MASK;

    switch (pix_width) {
    case CRTC_PIX_WIDTH_8BPP:
        return GMC_DST_DATATYPE_PSEUDO_COLOR_8;
    case CRTC_PIX_WIDTH_15BPP:
        return GMC_DST_DATATYPE_ARGB_1555;
    case CRTC_PIX_WIDTH_16BPP:
        return GMC_DST_DATATYPE_RGB_565;
    case CRTC_PIX_WIDTH_24BPP:
        return GMC_DST_DATATYPE_RGB_888;
    case CRTC_PIX_WIDTH_32BPP:
    default:
        return GMC_DST_DATATYPE_ARGB_8888;
    }
}

// Paint one or more rectangles via CCE PAINT_MULTI packet
static void
cce_paint(ati_device_t *dev, int argc, char **args)
{
    // Format: cce paint <x> <y> <w> <h> [x y w h]... <color>
    // Need at least: cce paint x y w h color = 7 args
    int rect_args = argc - 3;

    if (argc < 7 || rect_args % 4 != 0) {
        printf("Usage: cce paint <x> <y> <w> <h> [x y w h]... <color>\n");
        return;
    }

    int num_rects = rect_args / 4;
    if (num_rects > MAX_PAINT_RECTS) {
        printf("Too many rectangles (max %d)\n", MAX_PAINT_RECTS);
        return;
    }

    uint32_t color;
    if (parse_int(args[argc - 1], &color) != 0) {
        printf("Invalid color: %s\n", args[argc - 1]);
        return;
    }

    // Parse rectangles
    uint32_t rects[MAX_PAINT_RECTS][4];
    for (int i = 0; i < num_rects; i++) {
        int base = 2 + i * 4;
        if (parse_int(args[base], &rects[i][0]) != 0 ||
            parse_int(args[base + 1], &rects[i][1]) != 0 ||
            parse_int(args[base + 2], &rects[i][2]) != 0 ||
            parse_int(args[base + 3], &rects[i][3]) != 0) {
            printf("Invalid rectangle %d\n", i + 1);
            return;
        }
    }

    // Get current display settings
    uint32_t dst_datatype = get_dst_datatype(dev);
    uint32_t pitch = rd_default_pitch(dev);
    uint32_t offset = rd_default_offset(dev);

    // Build GMC value (dst_datatype is already pre-shifted)
    uint32_t gmc = GMC_DST_PITCH_OFFSET_CNTL |
                   GMC_BRUSH_DATATYPE_SOLIDCOLOR |
                   dst_datatype |
                   GMC_SRC_DATATYPE_DST_COLOR |
                   (0xF0 << GMC_ROP3_SHIFT) |
                   GMC_CLR_CMP_CNTL_DIS |
                   GMC_AUX_CLIP_DIS;

    uint32_t pitch_offset = (pitch << 21) | (offset >> 5);

    // Build PAINT_MULTI packet
    uint32_t body_dwords = 3 + num_rects * 2;
    static uint32_t packets[MAX_PAINT_DWORDS];
    int idx = 0;

    packets[idx++] = CCE_PKT3(CCE_CNTL_PAINT_MULTI, body_dwords - 1);
    packets[idx++] = gmc;
    packets[idx++] = pitch_offset;
    packets[idx++] = color;

    for (int i = 0; i < num_rects; i++) {
        packets[idx++] = (rects[i][0] << 16) | rects[i][1];
        packets[idx++] = (rects[i][2] << 16) | rects[i][3];
    }

    ati_cce_pio_submit(dev, packets, idx);
    ati_wait_for_idle(dev);

    printf("Painted %d rectangle(s) with color 0x%06x\n", num_rects, color);
}

static void
cce_statmux(ati_device_t *dev)
{
    // Save current state
    uint32_t saved_micro_cntl = rd_pm4_micro_cntl(dev);

    // Stop the engine (clear FREERUN) but don't wait for idle -
    // capture state as-is for debugging
    wr_pm4_micro_cntl(dev, saved_micro_cntl & ~PM4_MICRO_FREERUN);

    printf("ME_STAT_MUX dump:\n");

    for (uint32_t mux = 0; mux < 32; mux++) {
        // Set the mux value (keep FREERUN cleared)
        wr_pm4_micro_cntl(dev, mux << ME_STAT_MUX_SHIFT);

        // Read back and extract ME_STAT
        uint32_t micro_cntl = rd_pm4_micro_cntl(dev);
        uint32_t stat = micro_cntl & ME_STAT_MASK;

        // Print with labels for known values
        printf("  [%02x] 0x%04x", mux, stat);
        if (mux == 0)
            printf("  (PKT_DWORDS_REMAIN)");
        else if (mux == 1)
            printf("  (PKT_CURR_REGISTER)");
        printf("\n");
    }

    // Restore original state
    wr_pm4_micro_cntl(dev, saved_micro_cntl);
}

static void
cce_step(ati_device_t *dev, int argc, char **args)
{
    uint32_t count = 1;

    if (argc >= 3 && parse_int(args[2], &count) != 0) {
        printf("Usage: cce step [n]\n");
        return;
    }

    // Check if engine is in freerun mode
    uint32_t micro_cntl = rd_pm4_micro_cntl(dev);
    if (micro_cntl & PM4_MICRO_FREERUN) {
        printf("Error: engine is running (use 'cce stop' first)\n");
        return;
    }

    // Single-step by writing ME_STEP bit
    for (uint32_t i = 0; i < count; i++) {
        wr_pm4_micro_cntl(dev, ME_STEP);
    }

    printf("Stepped %u instruction%s\n", count, count == 1 ? "" : "s");
}

// Public functions

void
cce_cmd_help(void)
{
    char buf[32];
    for (int i = 0; cce_cmd_table[i].name != NULL; i++) {
        if (cce_cmd_table[i].usage) {
            snprintf(buf, sizeof(buf), "%s %s", cce_cmd_table[i].name,
                     cce_cmd_table[i].usage);
        } else {
            snprintf(buf, sizeof(buf), "%s", cce_cmd_table[i].name);
        }
        printf("  %-20s - %s\n", buf, cce_cmd_table[i].desc);
    }
}

void
cmd_cce(ati_device_t *dev, int argc, char **args)
{
    if (argc < 2) {
        cce_cmd_help();
        return;
    }

    switch (lookup_cce_cmd(args[1])) {
    case CCE_CMD_INIT:
        cce_init(dev);
        break;
    case CCE_CMD_START:
        cce_start(dev);
        break;
    case CCE_CMD_STOP:
        cce_stop(dev);
        break;
    case CCE_CMD_MODE:
        cce_mode(dev);
        break;
    case CCE_CMD_RELOAD:
        cce_reload(dev);
        break;
    case CCE_CMD_DUMP:
        cce_dump(dev);
        break;
    case CCE_CMD_R:
        cce_read(dev, argc, args);
        break;
    case CCE_CMD_W:
        cce_write(dev, argc, args);
        break;
    case CCE_CMD_PAINT:
        cce_paint(dev, argc, args);
        break;
    case CCE_CMD_STATMUX:
        cce_statmux(dev);
        break;
    case CCE_CMD_STEP:
        cce_step(dev, argc, args);
        break;
    case CCE_CMD_UNKNOWN:
        printf("Unknown cce command: %s\n", args[1]);
        break;
    }
}
