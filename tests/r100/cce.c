/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/cce.h"
#include "../../ati/r100_cce.h"
#include "../../ati/r100_mc.h"
#include "../test.h"

static volatile uint32_t mem[1024] __attribute__((aligned(0x08000000)));

bool test_r100_cce(ati_device_t *dev) {
    // Initialize CCE engine for this test
    ati_init_cce_engine(dev, R100_CSQ_MODE_PIO);

    wr_bios_0_scratch(dev, 0x0);
    wr_bios_1_scratch(dev, 0x0);
    wr_bios_2_scratch(dev, 0x0);
    wr_bios_3_scratch(dev, 0x0);

    /* Type-0 packets */
    // Single register write
    uint32_t packets[] = {CCE_PKT0(BIOS_0_SCRATCH, 1), 0xcafebabe};
    ati_r100_cce_pio_submit(dev, packets, 2);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_bios_0_scratch(dev), packets[1]);

    // Multi-register write
    uint32_t packets2[] = {CCE_PKT0(BIOS_0_SCRATCH, 2), 0xdeadbeef,
                           0xbeefc0de};
    ati_r100_cce_pio_submit(dev, packets2, 3);

    ati_wait_for_idle(dev);
    ASSERT_EQ(rd_bios_0_scratch(dev), packets2[1]);
    ASSERT_EQ(rd_bios_1_scratch(dev), packets2[2]);

    // Write with one_reg_wr flag set
    // FIXME: This does not currently work on the R100. I'm a bit confused as to
    //        why. The packet seems to get processed (status bits update) but the
    //        write never occurs. I've tried this on gui scratch registers as well
    //        as RE_STIPPLE_{ADDR,DATA} which is where this is actually used by the
    //        linux driver. No luck. It's possible this only works when the ring
    //        buffer is enabled?
    //uint32_t packets3[] = {CCE_PKT0_ONE(BIOS_2_SCRATCH, 3), 0x11111111,
    //                       0x22222222, 0x33333333};
    ////wr_bios_3_scratch(dev, 0x00000000);
    //ati_wait_for_idle(dev);
    //ati_r100_cce_pio_submit(dev, packets3, 3);

    //ati_wait_for_idle(dev);
    //ASSERT_EQ(rd_bios_2_scratch(dev), packets3[2]);
    //ASSERT_EQ(rd_bios_3_scratch(dev), 0x00000000);

    ati_stop_cce_engine(dev);

    return true;
}

bool
test_r100_cce_setup(ati_device_t *dev)
{
    /* Changing the buffer mode affects the number of CCE FIFO buffer slots */
    wr_r100_cp_csq_cntl(dev, R100_CSQ_MODE_DISABLED);
    ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_PRIMARY_MASK, 0);
    ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_INDIRECT_MASK, 0);
    ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_MODE_MASK, R100_CSQ_MODE_DISABLED);

    wr_r100_cp_csq_cntl(dev, R100_CSQ_MODE_PIO);
    //ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_PRIMARY_MASK, 255);
    //ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_INDIRECT_MASK, 0);
    ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_MODE_MASK, R100_CSQ_MODE_PIO);

    wr_r100_cp_csq_cntl(dev, R100_CSQ_MODE_BM);
    //ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_PRIMARY_MASK, 255);
    //ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_INDIRECT_MASK, 0);
    ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_MODE_MASK, R100_CSQ_MODE_BM);

    wr_r100_cp_csq_cntl(dev, R100_CSQ_MODE_PIO_INDBM);
    //ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_PRIMARY_MASK, 127);
    //ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_CNT_INDIRECT_MASK, 127 << 8);
    ASSERT_EQ(rd_r100_cp_csq_cntl(dev) & R100_CSQ_MODE_MASK, R100_CSQ_MODE_PIO_INDBM);

    // Disable CP
    wr_r100_cp_csq_cntl(dev, R100_CSQ_MODE_DISABLED);

    /* Unlike r128 enabling the CP on the r100 does not seem to disable the gui
     * engine registers. (0x1400-0x1fff) */
    uint32_t dst_offset = rd_dst_offset(dev);

    wr_dst_offset(dev, 0x11111110);
    ASSERT_EQ(rd_dst_offset(dev), 0x11111110);

    // Re-enable CP
    wr_r100_cp_csq_cntl(dev, R100_CSQ_MODE_PIO);
    wr_dst_offset(dev, 0x22222220);
    // Wrote to GUI register, they are not disabled in PIO mode
    ASSERT_EQ(rd_dst_offset(dev), 0x22222220);

    // Reset to initial
    wr_dst_offset(dev, dst_offset);

    ati_stop_cce_engine(dev);

    return true;
}

