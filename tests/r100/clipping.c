/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

bool
test_r100_src_clipping_latches(ati_device_t *dev)
{
    uint32_t r100_dp_gui_master_cntl;

    // Setting initial state
    wr_r100_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
    wr_src_sc_bottom(dev, 0x00000111);
    wr_src_sc_right(dev, 0x00000222);
    ASSERT_EQ(rd_r100_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00000222);

    // Set GMC_SRC_CLIPPING to default
    r100_dp_gui_master_cntl = rd_r100_dp_gui_master_cntl(dev);
    wr_r100_dp_gui_master_cntl(dev, r100_dp_gui_master_cntl & ~0x4);

    // On r100 src_sc_bottom and src_sc_right do NOT latch default
    ASSERT_EQ(rd_r100_dp_gui_master_cntl(dev), r100_dp_gui_master_cntl & ~0x4);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00000222);

    // Set GMC_SRC_CLIPPING to leave alone
    r100_dp_gui_master_cntl = rd_r100_dp_gui_master_cntl(dev);
    wr_r100_dp_gui_master_cntl(dev, r100_dp_gui_master_cntl | 0x4);

    // src_sc_bottom and src_sc_right should remain default
    //TODO: r100_dp_gui_master_cntl is read-only on r100!
    //ASSERT_EQ(rd_r100_dp_gui_master_cntl(dev), r100_dp_gui_master_cntl | 0x4);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00000222);

    return true;
}

bool
test_r100_dst_clipping_latches(ati_device_t *dev)
{
    uint32_t r100_dp_gui_master_cntl;

    // Setting initial state
    wr_r100_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
    wr_sc_bottom(dev, 0x00000111);
    wr_sc_right(dev, 0x00000222);
    wr_sc_top(dev, 0x00000333);
    wr_sc_left(dev, 0x00000444);

    ASSERT_EQ(rd_r100_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_sc_right(dev), 0x00000222);
    ASSERT_EQ(rd_sc_top(dev), 0x00000333);
    ASSERT_EQ(rd_sc_left(dev), 0x00000444);

    // Set GMC_DST_CLIPPING to default
    r100_dp_gui_master_cntl = rd_r100_dp_gui_master_cntl(dev);
    wr_r100_dp_gui_master_cntl(dev, r100_dp_gui_master_cntl & ~0x8);

    // On r100 sc_bottom and sc_right do NOT latch default
    // and sc_top and sc_left are NOT set to the origin
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_sc_right(dev), 0x00000222);
    ASSERT_EQ(rd_sc_top(dev), 0x00000333);
    ASSERT_EQ(rd_sc_left(dev), 0x00000444);

    // Set GMC_DST_CLIPPING to leave alone
    r100_dp_gui_master_cntl = rd_r100_dp_gui_master_cntl(dev);
    wr_r100_dp_gui_master_cntl(dev, r100_dp_gui_master_cntl | 0x8);

    // sc_bottom, src_right, sc_top, and sc_left should remain as they were
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_sc_right(dev), 0x00000222);
    ASSERT_EQ(rd_sc_top(dev), 0x00000333);
    ASSERT_EQ(rd_sc_left(dev), 0x00000444);

    return true;
}


void
register_r100_clipping_tests(void)
{
    REGISTER_TEST_FOR(test_r100_src_clipping_latches, "SRC clipping does not latch", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_dst_clipping_latches, "DST clipping does not latch", CHIP_R100);
}
