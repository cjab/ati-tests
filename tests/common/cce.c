/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/cce.h"
#include "../test.h"

bool
test_cce(ati_device_t *dev)
{
    // Initialize CCE engine for this test
    ati_init_cce_engine(dev);

    /* Type-0 packets */
    // Single register write
    uint32_t packets[] = {CCE_PKT0(BIOS_0_SCRATCH, 1), 0xcafebabe};
    ati_cce_pio_submit(dev, packets, 2);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_bios_0_scratch(dev), packets[1]);

    // Multi-register write
    uint32_t packets2[] = {CCE_PKT0(BIOS_0_SCRATCH, 2), 0xdeadbeef,
                           0xbeefc0de};
    ati_cce_pio_submit(dev, packets2, 3);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_bios_0_scratch(dev), packets2[1]);
    ASSERT_EQ(rd_bios_1_scratch(dev), packets2[2]);

    // Write with one_reg_wr flag set
    uint32_t packets3[] = {CCE_PKT0_ONE(BIOS_0_SCRATCH, 2), 0x11111111,
                           0x22222222};
    wr_bios_1_scratch(dev, 0x00000000);
    ati_cce_pio_submit(dev, packets3, 3);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_bios_0_scratch(dev), packets3[2]);
    ASSERT_EQ(rd_bios_1_scratch(dev), 0x00000000);

    ati_stop_cce_engine(dev);

    return true;
}

bool
test_cce_setup(ati_device_t *dev)
{
    uint32_t dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    uint32_t pm4_buffer_cntl = rd_pm4_buffer_cntl(dev);

    /* Changing the buffer mode affects the number of CCE FIFO buffer slots */
    wr_pm4_buffer_cntl(dev, 0x00000000);
    ASSERT_EQ(rd_pm4_stat(dev) & PM4_FIFOCNT_MASK, 192);
    wr_pm4_buffer_cntl(dev, PM4_BUFFER_MODE_192PIO);
    ASSERT_EQ(rd_pm4_buffer_cntl(dev) & PM4_BUFFER_MODE_MASK, PM4_BUFFER_MODE_192PIO);
    ASSERT_EQ(rd_pm4_stat(dev) & PM4_FIFOCNT_MASK, 192);
    wr_pm4_buffer_cntl(dev, PM4_BUFFER_MODE_128PIO_64INDBM);
    ASSERT_EQ(rd_pm4_stat(dev) & PM4_FIFOCNT_MASK, 128);
    wr_pm4_buffer_cntl(dev, PM4_BUFFER_MODE_64PIO_128INDBM);
    ASSERT_EQ(rd_pm4_stat(dev) & PM4_FIFOCNT_MASK, 64);
    // Reset to initial
    wr_pm4_buffer_cntl(dev, pm4_buffer_cntl);

    /* Changing the buffer mode to anything other than 0 (NONPM4) disables
     * MMIO writes to GUI engine registers. (0x1400-0x1fff) */
    wr_dp_gui_master_cntl(dev, (dp_gui_master_cntl & ~GMC_BRUSH_DATATYPE_MASK) |
                          GMC_BRUSH_DATATYPE_32X1_MONO);
    ASSERT_EQ(rd_dp_gui_master_cntl(dev) & GMC_BRUSH_DATATYPE_MASK,
              GMC_BRUSH_DATATYPE_32X1_MONO);
    wr_pm4_buffer_cntl(dev, PM4_BUFFER_MODE_192PIO);
    wr_dp_gui_master_cntl(dev, (dp_gui_master_cntl & ~GMC_BRUSH_DATATYPE_MASK) |
                          GMC_BRUSH_DATATYPE_SOLIDCOLOR);
    // Failed to write to GUI register because we're in CCE 192PIO mode
    ASSERT_EQ(rd_dp_gui_master_cntl(dev) & GMC_BRUSH_DATATYPE_MASK,
              GMC_BRUSH_DATATYPE_32X1_MONO);

    // Reset to initial
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl);
    wr_pm4_buffer_cntl(dev, pm4_buffer_cntl);

    return true;
}

bool
test_cce_packet_submission(ati_device_t *dev)
{
    uint32_t packets[] = {CCE_PKT0(BIOS_0_SCRATCH, 1), 0xcafebabe};
    ati_init_cce_engine(dev);

    ASSERT_EQ(rd_pm4_stat(dev), 192);

    wr_pm4_fifo_data_even(dev, packets[0]);
    ati_wait_for_reg_value(dev, PM4_STAT, MICRO_BUSY | PM4_GUI_ACTIVE | 192);
    ASSERT_EQ(rd_pm4_stat(dev), MICRO_BUSY | PM4_GUI_ACTIVE | 192);

    wr_pm4_fifo_data_odd(dev, packets[1]);
    ati_wait_for_reg_value(dev, PM4_STAT, 192);
    ASSERT_EQ(rd_pm4_stat(dev), 192);

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
    // which also writes through to addr
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);
    // raddr, however, _always_ returns 0 despite internally
    // writing to an independent read address.
    ASSERT_EQ(rd_pm4_microcode_raddr(dev), 0);

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
     * pointer unchanged. The pointers re-sync after reading DATAL (writes
     * do not sync).
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

    /*
     * Test that DATAL writes do NOT sync the internal read pointer.
     */
    // write known values at indices 80 and 81
    wr_pm4_microcode_addr(dev, 80);
    wr_pm4_microcode_datah(dev, 0x08);
    wr_pm4_microcode_datal(dev, 0x80808080);

    wr_pm4_microcode_addr(dev, 81);
    wr_pm4_microcode_datah(dev, 0x18);
    wr_pm4_microcode_datal(dev, 0x81818181);

    // set read pointer to 80, read to advance internal pointer to 81
    wr_pm4_microcode_raddr(dev, 80);
    ASSERT_EQ(rd_pm4_microcode_datal(dev), 0x80808080);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 81);

    // set ADDR=200, internal read pointer still at 81
    wr_pm4_microcode_addr(dev, 200);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 200);

    // write to index 200, DATAL write does NOT sync internal pointer
    wr_pm4_microcode_datah(dev, 0x1f);
    wr_pm4_microcode_datal(dev, 0xdeadbeef);
    ASSERT_EQ(rd_pm4_microcode_addr(dev), 201);

    // read still comes from internal pointer at 81, not from ADDR (201)
    uint32_t datah_after_write = rd_pm4_microcode_datah(dev);
    uint32_t datal_after_write = rd_pm4_microcode_datal(dev);

    // verify we got index 81's data (write did not sync)
    ASSERT_EQ(datah_after_write, 0x18);
    ASSERT_EQ(datal_after_write, 0x81818181);

    return true;
}

void
register_cce_tests(void)
{
    REGISTER_TEST(test_cce, "cce");
    REGISTER_TEST(test_cce_setup, "cce setup");
    REGISTER_TEST(test_cce_packet_submission, "cce packet submission");
    REGISTER_TEST(test_pm4_microcode, "pm4 microcode");
}