bool
test_r100_cce_packet_submission(ati_device_t *dev)
{
    uint32_t packets[] = {CCE_PKT0(BIOS_0_SCRATCH, 1), 0xcafebabe};
    ati_init_cce_engine(dev, R100_CSQ_MODE_PIO);

    ASSERT_EQ(rd_r100_rbbm_status(dev), R100_HIRQ_ON_RBB | 64);
    ASSERT_EQ(rd_r100_cp_csq_stat(dev), 0);

    wr_r100_cp_csq_aper_primary(dev, packets[0]);
    ati_wait_for_reg_value(dev, R100_RBBM_STATUS,
                           R100_CP_CMDSTRM_BUSY | R100_GUI_ACTIVE | R100_HIRQ_ON_RBB | 64);
    ASSERT_EQ(rd_r100_rbbm_status(dev),
              R100_CP_CMDSTRM_BUSY | R100_GUI_ACTIVE | R100_HIRQ_ON_RBB | 64);
    ASSERT_EQ(rd_r100_cp_stat(dev),
              R100_RSIU_BUSY | R100_CSI_BUSY | R100_CMDSTRM_BUSY | R100_CP_BUSY);

    wr_r100_cp_csq_aper_primary(dev, packets[1]);
    ati_wait_for_reg_value(dev, R100_RBBM_STATUS, R100_HIRQ_ON_RBB | 64);
    ASSERT_EQ(rd_r100_rbbm_status(dev), R100_HIRQ_ON_RBB | 64);
    ASSERT_EQ(rd_r100_cp_stat(dev), R100_RSIU_BUSY | R100_CP_BUSY);

    return true;
}

bool
test_r100_cce_mm_indirect(ati_device_t *dev)
{
    wr_mm_index(dev, BIOS_0_SCRATCH);
    wr_mm_data(dev, 0x1337beef);

    ati_init_cce_engine(dev, R100_CSQ_MODE_PIO);
    //ASSERT_EQ(rd_r100_rbbm_status(dev), R100_HIRQ_ON_RBB | 64);

    uint32_t idx_packets[] = {CCE_PKT0(MM_INDEX, 1), BIOS_1_SCRATCH};
    wr_r100_cp_csq_aper_primary(dev, idx_packets[0]);
    //ati_wait_for_reg_value(dev, R100_RBBM_STATUS,
    //                       R100_CP_CMDSTRM_BUSY | R100_GUI_ACTIVE | R100_HIRQ_ON_RBB | 64);
    //ASSERT_EQ(rd_r100_rbbm_status(dev),
    //          R100_CP_CMDSTRM_BUSY | R100_GUI_ACTIVE | R100_HIRQ_ON_RBB | 64);

    wr_r100_cp_csq_aper_primary(dev, idx_packets[1]);
    //ati_wait_for_reg_value(dev, R100_RBBM_STATUS, R100_HIRQ_ON_RBB | 64);
    //ASSERT_EQ(rd_r100_rbbm_status(dev), R100_HIRQ_ON_RBB | 64);

    // The CCE engine cannot write to MM_INDEX
    ASSERT_NEQ(rd_mm_index(dev), BIOS_1_SCRATCH);

    uint32_t data_packets[] = {CCE_PKT0(MM_DATA, 1), 0x11111111};
    wr_r100_cp_csq_aper_primary(dev, data_packets[0]);
    //ati_wait_for_reg_value(dev, R100_RBBM_STATUS,
    //                       R100_CP_CMDSTRM_BUSY | R100_GUI_ACTIVE | R100_HIRQ_ON_RBB | 64);
    //ASSERT_EQ(rd_r100_rbbm_status(dev),
    //          R100_CP_CMDSTRM_BUSY | R100_GUI_ACTIVE | R100_HIRQ_ON_RBB | 64);
    wr_r100_cp_csq_aper_primary(dev, data_packets[1]);
    //ati_wait_for_reg_value(dev, R100_RBBM_STATUS, R100_HIRQ_ON_RBB | 64);
    //ASSERT_EQ(rd_r100_rbbm_status(dev), R100_HIRQ_ON_RBB | 64);
    // The CCE engine cannot write to MM_DATA
    ASSERT_NEQ(rd_mm_data(dev), 0x11111111);

    return true;
}


