/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../ati/ati.h"
#include "cce_cmd.h"
#include "../tests/test.h"
#include "dump_cmd.h"
#include "../platform/platform.h"

// ANSI color codes
#define C_RESET   "\x1b[0m"
#define C_BOLD    "\x1b[1m"
#define C_DIM     "\x1b[90m"
#define C_RED     "\x1b[31m"
#define C_GREEN   "\x1b[32m"
#define C_YELLOW  "\x1b[33m"
#define C_CYAN    "\x1b[36m"
#define C_MAGENTA "\x1b[35m"

// Semantic color aliases
#define C_REG_NAME    C_BOLD
#define C_REG_ADDR    C_DIM
#define C_VALUE       C_YELLOW
#define C_FIELD_SET   C_CYAN
#define C_FIELD_UNSET C_DIM
#define C_ENUM_VAL    C_GREEN
#define C_RE_MARKER   C_MAGENTA

// Symbols
#define SYM_DAGGER "\xe2\x80\xa0"
#define SYM_NEQ    "\xe2\x89\xa0"
#define SYM_RSAQUO "\xe2\x80\xba"

// Generate register name lookup table from X-macro
typedef struct {
    const char *name;
    uint32_t offset;
    uint8_t flags;
    const field_entry_t *fields;
    const field_entry_t *aliases;
} reg_entry_t;

// Build table from common registers (shared between R128 and R100)
// TODO: When chip-specific registers are added, this could be extended
// to include them conditionally based on detected chip
#define X(func_name, const_name, offset, flags, fields, aliases)               \
    {#const_name, offset, flags, fields, aliases},
static const reg_entry_t r128_reg_table[] = {R128_REGISTERS COMMON_REGISTERS{NULL, 0, 0, NULL, NULL}};
static const reg_entry_t r100_reg_table[] = {R100_REGISTERS COMMON_REGISTERS{NULL, 0, 0, NULL, NULL}};
#undef X

// Register snapshot storage - indexed by offset/4
// Highest register offset is ~0x1a00, so 0x2000/4 = 2048 entries is plenty
#define REG_SNAPSHOT_SIZE (0x2000 / 4)
static uint32_t reg_snapshot[REG_SNAPSHOT_SIZE];
static bool snapshot_valid = false;

// Command enum for dispatch
typedef enum {
    CMD_REBOOT,
    CMD_INFO,
    CMD_R,
    CMD_RX,
    CMD_W,
    CMD_VR,
    CMD_VW,
    CMD_PR,
    CMD_PW,
    CMD_CLR,
    CMD_MR,
    CMD_T,
    CMD_TL,
    CMD_CCE,
    CMD_REGS,
    CMD_DUMP,
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
    {"info",     CMD_INFO,     NULL,                     "display system info"},
    {"r",        CMD_R,        "<addr|reg>",             "register read"},
    {"rx",       CMD_RX,       "<addr|reg>",             "register read (expanded)"},
    {"w",        CMD_W,        "<addr|reg> <val>",       "register write"},
    {"vr",       CMD_VR,       "<offset> [count]",       "vram read"},
    {"vw",       CMD_VW,       "<offset> <val> [count]", "vram write"},
    {"pr",       CMD_PR,       "<pixel> [count]",        "pixel read"},
    {"pw",       CMD_PW,       "<pixel> <val> [count]",  "pixel write"},
    {"clr",      CMD_CLR,      "[color]",                "clear the screen"},
    {"mr",       CMD_MR,       "<addr> [count]",         "system memory read"},
    {"t",        CMD_T,        "[test_name]",            "run test(s)"},
    {"tl",       CMD_TL,       NULL,                     "list tests"},
    {"cce",      CMD_CCE,      "<cmd>",                  "CCE control (init/start/stop/r/w)"},
    {"regs",     CMD_REGS,     "<save|diff>",            "register snapshot/diff"},
    {"dump",     CMD_DUMP,     "<cmd>",                  "dump data (screen/vram)"},
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

static const reg_entry_t *
get_chip_reg_table(const ati_device_t *dev)
{
    switch (ati_get_chip_family(dev)) {
        case CHIP_R128:
            return r128_reg_table;
        break;
        case CHIP_R100:
            return r100_reg_table;
        break;
        default:
            return NULL;
    }
}

static const reg_entry_t *
lookup_reg_by_name(const ati_device_t *dev, const char *name)
{
    const reg_entry_t *reg_table = get_chip_reg_table(dev);

    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (strcasecmp(name, reg_table[i].name) == 0) {
            return &reg_table[i];
        }
    }
    return NULL;
}

static const reg_entry_t *
lookup_reg_by_addr(ati_device_t *dev, uint32_t offset)
{
    const reg_entry_t *reg_table = get_chip_reg_table(dev);

    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (offset == reg_table[i].offset) {
            return &reg_table[i];
        }
    }
    return NULL;
}

