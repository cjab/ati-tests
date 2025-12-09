/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "ati.h"
#include "cce.h"
#include "common.h"
#include "platform/platform.h"

// Access mode enum
typedef enum { RW, WO, RO } reg_mode_t;

// Generate register name lookup table from X-macro
typedef struct {
    const char *name;
    uint32_t offset;
    reg_mode_t mode;
} reg_entry_t;

#define X(func_name, const_name, offset, mode) {#const_name, offset, mode},
static const reg_entry_t reg_table[] = {ATI_REGISTERS{NULL, 0, RW}};
#undef X

// Command enum for dispatch
typedef enum {
    CMD_REBOOT,
    CMD_POWEROFF,
    CMD_R,
    CMD_W,
    CMD_VR,
    CMD_VW,
    CMD_PR,
    CMD_PW,
    CMD_MR,
    CMD_T,
    CMD_TL,
    CMD_CCE,
    CMD_HELP,
    CMD_UNKNOWN
} cmd_t;

// clang-format off
static const struct {
    const char *name;
    cmd_t cmd;
    const char *usage;
    const char *desc;
} cmd_table[] = {
    {"reboot",   CMD_REBOOT,   NULL,                     "reboot system (baremetal)"},
    {"poweroff", CMD_POWEROFF, NULL,                     "power off system (baremetal)"},
    {"r",        CMD_R,        "<addr|reg>",             "register read"},
    {"w",        CMD_W,        "<addr|reg> <val>",       "register write"},
    {"vr",       CMD_VR,       "<offset> [count]",       "vram read"},
    {"vw",       CMD_VW,       "<offset> <val> [count]", "vram write"},
    {"pr",       CMD_PR,       "<pixel> [count]",        "pixel read"},
    {"pw",       CMD_PW,       "<pixel> <val> [count]",  "pixel write"},
    {"mr",       CMD_MR,       "<addr> [count]",         "system memory read"},
    {"t",        CMD_T,        "[test_name]",            "run test(s)"},
    {"tl",       CMD_TL,       NULL,                     "list tests"},
    {"cce",      CMD_CCE,      "<cmd>",                  "CCE control (init/start/stop/r/w)"},
    {"help",     CMD_HELP,     NULL,                     NULL},
    {"?",        CMD_HELP,     NULL,                     NULL},
    {NULL,       CMD_UNKNOWN,  NULL,                     NULL}
};
// clang-format on

static cmd_t
lookup_cmd(const char *name)
{
    for (int i = 0; cmd_table[i].name != NULL; i++) {
        if (strcmp(name, cmd_table[i].name) == 0)
            return cmd_table[i].cmd;
    }
    return CMD_UNKNOWN;
}

static void
print_usage(cmd_t cmd)
{
    for (int i = 0; cmd_table[i].name != NULL; i++) {
        if (cmd_table[i].cmd == cmd && cmd_table[i].desc != NULL) {
            if (cmd_table[i].usage)
                printf("Usage: %s %s\n", cmd_table[i].name, cmd_table[i].usage);
            else
                printf("Usage: %s\n", cmd_table[i].name);
            return;
        }
    }
}

// Simple tokenizer: returns next whitespace-delimited token, advances *p
static char *
next_token(char **p)
{
    char *start;

    // Skip leading whitespace
    while (**p == ' ' || **p == '\t')
        (*p)++;

    if (**p == '\0')
        return NULL;

    start = *p;

    // Find end of token
    while (**p && **p != ' ' && **p != '\t')
        (*p)++;

    // Null-terminate if not at end of string
    if (**p)
        *(*p)++ = '\0';

    return start;
}

static int
lookup_reg(const char *name, uint32_t *out, reg_mode_t *mode_out)
{
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (strcasecmp(name, reg_table[i].name) == 0) {
            *out = reg_table[i].offset;
            if (mode_out)
                *mode_out = reg_table[i].mode;
            return 0;
        }
    }
    return -1;
}

static const char *
lookup_reg_name(uint32_t offset)
{
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (reg_table[i].offset == offset) {
            return reg_table[i].name;
        }
    }
    return NULL;
}

static void
print_reg(uint32_t addr, uint32_t val, char separator)
{
    const char *name = lookup_reg_name(addr);
    if (name) {
        printf("\x1b[36m%s\x1b[0m \x1b[90m(0x%04x)\x1b[0m %c \x1b[33m0x%08x\x1b[0m\n",
               name, addr, separator, val);
    } else {
        printf("\x1b[90m0x%04x\x1b[0m %c \x1b[33m0x%08x\x1b[0m\n",
               addr, separator, val);
    }
}

static int
parse_hex(const char *s, uint32_t *out)
{
    uint32_t val = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    if (*s == '\0')
        return -1;

    while (*s) {
        char c = *s++;
        uint32_t digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else
            return -1;
        val = (val << 4) | digit;
    }

    *out = val;
    return 0;
}

