/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

bool
test_src_clipping_latches(ati_device_t *dev)
{
    uint32_t dp_gui_master_cntl;

    // Setting initial state
    wr_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
    wr_src_sc_bottom(dev, 0x00000111);
    wr_src_sc_right(dev, 0x00000222);

    ASSERT_EQ(rd_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00000222);

    // Set GMC_SRC_CLIPPING to default
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl & ~0x4);

    // src_sc_bottom and src_sc_right should latch default
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl & ~0x4);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00000aaa);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00000bbb);

    // Set GMC_SRC_CLIPPING to leave alone
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl | 0x4);

    // src_sc_bottom and src_sc_right should remain default
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl | 0x4);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00000aaa);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00000bbb);

    return true;
}

bool
test_dst_clipping_latches(ati_device_t *dev)
{
    uint32_t dp_gui_master_cntl;

    // Setting initial state
    wr_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
    wr_sc_bottom(dev, 0x00000111);
    wr_sc_right(dev, 0x00000222);
    wr_sc_top(dev, 0x00000333);
    wr_sc_left(dev, 0x00000444);

    ASSERT_EQ(rd_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00000111);
    ASSERT_EQ(rd_sc_right(dev), 0x00000222);
    ASSERT_EQ(rd_sc_top(dev), 0x00000333);
    ASSERT_EQ(rd_sc_left(dev), 0x00000444);

    // Set GMC_DST_CLIPPING to default
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl & ~0x8);

    // sc_bottom and src_right should latch default
    // sc_top and sc_left are set to the origin
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl & ~0x8);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00000aaa);
    ASSERT_EQ(rd_sc_right(dev), 0x00000bbb);
    ASSERT_EQ(rd_sc_top(dev), 0x00000000);
    ASSERT_EQ(rd_sc_left(dev), 0x00000000);

    // Set GMC_DST_CLIPPING to leave alone
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl | 0x8);

    // sc_bottom and src_right should remain default
    // sc_top and sc_left remain at the origin
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl | 0x8);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x0aaa0bbb);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00000aaa);
    ASSERT_EQ(rd_sc_right(dev), 0x00000bbb);
    ASSERT_EQ(rd_sc_top(dev), 0x00000000);
    ASSERT_EQ(rd_sc_left(dev), 0x00000000);

    return true;
}

bool
test_reserved_scissor_bits(ati_device_t *dev)
{
    // Setting initial state
    wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
    wr_sc_bottom(dev, 0x0);
    wr_sc_right(dev, 0x0);
    wr_sc_top(dev, 0x0);
    wr_sc_left(dev, 0x0);

    ASSERT_EQ(rd_sc_bottom(dev), 0x00000000);
    ASSERT_EQ(rd_sc_right(dev), 0x00000000);
    ASSERT_EQ(rd_sc_top(dev), 0x00000000);
    ASSERT_EQ(rd_sc_left(dev), 0x00000000);

    wr_sc_bottom(dev, 0xffffffff);
    wr_sc_top(dev, 0xffffffff);

    ASSERT_EQ(rd_sc_bottom(dev), 0x00003fff);
    ASSERT_EQ(rd_sc_right(dev), 0x00000000);
    ASSERT_EQ(rd_sc_top(dev), 0x00003fff);
    ASSERT_EQ(rd_sc_left(dev), 0x00000000);

    wr_sc_right(dev, 0xffffffff);
    wr_sc_left(dev, 0xffffffff);

    ASSERT_EQ(rd_sc_bottom(dev), 0x00003fff);
    ASSERT_EQ(rd_sc_right(dev), 0x00003fff);
    ASSERT_EQ(rd_sc_top(dev), 0x00003fff);
    ASSERT_EQ(rd_sc_left(dev), 0x00003fff);

    wr_sc_bottom_right(dev, 0xfeeefeee);
    wr_sc_top_left(dev, 0xfeeefeee);

    ASSERT_EQ(rd_sc_bottom(dev), 0x00003eee);
    ASSERT_EQ(rd_sc_right(dev), 0x00003eee);
    ASSERT_EQ(rd_sc_top(dev), 0x00003eee);
    ASSERT_EQ(rd_sc_left(dev), 0x00003eee);

    return true;
}

void
register_clipping_tests(void)
{
    REGISTER_TEST(test_src_clipping_latches, "SRC clipping latches");
    REGISTER_TEST(test_dst_clipping_latches, "DST clipping latches");
    REGISTER_TEST(test_reserved_scissor_bits, "reserved scissor bits");
}
