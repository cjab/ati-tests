/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "platform/platform.h"
#include "ati.h"

// Generate register name lookup table from X-macro
typedef struct {
    const char *name;
    uint32_t offset;
} reg_entry_t;

#define X(func_name, const_name, offset, mode) { #const_name, offset },
static const reg_entry_t reg_table[] = {
    ATI_REGISTERS
    { NULL, 0 }
};
#undef X

static int
strcasecmp_simple(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        char c1 = *s1++;
        char c2 = *s2++;
        if (c1 >= 'a' && c1 <= 'z')
            c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z')
            c2 -= 32;
        if (c1 != c2)
            return c1 - c2;
    }
    return *s1 - *s2;
}

static int
lookup_reg(const char *name, uint32_t *out)
{
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (strcasecmp_simple(name, reg_table[i].name) == 0) {
            *out = reg_table[i].offset;
            return 0;
        }
    }
    return -1;
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

static int
parse_addr(const char *s, uint32_t *out)
{
    // Try hex first
    if (parse_hex(s, out) == 0) {
        return 0;
    }
    // Fall back to register name lookup
    return lookup_reg(s, out);
}

void
repl(ati_device_t *dev)
{
    char buf[64];
    char *cmd, *arg1, *arg2, *arg3, *p;

    printf("\n\nTests complete.\n");
    printf("Commands: reboot, r <addr>, w <addr> <val>\n");
    printf("          vr <offset> [count], vw <offset> <val> [count]\n");
    printf("> ");
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
        while (*p == ' ' || *p == '\t')
            p++;
        cmd = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        while (*p == ' ' || *p == '\t')
            p++;
        arg1 = *p ? p : NULL;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        while (*p == ' ' || *p == '\t')
            p++;
        arg2 = *p ? p : NULL;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        while (*p == ' ' || *p == '\t')
            p++;
        arg3 = *p ? p : NULL;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        if (strcmp(cmd, "reboot") == 0) {
            printf("Rebooting...\n");
            platform_reboot();
        } else if (strcmp(cmd, "r") == 0) {
            uint32_t addr;
            if (arg1 && parse_addr(arg1, &addr) == 0) {
                uint32_t val = ati_reg_read(dev, addr);
                printf("0x%04x = 0x%08x\n", addr, val);
            } else {
                printf("Usage: r <addr|reg_name>\n");
            }
        } else if (strcmp(cmd, "w") == 0) {
            uint32_t addr, val;
            if (arg1 && arg2 && parse_addr(arg1, &addr) == 0 &&
                parse_hex(arg2, &val) == 0) {
                ati_reg_write(dev, addr, val);
                printf("0x%04x <- 0x%08x\n", addr, val);
            } else {
                printf("Usage: w <addr|reg_name> <val>\n");
            }
        } else if (strcmp(cmd, "vr") == 0) {
            uint32_t offset, count = 1;
            if (arg1 && parse_hex(arg1, &offset) == 0) {
                if (arg2)
                    parse_hex(arg2, &count);
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t addr = offset + i * 4;
                    uint32_t val = ati_vram_read(dev, addr);
                    uint8_t r = (val >> 16) & 0xff;
                    uint8_t g = (val >> 8) & 0xff;
                    uint8_t b = val & 0xff;
                    printf("0x%08x: 0x%08x \x1b[48;2;%d;%d;%dm  \x1b[0m\n",
                           addr, val, r, g, b);
                }
            } else {
                printf("Usage: vr <offset> [count]\n");
            }
        } else if (strcmp(cmd, "vw") == 0) {
            uint32_t offset, val, count = 1;
            if (arg1 && arg2 && parse_hex(arg1, &offset) == 0 &&
                parse_hex(arg2, &val) == 0) {
                if (arg3)
                    parse_hex(arg3, &count);
                for (uint32_t i = 0; i < count; i++) {
                    ati_vram_write(dev, offset + i * 4, val);
                }
                printf("0x%08x <- 0x%08x (x%d)\n", offset, val, count);
            } else {
                printf("Usage: vw <offset> <val> [count]\n");
            }
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
        }

        printf("> ");
        fflush(stdout);
    }
}