// Print register flags in colored brackets
// Returns true if any flags were printed
static bool
print_flags(uint8_t flags)
{
    if (flags == 0)
        return false;

    bool first = true;
    printf(" [");

    if (flags & FLAG_READ_SIDE_EFFECTS) {
        printf(C_RED "%sside-effects" C_RESET, first ? "" : ", ");
        first = false;
    }
    if (flags & FLAG_INDIRECT) {
        printf(C_MAGENTA "%sindirect" C_RESET, first ? "" : ", ");
        first = false;
    }
    if (flags & FLAG_NO_READ) {
        printf(C_YELLOW "%sno-read" C_RESET, first ? "" : ", ");
        first = false;
    }
    if (flags & FLAG_NO_WRITE) {
        printf(C_CYAN "%sno-write" C_RESET, first ? "" : ", ");
        first = false;
    }

    printf("]");
    return true;
}

static void
print_reg(ati_device_t *dev, uint32_t addr,
          uint32_t val, char separator, uint8_t flags)
{
    const reg_entry_t *reg = lookup_reg_by_addr(dev, addr);
    if (!reg || !reg->name) {
        printf(C_REG_ADDR "0x%04x" C_RESET, addr);
        print_flags(flags);
        printf(" %c " C_VALUE "0x%08x" C_RESET "\n", separator, val);
        return;
    }
    printf(C_REG_NAME "%s" C_RESET " " C_REG_ADDR "(0x%04x)" C_RESET, reg->name, addr);
    print_flags(flags);
    printf(" %c " C_VALUE "0x%08x" C_RESET "\n", separator, val);
}

static void
print_reg_mismatch(ati_device_t *dev, uint32_t addr, uint32_t wrote, uint32_t got)
{
    const reg_entry_t *reg = lookup_reg_by_addr(dev, addr);
    if (!reg || !reg->name) {
        printf(C_REG_ADDR "0x%04x" C_RESET " = "
               C_VALUE "0x%08x" C_RESET " " SYM_NEQ " " C_RED "0x%08x" C_RESET "\n",
               addr, wrote, got);
        return;
    }
    const char *name = reg->name;
    printf(C_REG_NAME "%s" C_RESET " " C_REG_ADDR "(0x%04x)" C_RESET " = "
           C_VALUE "0x%08x" C_RESET " " SYM_NEQ " " C_RED "0x%08x" C_RESET "\n",
           name, addr, wrote, got);
}

static void
print_mem(uint32_t addr, uint32_t val, char separator)
{
    printf(C_DIM "0x%08x" C_RESET " %c " C_VALUE "0x%08x" C_RESET "\n",
           addr, separator, val);
}

// Find field that starts at exactly 'bit', or NULL
static const field_entry_t *
find_field_at_bit(const field_entry_t *fields, uint8_t bit)
{
    if (fields == NULL)
        return NULL;
    for (const field_entry_t *f = fields; f->name != NULL; f++) {
        if (f->shift == bit)
            return f;
    }
    return NULL;
}

// Find the lowest field start position > after_bit, or 32 if none
static uint8_t
find_next_field_start(const field_entry_t *fields, uint8_t after_bit)
{
    uint8_t next = 32;
    if (fields == NULL)
        return next;
    for (const field_entry_t *f = fields; f->name != NULL; f++) {
        if (f->shift > after_bit && f->shift < next)
            next = f->shift;
    }
    return next;
}

// Look up a value name in the field's value table
static const char *
lookup_value_name(const field_value_t *values, uint32_t val)
{
    if (!values)
        return NULL;
    for (const field_value_t *v = values; v->name != NULL; v++) {
        if (v->value == val)
            return v->name;
    }
    return NULL;
}

