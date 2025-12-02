/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "platform/platform.h"
#include "ati.h"

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

void
repl(ati_device_t *dev)
{
    char buf[64];
    char *cmd, *arg1, *arg2, *p;

    printf("\n\nTests complete.\n");
    printf("Commands: reboot, r <addr>, w <addr> <val>\n");
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

        if (strcmp(cmd, "reboot") == 0) {
            printf("Rebooting...\n");
            platform_reboot();
        } else if (strcmp(cmd, "r") == 0) {
            uint32_t addr;
            if (arg1 && parse_hex(arg1, &addr) == 0) {
                uint32_t val = ati_reg_read(dev, addr);
                printf("0x%04x = 0x%08x\n", addr, val);
            } else {
                printf("Usage: r <addr>\n");
            }
        } else if (strcmp(cmd, "w") == 0) {
            uint32_t addr, val;
            if (arg1 && arg2 && parse_hex(arg1, &addr) == 0 &&
                parse_hex(arg2, &val) == 0) {
                ati_reg_write(dev, addr, val);
                printf("0x%04x <- 0x%08x\n", addr, val);
            } else {
                printf("Usage: w <addr> <val>\n");
            }
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
        }

        printf("> ");
        fflush(stdout);
    }
}
