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

bool
test_pm4_microcode(ati_device_t *dev)
{
    /* Microcode writes */
    // Set write addr start
    wr_pm4_microcode_addr(dev, 200);
    wr_pm4_microcode_datah(dev, 0xdeadbeef);
    wr_pm4_microcode_datal(dev, 0xcafec0de);
    wr_pm4_microcode_datah(dev, 0xffffffff);
    wr_pm4_microcode_datal(dev, 0xffffffff);
    // Write addr auto-incremented
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 202);

    ati_cce_wait_for_idle(dev);

    /* Microcode reads */
    // Set read addr
    wr_pm4_microcode_raddr(dev, 200);
    // Which actually writes through to the write addr
    // but must set some sort of internal flag saying this
    // is a read. Setting the write addr directly before reading
    // breaks things.
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);
    ASSERT_EQ(rd_pm4_microcode_datah(dev), 0x0000000f);
    ASSERT_EQ(rd_pm4_microcode_datal(dev), 0xcafec0de);
    // datah is masked to 0x1f
    ASSERT_EQ(rd_pm4_microcode_datah(dev), 0x0000001f);
    ASSERT_EQ(rd_pm4_microcode_datal(dev), 0xffffffff);
    // Read address _always_ returns 0
    ASSERT_EQ(rd_pm4_microcode_raddr(dev), 0);
    // Write addr auto-incremented by reads
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 202);

    return true;
}

void
register_cce_tests(void)
{
    REGISTER_TEST(test_cce, "cce");
    REGISTER_TEST(test_pm4_microcode, "pm4 microcode");
}
