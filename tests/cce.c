/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../cce.h"
#include "../common.h"

bool
test_cce(ati_device_t *dev)
{
    printf("test_cce: start\n");
    uint32_t packets[] = {
        CCE_PKT0(GUI_SCRATCH_REG0, 5),
        0xdeadbeef,
        0xdeadbabe,
        0xdeadc0de,
        0xdeadf00d,
        0xdeadface,
        0xdeadcafe,
    };
    size_t count = sizeof(packets) / sizeof(packets[0]);
    printf("test_cce: calling submit with %zu dwords\n", count);
    ati_cce_pio_submit(dev, packets, count);
    printf("test_cce: submitted\n");
    return true;
}

void
register_cce_tests(void)
{
    REGISTER_TEST(test_cce, "cce");
}