bool
test_r100_microcode(ati_device_t *dev)
{
    /* Microcode writes */
    // set write addr start
    wr_r100_cp_me_ram_addr(dev, 200);
    wr_r100_cp_me_ram_datah(dev, 0xdeadbeef);
    // writing to datah does _not_ increment addr
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 200);

    wr_r100_cp_me_ram_datal(dev, 0xcafec0de);
    // writing to datal increments addr
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 201);

    wr_r100_cp_me_ram_datah(dev, 0xffffffff);
    wr_r100_cp_me_ram_datal(dev, 0xffffffff);
    // write addr incremented again
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 202);

    //ati_cce_wait_for_idle(dev);

    /* Microcode reads */
    // set read addr
    wr_r100_cp_me_ram_raddr(dev, 200);
    // which also writes through to addr
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 200);
    // raddr, however, _always_ returns 0 despite internally
    // writing to an independent read address.
    ASSERT_EQ(rd_r100_cp_me_ram_raddr(dev), 0);

    ASSERT_EQ(rd_r100_cp_me_ram_datah(dev), 0x0000002f);
    // reading datah does _not_ increment addr
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 200);
    // and can safely be read again without changing state
    ASSERT_EQ(rd_r100_cp_me_ram_datah(dev), 0x0000002f);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 200);

    ASSERT_EQ(rd_r100_cp_me_ram_datal(dev), 0xcafec0de);
    // reading datal increments addr
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 201);

    // datah is masked to 0x3f on r100 (0x1f on r128)
    ASSERT_EQ(rd_r100_cp_me_ram_datah(dev), 0x0000003f);
    ASSERT_EQ(rd_r100_cp_me_ram_datal(dev), 0xffffffff);

    // Read address _always_ returns 0
    ASSERT_EQ(rd_r100_cp_me_ram_raddr(dev), 0);

    // Write addr incremented by reads
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 202);

    /*
     * Test that ADDR and RADDR control separate pointers for reads.
     *
     * RADDR sets both the visible ADDR register and an internal read pointer.
     * ADDR only sets the visible ADDR register, leaving the internal read
     * pointer unchanged. The pointers re-sync after reading DATAL (writes
     * do not sync).
     */
    // write known values at indices 10 and 50
    wr_r100_cp_me_ram_addr(dev, 10);
    wr_r100_cp_me_ram_datah(dev, 0x0a);
    wr_r100_cp_me_ram_datal(dev, 0x10101010);

    wr_r100_cp_me_ram_addr(dev, 50);
    wr_r100_cp_me_ram_datah(dev, 0x0b);
    wr_r100_cp_me_ram_datal(dev, 0x50505050);

    // set read pointer to index 10 using RADDR
    wr_r100_cp_me_ram_raddr(dev, 10);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 10);

    // read index 10
    ASSERT_EQ(rd_r100_cp_me_ram_datah(dev), 0x0a);
    ASSERT_EQ(rd_r100_cp_me_ram_datal(dev), 0x10101010);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 11);

    // now write ADDR=50, this should _not_ affect the internal read pointer
    wr_r100_cp_me_ram_addr(dev, 50);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 50);

    // DATAH read still comes from internal pointer (index 11), not ADDR (50)
    uint32_t datah_at_11 = rd_r100_cp_me_ram_datah(dev);
    // ADDR unchanged after DATAH read
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 50);

    // DATAL read comes from internal pointer (index 11), then syncs to ADDR+1
    uint32_t datal_at_11 = rd_r100_cp_me_ram_datal(dev);
    // after DATAL read, ADDR increments and internal pointer syncs
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 51);

    // verify we did _not_ read index 50's data
    ASSERT_TRUE(datah_at_11 != 0x0b);
    ASSERT_TRUE(datal_at_11 != 0x50505050);

    // now reads proceed from index 51 (synced after first DATAL read)
    uint32_t datah_at_51 = rd_r100_cp_me_ram_datah(dev);
    uint32_t datal_at_51 = rd_r100_cp_me_ram_datal(dev);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 52);

    // verify by reading index 51 properly via RADDR
    wr_r100_cp_me_ram_raddr(dev, 51);
    ASSERT_EQ(rd_r100_cp_me_ram_datah(dev), datah_at_51);
    ASSERT_EQ(rd_r100_cp_me_ram_datal(dev), datal_at_51);

    /*
     * Test that DATAL writes do NOT sync the internal read pointer.
     */
    // write known values at indices 80 and 81
    wr_r100_cp_me_ram_addr(dev, 80);
    wr_r100_cp_me_ram_datah(dev, 0x08);
    wr_r100_cp_me_ram_datal(dev, 0x80808080);

    wr_r100_cp_me_ram_addr(dev, 81);
    wr_r100_cp_me_ram_datah(dev, 0x18);
    wr_r100_cp_me_ram_datal(dev, 0x81818181);

    // set read pointer to 80, read to advance internal pointer to 81
    wr_r100_cp_me_ram_raddr(dev, 80);
    ASSERT_EQ(rd_r100_cp_me_ram_datal(dev), 0x80808080);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 81);

    // set ADDR=200, internal read pointer still at 81
    wr_r100_cp_me_ram_addr(dev, 200);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 200);

    // write to index 200, DATAL write does NOT sync internal pointer
    wr_r100_cp_me_ram_datah(dev, 0x1f);
    wr_r100_cp_me_ram_datal(dev, 0xdeadbeef);
    ASSERT_EQ(rd_r100_cp_me_ram_addr(dev), 201);

    // read still comes from internal pointer at 81, not from ADDR (201)
    uint32_t datah_after_write = rd_r100_cp_me_ram_datah(dev);
    uint32_t datal_after_write = rd_r100_cp_me_ram_datal(dev);

    // verify we got index 81's data (write did not sync)
    ASSERT_EQ(datah_after_write, 0x18);
    ASSERT_EQ(datal_after_write, 0x81818181);

    return true;
}