// Helper to build a padded field name string
// Writes to buf (must be at least 64 bytes), returns visual width
static int
build_padded_name(char *buf, size_t buflen, const char *name,
                  uint8_t flags, bool *has_re_fields)
{
    int len = strlen(name);
    int target_width = 25;
    char *p = buf;
    char *end = buf + buflen - 1;

    // Copy name
    while (*name && p < end)
        *p++ = *name++;

    // Add RE dagger if needed (uses 1 visual column)
    if (flags & FLAG_REVERSE_ENGINEERED) {
        const char *dagger = C_RE_MARKER SYM_DAGGER C_RESET;
        while (*dagger && p < end)
            *p++ = *dagger++;
        len++;  // dagger takes 1 visual column
        if (has_re_fields)
            *has_re_fields = true;
    }

    // Pad with spaces
    while (len < target_width && p < end) {
        *p++ = ' ';
        len++;
    }

    *p = '\0';
    return len;
}

// Print a single field (known or unknown)
// For unknown fields, pass 0 for flags and NULL for values
static void
print_field(const char *name, uint8_t shift, uint8_t width,
            uint8_t flags, const field_value_t *values, uint32_t val,
            bool *has_re_fields)
{
    uint32_t mask = (width >= 32) ? 0xFFFFFFFF : ((1u << width) - 1);
    uint32_t field_val = (val >> shift) & mask;

    // Build padded name (25 visual chars) with optional RE dagger
    char padded_name[64];
    build_padded_name(padded_name, sizeof(padded_name), name, flags,
                      has_re_fields);

    if (width == 1) {
        // Single bit: [16]
        if (field_val)
            printf("  " C_FIELD_SET "%s" C_RESET " [%5d]   = " C_VALUE "1" C_RESET "\n",
                   padded_name, shift);
        else
            printf("  " C_FIELD_UNSET "%s [%5d]   = 0" C_RESET "\n", padded_name, shift);
    } else {
        // Multi-bit: [11:0]
        const char *value_name = lookup_value_name(values, field_val);
        if (field_val) {
            if (value_name) {
                printf("  " C_FIELD_SET "%s" C_RESET " [%2d:%-2d]   = " C_VALUE "%-5u" C_RESET
                       " (0x%x) " C_ENUM_VAL "%s" C_RESET "\n",
                       padded_name, shift + width - 1, shift,
                       field_val, field_val, value_name);
            } else {
                printf("  " C_FIELD_SET "%s" C_RESET " [%2d:%-2d]   = " C_VALUE "%-5u" C_RESET
                       " (0x%x)\n",
                       padded_name, shift + width - 1, shift,
                       field_val, field_val);
            }
        } else {
            printf("  " C_FIELD_UNSET "%s [%2d:%-2d]   = 0" C_RESET "\n",
                   padded_name, shift + width - 1, shift);
        }
    }
}

// Print a field diff showing old -> new values
// Only prints if the field value changed
static void
print_field_diff(const char *name, uint8_t shift, uint8_t width,
                 uint8_t flags, const field_value_t *values,
                 uint32_t old_reg, uint32_t new_reg, bool *has_re_fields)
{
    uint32_t mask = (width >= 32) ? 0xFFFFFFFF : ((1u << width) - 1);
    uint32_t old_val = (old_reg >> shift) & mask;
    uint32_t new_val = (new_reg >> shift) & mask;

    if (old_val == new_val)
        return;

    // Build padded name (25 visual chars) with optional RE dagger
    char padded_name[64];
    build_padded_name(padded_name, sizeof(padded_name), name, flags,
                      has_re_fields);

    if (width == 1) {
        // Single bit: [16]
        printf("  " C_FIELD_SET "%s" C_RESET " [%5d]   : %u -> %u\n",
               padded_name, shift, old_val, new_val);
    } else {
        // Multi-bit: [11:0]
        const char *old_name = lookup_value_name(values, old_val);
        const char *new_name = lookup_value_name(values, new_val);

        printf("  " C_FIELD_SET "%s" C_RESET " [%2d:%-2d]   : ",
               padded_name, shift + width - 1, shift);

        // Print old value
        if (old_val == 0) {
            printf("0");
        } else if (old_name) {
            printf("%u (0x%x) " C_ENUM_VAL "%s" C_RESET, old_val, old_val, old_name);
        } else {
            printf("%u (0x%x)", old_val, old_val);
        }

        printf(" -> ");

        // Print new value
        if (new_val == 0) {
            printf("0");
        } else if (new_name) {
            printf("%u (0x%x) " C_ENUM_VAL "%s" C_RESET, new_val, new_val, new_name);
        } else {
            printf("%u (0x%x)", new_val, new_val);
        }

        printf("\n");
    }
}

