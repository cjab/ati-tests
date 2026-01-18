/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "dump_cmd.h"
#include "../tests/test.h"

typedef enum {
    DUMP_CMD_SCREEN,
    DUMP_CMD_VRAM,
    DUMP_CMD_UNKNOWN
} dump_cmd_t;

// clang-format off
static const struct {
    const char *name;
    dump_cmd_t cmd;
    const char *usage;
    const char *desc;
} dump_cmd_table[] = {
    {"screen", DUMP_CMD_SCREEN, "[filename]", "dump visible framebuffer (640x480x4)"},
    {"vram",   DUMP_CMD_VRAM,   "[filename]", "dump full VRAM"},
    {NULL,     DUMP_CMD_UNKNOWN, NULL,        NULL}
};
// clang-format on

static dump_cmd_t
lookup_dump_cmd(const char *name)
{
    for (int i = 0; dump_cmd_table[i].name != NULL; i++) {
        if (strcmp(name, dump_cmd_table[i].name) == 0)
            return dump_cmd_table[i].cmd;
    }
    return DUMP_CMD_UNKNOWN;
}

static void
dump_screen(ati_device_t *dev, int argc, char **args)
{
    const char *filename = "screen_dump.bin";
    if (argc >= 3)
        filename = args[2];
    ati_screen_dump(dev, filename);
}

static void
dump_vram(ati_device_t *dev, int argc, char **args)
{
    const char *filename = "vram_dump.bin";
    if (argc >= 3)
        filename = args[2];
    ati_vram_dump(dev, filename);
}

// Public functions

// Print usage string with colored arguments
// <required> in cyan, [optional] in gray
static void
print_usage_colored(const char *usage)
{
    const char *p = usage;
    while (*p) {
        if (*p == '<') {
            printf("\x1b[36m");
            while (*p && *p != '>')
                printf("%c", *p++);
            if (*p == '>')
                printf("%c", *p++);
            printf("\x1b[0m");
        } else if (*p == '[') {
            printf("\x1b[90m");
            while (*p && *p != ']')
                printf("%c", *p++);
            if (*p == ']')
                printf("%c", *p++);
            printf("\x1b[0m");
        } else {
            printf("%c", *p++);
        }
    }
}

void
dump_cmd_help(void)
{
    for (int i = 0; dump_cmd_table[i].name != NULL; i++) {
        // Print command name (bold)
        printf("  \x1b[1m%-8s\x1b[0m", dump_cmd_table[i].name);

        // Print usage args (colored) or padding
        if (dump_cmd_table[i].usage) {
            print_usage_colored(dump_cmd_table[i].usage);
            int len = strlen(dump_cmd_table[i].usage);
            for (int j = len; j < 22; j++)
                printf(" ");
        } else {
            printf("%-22s", "");
        }

        // Print description
        printf("\x1b[90m\xe2\x80\xba\x1b[0m %s\n", dump_cmd_table[i].desc);
    }
}

void
cmd_dump(ati_device_t *dev, int argc, char **args)
{
    if (argc < 2) {
        dump_cmd_help();
        return;
    }

    switch (lookup_dump_cmd(args[1])) {
    case DUMP_CMD_SCREEN:
        dump_screen(dev, argc, args);
        break;
    case DUMP_CMD_VRAM:
        dump_vram(dev, argc, args);
        break;
    case DUMP_CMD_UNKNOWN:
        printf("Unknown dump command: %s\n", args[1]);
        break;
    }
}
