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
    // set write addr start
    wr_pm4_microcode_addr(dev, 200);
    wr_pm4_microcode_datah(dev, 0xdeadbeef);
    // writing to datah does _not_ increment addr
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);

    wr_pm4_microcode_datal(dev, 0xcafec0de);
    // writing to datal increments addr
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 201);

    wr_pm4_microcode_datah(dev, 0xffffffff);
    wr_pm4_microcode_datal(dev, 0xffffffff);
    // write addr incremented again
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 202);

    //ati_cce_wait_for_idle(dev);

    /* Microcode reads */
    // set read addr
    wr_pm4_microcode_raddr(dev, 200);
    // Which actually writes through to the write addr
    // but must set some sort of internal flag saying this
    // is a read. Setting the write addr directly before reading
    // breaks things.
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);

    ASSERT_EQ(rd_pm4_microcode_datah(dev), 0x0000000f);
    // reading datah does _not_ increment addr
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);
    // and can safely be read again without changing state
    ASSERT_EQ(rd_pm4_microcode_datah(dev), 0x0000000f);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);

    ASSERT_EQ(rd_pm4_microcode_datal(dev), 0xcafec0de);
    // reading datal increments addr
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 201);

    // datah is masked to 0x1f
    ASSERT_EQ(rd_pm4_microcode_datah(dev), 0x0000001f);
    ASSERT_EQ(rd_pm4_microcode_datal(dev), 0xffffffff);

    // Read address _always_ returns 0
    ASSERT_EQ(rd_pm4_microcode_raddr(dev), 0);

    // Write addr incremented by reads
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 202);

    /*
     * Test that ADDR and RADDR control separate pointers for reads.
     *
     * RADDR sets both the visible ADDR register and an internal read pointer.
     * ADDR only sets the visible ADDR register, leaving the internal read
     * pointer unchanged. The pointers re-sync after reading DATAL.
     */
    // write known values at indices 10 and 50
    wr_pm4_microcode_addr(dev, 10);
    wr_pm4_microcode_datah(dev, 0x0a);
    wr_pm4_microcode_datal(dev, 0x10101010);

    wr_pm4_microcode_addr(dev, 50);
    wr_pm4_microcode_datah(dev, 0x0b);
    wr_pm4_microcode_datal(dev, 0x50505050);

    // set read pointer to index 10 using RADDR
    wr_pm4_microcode_raddr(dev, 10);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 10);

    // read index 10
    ASSERT_EQ(rd_pm4_microcode_datah(dev), 0x0a);
    ASSERT_EQ(rd_pm4_microcode_datal(dev), 0x10101010);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 11);

    // now write ADDR=50, this should _not_ affect the internal read pointer
    wr_pm4_microcode_addr(dev, 50);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 50);

    // DATAH read still comes from internal pointer (index 11), not ADDR (50)
    uint32_t datah_at_11 = rd_pm4_microcode_datah(dev);
    // ADDR unchanged after DATAH read
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 50);

    // DATAL read comes from internal pointer (index 11), then syncs to ADDR+1
    uint32_t datal_at_11 = rd_pm4_microcode_datal(dev);
    // after DATAL read, ADDR increments and internal pointer syncs
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 51);

    // verify we did _not_ read index 50's data
    ASSERT_TRUE(datah_at_11 != 0x0b);
    ASSERT_TRUE(datal_at_11 != 0x50505050);

    // now reads proceed from index 51 (synced after first DATAL read)
    uint32_t datah_at_51 = rd_pm4_microcode_datah(dev);
    uint32_t datal_at_51 = rd_pm4_microcode_datal(dev);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 52);

    // verify by reading index 51 properly via RADDR
    wr_pm4_microcode_raddr(dev, 51);
    ASSERT_EQ(rd_pm4_microcode_datah(dev), datah_at_51);
    ASSERT_EQ(rd_pm4_microcode_datal(dev), datal_at_51);

    return true;
}

void
register_cce_tests(void)
{
    REGISTER_TEST(test_cce, "cce");
    REGISTER_TEST(test_pm4_microcode, "pm4 microcode");
}