// Parse integer: decimal by default, hex with 0x prefix
static int
parse_int(const char *s, uint32_t *out)
{
    // If it starts with 0x, parse as hex
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return parse_hex(s, out);
    }

    // Otherwise parse as decimal
    uint32_t val = 0;

    if (*s == '\0')
        return -1;

    while (*s) {
        char c = *s++;
        if (c >= '0' && c <= '9')
            val = val * 10 + (c - '0');
        else
            return -1;
    }

    *out = val;
    return 0;
}

static int
parse_addr(const char *s, uint32_t *out, reg_mode_t *mode_out)
{
    // Try hex first
    if (parse_int(s, out) == 0) {
        if (mode_out)
            *mode_out = RW; // Unknown register, assume RW
        return 0;
    }
    // Fall back to register name lookup
    return lookup_reg(s, out, mode_out);
}

// Print a single color swatch using ANSI 24-bit color
static void
print_swatch(uint8_t r, uint8_t g, uint8_t b)
{
    printf("\x1b[48;2;%d;%d;%dm  \x1b[0m", r, g, b);
}

// Get bytes per pixel for current CRTC mode
static uint32_t
get_bytes_per_pixel(ati_device_t *dev)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width =
        (crtc_gen_cntl & CRTC_PIX_WIDTH_MASK) >> CRTC_PIX_WIDTH_SHIFT;

    switch (pix_width) {
    case CRTC_PIX_WIDTH_8BPP:
        return 1;
    case CRTC_PIX_WIDTH_15BPP:
    case CRTC_PIX_WIDTH_16BPP:
        return 2;
    case CRTC_PIX_WIDTH_24BPP:
        return 3;
    case CRTC_PIX_WIDTH_32BPP:
        return 4;
    default:
        return 1;
    }
}

// Print pixel value and color swatch based on current pixel format
static void
print_pixel(ati_device_t *dev, uint32_t pixel_idx)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width =
        (crtc_gen_cntl & CRTC_PIX_WIDTH_MASK) >> CRTC_PIX_WIDTH_SHIFT;
    uint32_t bpp = get_bytes_per_pixel(dev);
    uint32_t byte_offset = pixel_idx * bpp;
    uint32_t val = ati_vram_read(dev, byte_offset & ~3); // Align to dword
    uint32_t shift = (byte_offset & 3) * 8;

    switch (pix_width) {
    case CRTC_PIX_WIDTH_32BPP: {
        // aRGB 8888: 0xAARRGGBB
        uint8_t r = (val >> 16) & 0xff;
        uint8_t g = (val >> 8) & 0xff;
        uint8_t b = val & 0xff;
        printf("0x%08x ", val);
        print_swatch(r, g, b);
        break;
    }
    case CRTC_PIX_WIDTH_16BPP: {
        // RGB 565
        uint16_t pixel = (val >> shift) & 0xffff;
        uint8_t r = ((pixel >> 11) & 0x1f) << 3;
        uint8_t g = ((pixel >> 5) & 0x3f) << 2;
        uint8_t b = (pixel & 0x1f) << 3;
        printf("0x%04x ", pixel);
        print_swatch(r, g, b);
        break;
    }
    case CRTC_PIX_WIDTH_15BPP: {
        // aRGB 1555
        uint16_t pixel = (val >> shift) & 0xffff;
        uint8_t r = ((pixel >> 10) & 0x1f) << 3;
        uint8_t g = ((pixel >> 5) & 0x1f) << 3;
        uint8_t b = (pixel & 0x1f) << 3;
        printf("0x%04x ", pixel);
        print_swatch(r, g, b);
        break;
    }
    default:
        // Unsupported format - just print raw value
        printf("0x%08x", val);
        break;
    }
}

#define MAX_ARGS 8

// Command handlers

static void
cmd_help(void)
{
    char buf[32];
    for (int i = 0; cmd_table[i].name != NULL; i++) {
        if (cmd_table[i].desc == NULL)
            continue;
        if (cmd_table[i].usage) {
            snprintf(buf, sizeof(buf), "%s %s", cmd_table[i].name,
                     cmd_table[i].usage);
        } else {
            snprintf(buf, sizeof(buf), "%s", cmd_table[i].name);
        }
        printf("  %-30s - %s\n", buf, cmd_table[i].desc);
    }
}

static void
cmd_reboot(void)
{
    printf("Rebooting...\n");
    platform_reboot();
}

static void
cmd_poweroff(void)
{
    printf("Powering off...\n");
    platform_poweroff();
}