// Print field-level diff for a register
// Returns true if any reverse-engineered fields were printed
static bool
print_reg_diff(const reg_entry_t *reg, uint32_t old_val, uint32_t new_val)
{
    bool has_re_fields = false;
    uint8_t bit = 0;
    while (bit < 32) {
        const field_entry_t *f = find_field_at_bit(reg->fields, bit);

        if (f) {
            // Known field
            print_field_diff(f->name, f->shift, f->width, f->flags, f->values,
                             old_val, new_val, &has_re_fields);
            bit += f->width;
        } else {
            // Unknown gap - find where next known field starts
            uint8_t next_start = find_next_field_start(reg->fields, bit);
            uint8_t gap_width = next_start - bit;

            print_field_diff("(unknown)", bit, gap_width, 0, NULL,
                             old_val, new_val, &has_re_fields);
            bit = next_start;
        }
    }
    return has_re_fields;
}

static void
print_reg_expanded(const reg_entry_t *reg, uint32_t val)
{
    // Header line
    printf(C_REG_NAME "%s" C_RESET " " C_REG_ADDR "(0x%04x)" C_RESET, reg->name, reg->offset);
    print_flags(reg->flags);
    printf(" : " C_VALUE "0x%08x" C_RESET "\n", val);

    bool has_re_fields = false;
    uint8_t bit = 0;
    while (bit < 32) {
        const field_entry_t *f = find_field_at_bit(reg->fields, bit);

        if (f) {
            // Known field
            print_field(f->name, f->shift, f->width, f->flags, f->values, val,
                        &has_re_fields);
            bit += f->width;
        } else {
            // Unknown gap - find where next known field starts
            uint8_t next_start = find_next_field_start(reg->fields, bit);
            uint8_t gap_width = next_start - bit;

            print_field("(unknown)", bit, gap_width, 0, NULL, val,
                        &has_re_fields);
            bit = next_start;
        }
    }

    // Print aliases section if present
    if (reg->aliases && reg->aliases[0].name != NULL) {
        printf("  " C_DIM "--- aliases ---" C_RESET "\n");
        for (const field_entry_t *a = reg->aliases; a->name != NULL; a++) {
            print_field(a->name, a->shift, a->width, 0, NULL, val,
                        &has_re_fields);
        }
    }

    // Print legend if any RE fields were displayed
    if (has_re_fields) {
        printf("\n" C_RE_MARKER SYM_DAGGER C_RESET " = reverse-engineered "
               "(not found in documentation or drivers)\n");
    }
}

int
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
int
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
parse_reg(ati_device_t *dev, const char *s)
{
    uint32_t addr;
    const reg_entry_t *reg;
    if (parse_int(s, &addr) == 0) {
        return addr;
    }
    if ((reg = lookup_reg_by_name(dev, s))) {
        return reg->offset;
    }
    return -1;
}

// Print a single color swatch using ANSI 24-bit background color
static void
print_swatch(uint8_t r, uint8_t g, uint8_t b)
{
    printf("\x1b[48;2;%d;%d;%dm  " C_RESET, r, g, b);
}

// Get bytes per pixel for current CRTC mode
static uint32_t
get_bytes_per_pixel(ati_device_t *dev)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width = crtc_gen_cntl & CRTC_PIX_WIDTH_MASK;

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
print_pixel(ati_device_t *dev, uint32_t pixel_idx, char separator)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width = crtc_gen_cntl & CRTC_PIX_WIDTH_MASK;
    uint32_t bpp = get_bytes_per_pixel(dev);
    uint32_t byte_offset = pixel_idx * bpp;
    uint32_t val = ati_vram_read(dev, byte_offset & ~3); // Align to dword
    uint32_t shift = (byte_offset & 3) * 8;

    printf(C_DIM "pixel %d" C_RESET " %c ", pixel_idx, separator);

    switch (pix_width) {
    case CRTC_PIX_WIDTH_32BPP: {
        // aRGB 8888: 0xAARRGGBB
        uint8_t r = (val >> 16) & 0xff;
        uint8_t g = (val >> 8) & 0xff;
        uint8_t b = val & 0xff;
        printf(C_VALUE "0x%08x" C_RESET " ", val);
        print_swatch(r, g, b);
        break;
    }
    case CRTC_PIX_WIDTH_16BPP: {
        // RGB 565
        uint16_t pixel = (val >> shift) & 0xffff;
        uint8_t r = ((pixel >> 11) & 0x1f) << 3;
        uint8_t g = ((pixel >> 5) & 0x3f) << 2;
        uint8_t b = (pixel & 0x1f) << 3;
        printf(C_VALUE "0x%04x" C_RESET " ", pixel);
        print_swatch(r, g, b);
        break;
    }
    case CRTC_PIX_WIDTH_15BPP: {
        // aRGB 1555
        uint16_t pixel = (val >> shift) & 0xffff;
        uint8_t r = ((pixel >> 10) & 0x1f) << 3;
        uint8_t g = ((pixel >> 5) & 0x1f) << 3;
        uint8_t b = (pixel & 0x1f) << 3;
        printf(C_VALUE "0x%04x" C_RESET " ", pixel);
        print_swatch(r, g, b);
        break;
    }
    default:
        // Unsupported format - just print raw value
        printf(C_VALUE "0x%08x" C_RESET, val);
        break;
    }
    printf("\n");
}