bool test_r100_ring_buffer_setup(ati_device_t *dev) {
    wr_bios_0_scratch(dev, 0);
    wr_bios_1_scratch(dev, 0);
    wr_bios_2_scratch(dev, 0);

    ati_init_cce_engine(dev, R100_CSQ_MODE_BM);
    ati_r100_cce_wait_for_idle(dev);
    uint32_t gart_addr = ati_r100_init_pci_gart(dev);
    ati_r100_cce_wait_for_idle(dev);

    // Place the ring buffer at the beginning of the gart
    wr_r100_cp_rb_base(dev, gart_addr);
    ati_r100_cce_wait_for_idle(dev);
    wr_r100_cp_rb_wptr_delay(dev, 0);

    // Reset/empty the ring buffer (wptr = rptr)
    wr_r100_cp_rb_rptr_wr(dev, 0x0);
    wr_r100_cp_rb_wptr(dev, 0x0);
    // Ring buffer size of 2^1 qwords or 4 dwords to test wrap
    wr_r100_cp_rb_cntl(dev, 1);
    // RPTR writeback placed in memory. Oddly this doesn't seem to go through
    // the memory controller and from testing requires a physical address.
    wr_r100_cp_rb_rptr_addr(dev, (uint32_t)&mem[0]);

    // Wait for writeback to flush
    ati_r100_cce_wait_for_idle(dev);
    // Clear the writeback memory before running the test
    memset((void *)mem, 0, sizeof(mem));
    ASSERT_EQ(mem[0], 0);

    // Now queue up packets for testing
    gart_mem[0] = CCE_PKT0(BIOS_0_SCRATCH, 1);
    gart_mem[1] = 0xcafebabe;
    // And process them
    wr_r100_cp_rb_wptr(dev, 2);
    // Wait for packets to be processed
    ati_r100_cce_wait_for_idle(dev);
    // The rptr advanced
    ASSERT_EQ(rd_r100_cp_rb_rptr(dev), 2);
    udelay(1000);
    // and it was written back
    ASSERT_EQ(mem[0], 2);

    gart_mem[2] = CCE_PKT0(BIOS_1_SCRATCH, 1);
    gart_mem[3] = 0x1337beef;
    // And process them
    wr_r100_cp_rb_wptr(dev, 0);
    // Wait for packets to be processed
    ati_r100_cce_wait_for_idle(dev);
    // The rptr advanced and wrapped
    ASSERT_EQ(rd_r100_cp_rb_rptr(dev), 0);
    // and it was written back
    ASSERT_EQ(mem[0], 0);

    // Wrap and queue up another packet
    gart_mem[0] = CCE_PKT0(BIOS_2_SCRATCH, 1);
    gart_mem[1] = 0xbeefcafe;
    // And process them
    wr_r100_cp_rb_wptr(dev, 2);
    // Wait for packets to be processed and writeback to flush
    ati_r100_cce_wait_for_idle(dev);

    // The packets were all processed!
    ASSERT_EQ(rd_bios_0_scratch(dev), 0xcafebabe);
    ASSERT_EQ(rd_bios_1_scratch(dev), 0x1337beef);
    ASSERT_EQ(rd_bios_2_scratch(dev), 0xbeefcafe);

    // The rptr advanced
    ASSERT_EQ(rd_r100_cp_rb_rptr(dev), 2);
    // and it was written back
    ASSERT_EQ(mem[0], 2);

    ati_stop_cce_engine(dev);
    ati_r100_cce_wait_for_idle(dev);

    return true;
}