static void
cmd_reg_read(ati_device_t *dev, int argc, char **args)
{
    uint32_t addr;
    reg_mode_t mode;

    if (argc < 2 || parse_addr(args[1], &addr, &mode) != 0) {
        print_usage(CMD_R);
        return;
    }

    if (mode == WO) {
        const char *name = lookup_reg_name(addr);
        printf("\x1b[31mWARNING: %s (\x1b[90m0x%04x\x1b[31m) is write-only\x1b[0m\n",
               name ? name : "register", addr);
    }

    uint32_t val = ati_reg_read(dev, addr);
    print_reg(addr, val, ':');
}

static void
cmd_reg_write(ati_device_t *dev, int argc, char **args)
{
    uint32_t addr, val;
    reg_mode_t mode;

    if (argc < 3 || parse_addr(args[1], &addr, &mode) != 0 ||
        parse_int(args[2], &val) != 0) {
        print_usage(CMD_W);
        return;
    }

    if (mode == RO) {
        const char *name = lookup_reg_name(addr);
        printf("\x1b[31mWARNING: %s (\x1b[90m0x%04x\x1b[31m) is read-only\x1b[0m\n",
               name ? name : "register", addr);
    }

    ati_reg_write(dev, addr, val);
    print_reg(addr, val, '=');
}

static void
cmd_vram_read(ati_device_t *dev, int argc, char **args)
{
    uint32_t offset, count = 1;

    if (argc < 2 || parse_int(args[1], &offset) != 0) {
        print_usage(CMD_VR);
        return;
    }

    if (argc >= 3)
        parse_int(args[2], &count);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t addr = offset + i * 4;
        uint32_t val = ati_vram_read(dev, addr);
        printf("0x%08x: 0x%08x\n", addr, val);
    }
}

static void
cmd_vram_write(ati_device_t *dev, int argc, char **args)
{
    uint32_t offset, val, count = 1;

    if (argc < 3 || parse_int(args[1], &offset) != 0 ||
        parse_int(args[2], &val) != 0) {
        print_usage(CMD_VW);
        return;
    }

    if (argc >= 4)
        parse_int(args[3], &count);
    for (uint32_t i = 0; i < count; i++) {
        ati_vram_write(dev, offset + i * 4, val);
    }
    printf("0x%08x <- 0x%08x (x%d)\n", offset, val, count);
}

static void
cmd_pixel_read(ati_device_t *dev, int argc, char **args)
{
    uint32_t pixel, count = 1;

    if (argc < 2 || parse_int(args[1], &pixel) != 0) {
        print_usage(CMD_PR);
        return;
    }

    if (argc >= 3)
        parse_int(args[2], &count);
    for (uint32_t i = 0; i < count; i++) {
        printf("pixel %d: ", pixel + i);
        print_pixel(dev, pixel + i);
        printf("\n");
    }
}

static void
cmd_pixel_write(ati_device_t *dev, int argc, char **args)
{
    uint32_t pixel, val, count = 1;

    if (argc < 3 || parse_int(args[1], &pixel) != 0 ||
        parse_int(args[2], &val) != 0) {
        print_usage(CMD_PW);
        return;
    }

    if (argc >= 4)
        parse_int(args[3], &count);
    uint32_t bpp = get_bytes_per_pixel(dev);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t byte_offset = (pixel + i) * bpp;
        // Write pixel value at appropriate size
        if (bpp == 4) {
            ati_vram_write(dev, byte_offset, val);
        } else if (bpp == 2) {
            uint32_t aligned = byte_offset & ~3;
            uint32_t shift = (byte_offset & 2) * 8;
            uint32_t mask = 0xffff << shift;
            uint32_t cur = ati_vram_read(dev, aligned);
            cur = (cur & ~mask) | ((val & 0xffff) << shift);
            ati_vram_write(dev, aligned, cur);
        } else if (bpp == 1) {
            uint32_t aligned = byte_offset & ~3;
            uint32_t shift = (byte_offset & 3) * 8;
            uint32_t mask = 0xff << shift;
            uint32_t cur = ati_vram_read(dev, aligned);
            cur = (cur & ~mask) | ((val & 0xff) << shift);
            ati_vram_write(dev, aligned, cur);
        }
    }
    printf("pixel %d <- 0x%x (x%d)\n", pixel, val, count);
}

static void
cmd_mem_read(int argc, char **args)
{
    uint32_t addr, count = 1;

    if (argc < 2 || parse_int(args[1], &addr) != 0) {
        print_usage(CMD_MR);
        return;
    }

    if (argc >= 3)
        parse_int(args[2], &count);
    for (uint32_t i = 0; i < count; i++) {
        volatile uint32_t *ptr =
            (volatile uint32_t *) (uintptr_t) (addr + i * 4);
        printf("0x%08x: 0x%08x\n", addr + i * 4, *ptr);
    }
}

static void
cmd_test(ati_device_t *dev, int argc, char **args)
{
    if (argc >= 2) {
        run_test_by_name(dev, args[1]);
    } else {
        run_all_tests(dev);
    }
}

