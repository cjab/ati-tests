/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "cce_cmd.h"
#include "../ati/cce.h"
#include "../tests/test.h"
#include "repl.h"

typedef enum {
    PKT_CMD_0,
    PKT_CMD_0_ONE,
    PKT_CMD_1,
    PKT_CMD_2,
    PKT_CMD_UNKNOWN
} pkt_cmd_t;

// clang-format off
static const struct {
    const char *name;
    pkt_cmd_t cmd;
    const char *usage;
    const char *desc;
} pkt_cmd_table[] = {
    {"0",     PKT_CMD_0,       "<addr|reg> *[val]",                       "Type 0 Packet"},
    {"0_ONE", PKT_CMD_0_ONE,   "<addr|reg> *[val]",                       "Type 0 Packet with ONE_REG_WR flag"},
    {"1",     PKT_CMD_1,       "<addr1|reg1> <val1> <addr2|reg2> <val2>", "Type 1 Packet"},
    {"2",     PKT_CMD_2,       NULL,                                      "Type 2 Packet"},
    {NULL,    PKT_CMD_UNKNOWN, NULL,                                      NULL}
};
// clang-format on

static pkt_cmd_t
lookup_pkt_cmd(const char *name)
{
    for (int i = 0; pkt_cmd_table[i].name != NULL; i++) {
        if (strcmp(name, pkt_cmd_table[i].name) == 0)
            return pkt_cmd_table[i].cmd;
    }
    return PKT_CMD_UNKNOWN;
}

// Subcommand handlers
static void
pkt_0(ati_device_t *dev, int argc, char **args)
{
    int addr;
    uint32_t packet[16];
    uint32_t dwords = 1;

    if (argc < 4 || (addr = parse_reg(dev, args[2])) == -1) {
        print_usage_colored(pkt_cmd_table[PKT_CMD_0].usage);
        return;
    }

    for (int i = 3; i < argc; i++) {
        if (parse_int(args[i], &packet[i - 2]) == -1) {
            print_usage_colored(pkt_cmd_table[PKT_CMD_0].usage);
        }
        dwords += 1;
    }

    packet[0] = CCE_PKT0(addr, dwords - 1);

    ati_send_packet(dev, packet, dwords);
}

static void
pkt_0_one(ati_device_t *dev, int argc, char **args)
{
    int addr;
    uint32_t packet[16];
    uint32_t dwords = 1;

    if (argc < 4 || (addr = parse_reg(dev, args[2])) == -1) {
        print_usage_colored(pkt_cmd_table[PKT_CMD_0].usage);
        return;
    }

    for (int i = 3; i < argc; i++) {
        if (parse_int(args[i], &packet[i - 2]) == -1) {
            print_usage_colored(pkt_cmd_table[PKT_CMD_0].usage);
        }
        dwords += 1;
    }

    packet[0] = CCE_PKT0_ONE(addr, dwords - 1);

    ati_send_packet(dev, packet, dwords);
}

static void
pkt_1(ati_device_t *dev, int argc, char **args)
{
}

static void
pkt_2(ati_device_t *dev)
{
}


void
pkt_cmd_help(void)
{
    for (int i = 0; pkt_cmd_table[i].name != NULL; i++) {
        // Print command name (bold)
        printf("  \x1b[1m%-8s\x1b[0m", pkt_cmd_table[i].name);

        // Print usage args (colored) or padding
        if (pkt_cmd_table[i].usage) {
            print_usage_colored(pkt_cmd_table[i].usage);
            int len = strlen(pkt_cmd_table[i].usage);
            for (int j = len; j < 22; j++)
                printf(" ");
        } else {
            printf("%-22s", "");
        }

        // Print description
        printf("\x1b[90m\xe2\x80\xba\x1b[0m %s\n", pkt_cmd_table[i].desc);
    }
}

void
cmd_pkt(ati_device_t *dev, int argc, char **args)
{
    if (argc < 2) {
        cce_cmd_help();
        return;
    }

    switch (lookup_pkt_cmd(args[1])) {
    case PKT_CMD_0:
        pkt_0(dev, argc, args);
        break;
    case PKT_CMD_0_ONE:
        pkt_0_one(dev, argc, args);
        break;
    case PKT_CMD_1:
        pkt_1(dev, argc, args);
        break;
    case PKT_CMD_2:
        pkt_2(dev);
        break;
    case PKT_CMD_UNKNOWN:
        printf("Unknown pkt command: %s\n", args[1]);
        break;
    }
}