bool
test_r100_indirect_buffer(ati_device_t *dev) {
    wr_bios_0_scratch(dev, 0);
    wr_bios_1_scratch(dev, 0);
    wr_bios_2_scratch(dev, 0);

    ati_init_cce_engine(dev, R100_CSQ_MODE_PIO_INDBM);
    ati_r100_cce_wait_for_idle(dev);
    uint32_t gart_addr = ati_r100_init_pci_gart(dev);
    ati_r100_cce_wait_for_idle(dev);

    ASSERT_EQ(rd_bios_0_scratch(dev), 0);

    gart_mem[0] = CCE_PKT0(BIOS_0_SCRATCH, 1);
    gart_mem[1] = 0xdeadbeef;
    gart_mem[2] = CCE_PKT0(BIOS_1_SCRATCH, 2);
    gart_mem[3] = 0xbeefcafe;
    gart_mem[4] = 0xfeedbeef;
    gart_mem[5] = CCE_PKT2();
    gart_mem[6] = CCE_PKT2();
    gart_mem[7] = CCE_PKT2();
    wr_r100_cp_ib_base(dev, gart_addr);
    wr_r100_cp_ib_bufsz(dev, 8);
    ati_r100_cce_wait_for_idle(dev);

    ASSERT_EQ(rd_bios_0_scratch(dev), 0xdeadbeef);
    ASSERT_EQ(rd_bios_1_scratch(dev), 0xbeefcafe);
    ASSERT_EQ(rd_bios_2_scratch(dev), 0xfeedbeef);

    return true;
}

void
register_r100_cce_tests(void)
{
    REGISTER_TEST_FOR(test_r100_cce, "cce", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_cce_setup, "cce setup", CHIP_R100);
    //REGISTER_TEST_FOR(test_r100_cce_packet_submission, "cce packet submission", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_cce_mm_indirect, "cce MM_INDEX and MM_DATA", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_microcode, "microcode", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_ring_buffer_setup, "ring buffer setup", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_indirect_buffer, "indirect buffer", CHIP_R100);
}