static void
cmd_test_list(void)
{
    list_tests();
}

static void
cmd_cce(ati_device_t *dev, int argc, char **args)
{
    if (argc < 2) {
        print_usage(CMD_CCE);
        printf(
            "  cce init             - full CCE init (load + mode + start)\n");
        printf("  cce start            - start microengine\n");
        printf("  cce stop             - stop microengine\n");
        printf(
            "  cce mode             - set PM4 PIO mode (no microcode load)\n");
        printf("  cce reload           - load microcode (no start/stop)\n");
        printf("  cce r <addr>         - read instruction (0-255)\n");
        printf("  cce w <addr> <h> <l> - write instruction (no start/stop)\n");
        return;
    }

    if (strcmp(args[1], "init") == 0) {
        ati_init_cce_engine(dev);
        printf("CCE initialized\n");
    } else if (strcmp(args[1], "start") == 0) {
        wr_pm4_micro_cntl(dev, PM4_MICRO_FREERUN);
        printf("CCE microengine started\n");
    } else if (strcmp(args[1], "stop") == 0) {
        ati_wait_for_idle(dev);
        wr_pm4_micro_cntl(dev, 0);
        printf("CCE microengine stopped\n");
    } else if (strcmp(args[1], "mode") == 0) {
        // Set PM4 PIO mode without loading microcode
        wr_pm4_buffer_cntl(dev, (PM4_192PIO << PM4_BUFFER_MODE_SHIFT) |
                                    PM4_BUFFER_CNTL_NOUPDATE);
        (void) rd_pm4_buffer_addr(dev);
        printf("PM4 PIO mode set\n");
    } else if (strcmp(args[1], "reload") == 0) {
        // Just load microcode, no start/stop
        // Must wait for idle before loading microcode
        ati_wait_for_idle(dev);
        ati_cce_load_microcode(dev);
        printf("CCE microcode loaded\n");
    } else if (strcmp(args[1], "r") == 0) {
        uint32_t addr;
        if (argc < 3 || parse_int(args[2], &addr) != 0 || addr >= 256) {
            printf("Usage: cce r <addr> (addr 0-255)\n");
            return;
        }
        wr_pm4_microcode_raddr(dev, addr);
        uint32_t high = rd_pm4_microcode_datah(dev);
        uint32_t low = rd_pm4_microcode_datal(dev);
        printf("[%d] HIGH=0x%08x LOW=0x%08x\n", addr, high, low);
    } else if (strcmp(args[1], "w") == 0) {
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
        // Print results
        printf("[%d] wrote HIGH=0x%08x LOW=0x%08x\n", addr, high, low);
        printf("[%d] read  HIGH=0x%08x LOW=0x%08x\n", addr, read_high,
               read_low);
        if (high != read_high || low != read_low) {
            printf("ERROR: write verification failed!\n");
        }
    } else {
        printf("Unknown cce command: %s\n", args[1]);
    }
}

// Main REPL

void
repl(ati_device_t *dev)
{
    char buf[64];
    char *args[MAX_ARGS];
    int argc;
    char *p;

    printf("Type ? for help\n> ");
    fflush(stdout);

    while (fgets(buf, sizeof(buf), stdin)) {
        // Strip trailing newline
        for (p = buf; *p; p++) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }

        // Parse command and arguments
        p = buf;
        argc = 0;
        while (argc < MAX_ARGS && (args[argc] = next_token(&p)) != NULL)
            argc++;

        if (argc == 0) {
            // Empty line
            printf("> ");
            fflush(stdout);
            continue;
        }

        switch (lookup_cmd(args[0])) {
        case CMD_REBOOT:
            cmd_reboot();
            break;
        case CMD_POWEROFF:
            cmd_poweroff();
            break;
        case CMD_R:
            cmd_reg_read(dev, argc, args);
            break;
        case CMD_W:
            cmd_reg_write(dev, argc, args);
            break;
        case CMD_VR:
            cmd_vram_read(dev, argc, args);
            break;
        case CMD_VW:
            cmd_vram_write(dev, argc, args);
            break;
        case CMD_PR:
            cmd_pixel_read(dev, argc, args);
            break;
        case CMD_PW:
            cmd_pixel_write(dev, argc, args);
            break;
        case CMD_MR:
            cmd_mem_read(argc, args);
            break;
        case CMD_T:
            cmd_test(dev, argc, args);
            break;
        case CMD_TL:
            cmd_test_list();
            break;
        case CMD_CCE:
            cmd_cce(dev, argc, args);
            break;
        case CMD_HELP:
            cmd_help();
            break;
        case CMD_UNKNOWN:
            printf("Unknown command: %s\n", args[0]);
            break;
        }

        printf("> ");
        fflush(stdout);
    }
}