#define MAX_ARGS 32

// Command handlers

// Print usage string with colored arguments
// <required> in cyan, [optional] in gray
static void
print_usage_colored(const char *usage)
{
    const char *p = usage;
    while (*p) {
        if (*p == '<') {
            printf(C_CYAN);
            while (*p && *p != '>')
                printf("%c", *p++);
            if (*p == '>')
                printf("%c", *p++);
            printf(C_RESET);
        } else if (*p == '[') {
            printf(C_DIM);
            while (*p && *p != ']')
                printf("%c", *p++);
            if (*p == ']')
                printf("%c", *p++);
            printf(C_RESET);
        } else {
            printf("%c", *p++);
        }
    }
}

static void
cmd_help(int argc, char **args)
{
    // Handle subcommand help: ? cce
    if (argc >= 2) {
        if (strcmp(args[1], "cce") == 0) {
            cce_cmd_help();
            return;
        }
        if (strcmp(args[1], "dump") == 0) {
            dump_cmd_help();
            return;
        }
        printf("Unknown help topic: %s\n", args[1]);
        return;
    }

    for (int i = 0; cmd_table[i].name != NULL; i++) {
        if (cmd_table[i].desc == NULL)
            continue;

        // Print command name (bold)
        printf("  " C_BOLD "%-8s" C_RESET, cmd_table[i].name);

        // Print usage args (colored) or padding
        if (cmd_table[i].usage) {
            print_usage_colored(cmd_table[i].usage);
            // Calculate padding needed (target 22 chars for usage field)
            int len = strlen(cmd_table[i].usage);
            for (int j = len; j < 22; j++)
                printf(" ");
        } else {
            printf("%-22s", "");
        }

        // Print description (gray angle quote separator)
        printf(C_DIM SYM_RSAQUO C_RESET " %s\n", cmd_table[i].desc);
    }
}

static void
cmd_reboot(void)
{
    printf("Rebooting...\n");
    platform_reboot();
}

static void
cmd_reg_read(ati_device_t *dev, int argc, char **args)
{
    int addr;
    if (argc < 2 || (addr = parse_reg(dev, args[1])) == -1) {
        print_usage(CMD_R);
        return;
    }
    uint32_t val = ati_reg_read(dev, addr);
    const reg_entry_t *reg = lookup_reg_by_addr(dev, addr);
    print_reg(dev, addr, val, ':', reg ? reg->flags : 0);
}

static void
cmd_reg_read_expanded(ati_device_t *dev, int argc, char **args)
{
    int addr;
    if (argc < 2 || (addr = parse_reg(dev, args[1])) == -1) {
        print_usage(CMD_RX);
        return;
    }

    const reg_entry_t *reg = lookup_reg_by_addr(dev, addr);
    if (reg == NULL) {
        // Unknown register, fall back to simple display
        uint32_t val = ati_reg_read(dev, addr);
        print_reg(dev, addr, val, ':', 0);
        printf("  " C_DIM "(unknown register)" C_RESET "\n");
        return;
    }

    uint32_t val = ati_reg_read(dev, addr);
    print_reg_expanded(reg, val);
}

// Flags that indicate we should skip readback after write
#define REG_SKIP_READBACK (FLAG_NO_READ | FLAG_READ_SIDE_EFFECTS | FLAG_INDIRECT)

