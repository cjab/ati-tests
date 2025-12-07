/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../cce.h"
#include "../common.h"

bool
test_cce(ati_device_t *dev)
{
    // Initialize CCE engine for this test
    ati_init_cce_engine(dev);

    uint32_t packets[] = {CCE_PKT0(GUI_SCRATCH_REG0, 0), 0xcafebabe};
    ati_cce_pio_submit(dev, packets, 2);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_gui_scratch_reg0(dev), packets[1]);

    uint32_t packets2[] = {CCE_PKT0(GUI_SCRATCH_REG0, 1), 0xdeadbeef,
                           0xbeefc0de};
    ati_cce_pio_submit(dev, packets2, 3);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_gui_scratch_reg0(dev), packets2[1]);
    ASSERT_EQ(rd_gui_scratch_reg1(dev), packets2[2]);

    return true;
}

void
register_cce_tests(void)
{
    REGISTER_TEST(test_cce, "cce");
}
