/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "cce_cmd.h"
#include "cce.h"
#include "common.h"
#include "repl.h"

typedef enum {
    CCE_CMD_INIT,
    CCE_CMD_START,
    CCE_CMD_STOP,
    CCE_CMD_MODE,
    CCE_CMD_RELOAD,
    CCE_CMD_R,
    CCE_CMD_W,
    CCE_CMD_UNKNOWN
} cce_cmd_t;

// clang-format off
static const struct {
    const char *name;
    cce_cmd_t cmd;
    const char *usage;
    const char *desc;
} cce_cmd_table[] = {
    {"init",   CCE_CMD_INIT,    NULL,              "full CCE init (load + mode + start)"},
    {"start",  CCE_CMD_START,   NULL,              "start microengine"},
    {"stop",   CCE_CMD_STOP,    NULL,              "stop microengine"},
    {"mode",   CCE_CMD_MODE,    NULL,              "set PM4 PIO mode (no microcode load)"},
    {"reload", CCE_CMD_RELOAD,  NULL,              "load microcode (no start/stop)"},
    {"r",      CCE_CMD_R,       "<addr>",          "read instruction (0-255)"},
    {"w",      CCE_CMD_W,       "<addr> <h> <l>",  "write instruction"},
    {NULL,     CCE_CMD_UNKNOWN, NULL,              NULL}
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
    wr_pm4_buffer_cntl(dev,
                       (PM4_192PIO << PM4_BUFFER_MODE_SHIFT) |
                           PM4_BUFFER_CNTL_NOUPDATE);
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
cce_read(ati_device_t *dev, int argc, char **args)
{
    uint32_t addr;

    if (argc < 3 || parse_int(args[2], &addr) != 0 || addr >= 256) {
        printf("Usage: cce r <addr> (addr 0-255)\n");
        return;
    }

    wr_pm4_microcode_raddr(dev, addr);
    uint32_t high = rd_pm4_microcode_datah(dev);
    uint32_t low = rd_pm4_microcode_datal(dev);
    print_instruction(addr, high, low);
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
    case CCE_CMD_R:
        cce_read(dev, argc, args);
        break;
    case CCE_CMD_W:
        cce_write(dev, argc, args);
        break;
    case CCE_CMD_UNKNOWN:
        printf("Unknown cce command: %s\n", args[1]);
        break;
    }
}