static void
cmd_reg_write(ati_device_t *dev, int argc, char **args)
{
    uint32_t val;

    int addr;
    if (argc < 2 || (addr = parse_reg(dev, args[1]) == -1) ||
        parse_int(args[2], &val) != 0) {
        print_usage(CMD_W);
        return;
    }

    ati_reg_write(dev, addr, val);

    const reg_entry_t *reg = lookup_reg_by_addr(dev, addr);
    uint8_t flags = reg ? reg->flags : 0;
    // Skip readback if register can't be read safely
    if (flags & REG_SKIP_READBACK) {
        print_reg(dev, addr, val, '=', flags);
    } else {
        uint32_t readback = ati_reg_read(dev, addr);
        if (readback != val) {
            print_reg_mismatch(dev, addr, val, readback);
        } else {
            print_reg(dev, addr, readback, '=', flags);
        }
    }
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
        print_mem(addr, val, ':');
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
        uint32_t addr = offset + i * 4;
        ati_vram_write(dev, addr, val);
        print_mem(addr, val, '=');
    }
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
        print_pixel(dev, pixel + i, ':');
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
        print_pixel(dev, pixel + i, '=');
    }
}


static void
cmd_clr(ati_device_t *dev, int argc, char **args)
{
    uint32_t color = 0x00000000;

    if (argc >= 2 && parse_int(args[1], &color) != 0) {
        print_usage(CMD_CLR);
        return;
    }

    ati_screen_clear(dev, color);
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
        uint32_t a = addr + i * 4;
        volatile uint32_t *ptr = (volatile uint32_t *) (uintptr_t) a;
        print_mem(a, *ptr, ':');
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

// Flags that indicate register is unsafe to read during snapshot
#define REG_UNSAFE_READ (FLAG_NO_READ | FLAG_READ_SIDE_EFFECTS | FLAG_INDIRECT)

static void
regs_save(ati_device_t *dev)
{
    const reg_entry_t *reg_table = get_chip_reg_table(dev);
    int count = 0;
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (reg_table[i].flags & REG_UNSAFE_READ)
            continue;
        uint32_t offset = reg_table[i].offset;
        reg_snapshot[offset / 4] = ati_reg_read(dev, offset);
        count++;
    }
    snapshot_valid = true;
    printf("Saved %d registers\n", count);
}

static void
regs_diff(ati_device_t *dev)
{
    if (!snapshot_valid) {
        printf("No snapshot taken. Use 'regs save' first.\n");
        return;
    }
    const reg_entry_t *reg_table = get_chip_reg_table(dev);
    bool has_re_fields = false;
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (reg_table[i].flags & REG_UNSAFE_READ)
            continue;
        uint32_t offset = reg_table[i].offset;
        uint32_t old_val = reg_snapshot[offset / 4];
        uint32_t new_val = ati_reg_read(dev, offset);
        if (old_val != new_val) {
            printf("%s (0x%04x) : 0x%08x -> 0x%08x\n",
                   reg_table[i].name, offset, old_val, new_val);
            if (print_reg_diff(&reg_table[i], old_val, new_val))
                has_re_fields = true;
        }
    }
    // Print legend if any RE fields were displayed
    if (has_re_fields) {
        printf("\n" C_RE_MARKER SYM_DAGGER C_RESET " = reverse-engineered "
               "(not found in documentation or drivers)\n");
    }
}

static void
cmd_regs(ati_device_t *dev, int argc, char **args)
{
    if (argc < 2) {
        print_usage(CMD_REGS);
        return;
    }

    if (strcmp(args[1], "save") == 0) {
        regs_save(dev);
    } else if (strcmp(args[1], "diff") == 0) {
        regs_diff(dev);
    } else {
        print_usage(CMD_REGS);
    }
}

// Main REPL

// End-of-transmission marker - signals command completion to client
#define EOT '\x04'

void
repl(ati_device_t *dev)
{
    char buf[64];
    char *args[MAX_ARGS];
    int argc;
    char *p;

    printf("Type ? for help\n%c> ", EOT);
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
            printf("%c> ", EOT);
            fflush(stdout);
            continue;
        }

        switch (lookup_cmd(args[0])) {
        case CMD_REBOOT:
            cmd_reboot();
            break;
        case CMD_INFO:
            ati_print_info(dev);
            break;
        case CMD_R:
            cmd_reg_read(dev, argc, args);
            break;
        case CMD_RX:
            cmd_reg_read_expanded(dev, argc, args);
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
        case CMD_CLR:
            cmd_clr(dev, argc, args);
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
        case CMD_REGS:
            cmd_regs(dev, argc, args);
            break;
        case CMD_DUMP:
            cmd_dump(dev, argc, args);
            break;
        case CMD_HELP:
            cmd_help(argc, args);
            break;
        case CMD_UNKNOWN:
            printf("Unknown command: %s\n", args[0]);
            break;
        }

        printf("%c> ", EOT);
        fflush(stdout);
    }
}
