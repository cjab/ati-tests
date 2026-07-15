/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

bool
test_r100_src_pitch_offset_cntl_latching(ati_device_t *dev)
{
    // Setting initial state
    wr_r100_dp_gui_master_cntl(dev, 0x0);

    wr_r100_default_pitch_offset(dev, 0xffffffff);
    wr_src_offset(dev, 0x00000011);
    wr_src_pitch(dev, 0x00000022);

    ASSERT_EQ(rd_r100_default_pitch_offset(dev), 0xffffffff);
    ASSERT_EQ(rd_src_offset(dev), 0x00000010);
    ASSERT_EQ(rd_src_pitch(dev), 0x00000020);

    // Set GMC_SRC_PITCH_OFFSET_CNTL to default
    wr_r100_dp_gui_master_cntl(dev, 0x0);

    // src_offset and src_pitch do NOT latch to default
    ASSERT_EQ(rd_r100_default_pitch_offset(dev), 0xffffffff);
    ASSERT_EQ(rd_src_offset(dev), 0x00000010);
    ASSERT_EQ(rd_src_pitch(dev), 0x00000020);

    // Set GMC_SRC_PITCH_OFFSET_CNTL to leave alone
    wr_r100_dp_gui_master_cntl(dev, 0x1);

    // src_offset and src_pitch should not change
    ASSERT_EQ(rd_r100_default_pitch_offset(dev), 0xffffffff);
    ASSERT_EQ(rd_src_offset(dev), 0x00000010);
    ASSERT_EQ(rd_src_pitch(dev), 0x00000020);

    return true;
}

bool
test_r100_dst_pitch_offset_cntl_latching(ati_device_t *dev)
{
    // Setting initial state
    wr_r100_dp_gui_master_cntl(dev, 0x0);
    wr_r100_default_pitch_offset(dev, 0xffffffff);
    wr_dst_offset(dev, 0x00000011);
    wr_dst_pitch(dev, 0x00000022);

    ASSERT_EQ(rd_r100_default_pitch_offset(dev), 0xffffffff);
    ASSERT_EQ(rd_dst_offset(dev), 0x00000010);
    ASSERT_EQ(rd_dst_pitch(dev), 0x00000020);

    // Set GMC_DST_PITCH_OFFSET_CNTL to default
    wr_r100_dp_gui_master_cntl(dev, 0x0);

    // dst_offset and dst_pitch should NOT latch default
    ASSERT_EQ(rd_r100_default_pitch_offset(dev), 0xffffffff);
    ASSERT_EQ(rd_dst_offset(dev), 0x00000010);
    ASSERT_EQ(rd_dst_pitch(dev), 0x00000020);

    // Set GMC_DST_PITCH_OFFSET_CNTL to leave alone
    wr_r100_dp_gui_master_cntl(dev, 0x2);

    // dst_offset and dst_pitch should not change
    ASSERT_EQ(rd_r100_default_pitch_offset(dev), 0xffffffff);
    ASSERT_EQ(rd_dst_offset(dev), 0x00000010);
    ASSERT_EQ(rd_dst_pitch(dev), 0x00000020);

    return true;
}

bool
test_r100_dst_clipping_latching(ati_device_t *dev)
{
    // Setting initial state
    wr_r100_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x22221111);
    wr_sc_top_left(dev, 0x000a000b);
    wr_sc_bottom_right(dev, 0x000c000d);

    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_sc_top(dev), 0x0000000a);
    ASSERT_EQ(rd_sc_left(dev), 0x0000000b);
    ASSERT_EQ(rd_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_sc_right(dev), 0x0000000d);

    // Set GMC_DST_PITCH_OFFSET_CNTL to default
    wr_r100_dp_gui_master_cntl(dev, 0x0);

    // dst_offset and dst_pitch should NOT latch default
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_sc_top(dev), 0x0000000a);
    ASSERT_EQ(rd_sc_left(dev), 0x0000000b);
    ASSERT_EQ(rd_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_sc_right(dev), 0x0000000d);

    // Set GMC_DST_PITCH_OFFSET_CNTL to leave alone
    wr_r100_dp_gui_master_cntl(dev, 0x2);

    // dst_offset and dst_pitch should not change
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_sc_top(dev), 0x0000000a);
    ASSERT_EQ(rd_sc_left(dev), 0x0000000b);
    ASSERT_EQ(rd_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_sc_right(dev), 0x0000000d);

    return true;
}

bool
test_r100_src_clipping_latching(ati_device_t *dev)
{
    // Setting initial state
    wr_r100_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x22221111);
    wr_src_sc_bottom_right(dev, 0x000c000d);

    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_src_sc_right(dev), 0x0000000d);

    // Set GMC_DST_PITCH_OFFSET_CNTL to default
    wr_r100_dp_gui_master_cntl(dev, 0x0);

    // dst_offset and dst_pitch should NOT latch default
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_src_sc_right(dev), 0x0000000d);

    // Set GMC_DST_PITCH_OFFSET_CNTL to leave alone
    wr_r100_dp_gui_master_cntl(dev, 0x2);

    // dst_offset and dst_pitch should not change
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_src_sc_right(dev), 0x0000000d);

    return true;
}

void
register_r100_pitch_offset_cntl_tests(void)
{
    REGISTER_TEST_FOR(test_r100_src_pitch_offset_cntl_latching,
                  "SRC pitch offset does not latch", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_dst_pitch_offset_cntl_latching,
                  "DST pitch offset does not latch", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_dst_clipping_latching,
                  "DST clipping does not latch", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_src_clipping_latching,
                  "SRC clipping does not latch", CHIP_R100);
}
