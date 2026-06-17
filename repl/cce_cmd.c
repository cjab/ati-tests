/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "cce_cmd.h"
#include "../ati/cce.h"
#include "../tests/test.h"
#include "repl.h"

typedef enum {
    CCE_CMD_INIT,
    CCE_CMD_START,
    CCE_CMD_STOP,
    CCE_CMD_DUMP,
    CCE_CMD_R,
    CCE_CMD_W,
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
    {"dump",    CCE_CMD_DUMP,    NULL,              "dump all 256 instructions"},
    {"r",       CCE_CMD_R,       "<addr> [count]",  "read instruction(s) (0-255)"},
    {"w",       CCE_CMD_W,       "<addr> <h> <l>",  "write instruction"},
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
    if (ati_init_cce_engine(dev, R128_PM4_BUFFER_MODE_192PIO)) {
        printf("CCE initialized\n");
    } else {
        printf("Failed to initialize CCE engine\n");
    }
}

static void
cce_start(ati_device_t *dev)
{
    if (ati_start_cce_engine(dev, R128_PM4_BUFFER_MODE_192PIO)) {
        printf("CCE engine started\n");
    } else {
        printf("Failed to start CCE engine\n");
    }
}

static void
cce_stop(ati_device_t *dev)
{
    if (ati_stop_cce_engine(dev)) {
        printf("CCE engine stopped\n");
    } else {
        printf("Failed to stop CCE engine\n");
    }
}

static void
cce_dump(ati_device_t *dev)
{
    uint32_t microcode[512];
    if (!ati_dump_microcode(dev, microcode)) {
        printf("Failed to dump microcode\n");
        return;
    }
    for (uint32_t addr = 0; addr < 256; addr++) {
        int idx = addr * 2;
        uint32_t high = microcode[idx];
        uint32_t low = microcode[idx + 1];
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

    for (uint32_t i = 0; i < count; i++) {
        uint64_t inst;
        ati_read_microcode(dev, addr + i, &inst);
        uint32_t high = inst >> 32;
        uint32_t low = inst;
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

    uint64_t inst = (uint64_t) high << 32 | low;
    ati_write_microcode(dev, addr, inst);

    // Read back to verify
    uint64_t test = 0;
    uint32_t read_high = test >> 32;
    uint32_t read_low = test;
    ati_read_microcode(dev, addr, &test);

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
    for (int i = 0; cce_cmd_table[i].name != NULL; i++) {
        // Print command name (bold)
        printf("  \x1b[1m%-8s\x1b[0m", cce_cmd_table[i].name);

        // Print usage args (colored) or padding
        if (cce_cmd_table[i].usage) {
            print_usage_colored(cce_cmd_table[i].usage);
            int len = strlen(cce_cmd_table[i].usage);
            for (int j = len; j < 22; j++)
                printf(" ");
        } else {
            printf("%-22s", "");
        }

        // Print description
        printf("\x1b[90m\xe2\x80\xba\x1b[0m %s\n", cce_cmd_table[i].desc);
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
    case CCE_CMD_DUMP:
        cce_dump(dev);
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
