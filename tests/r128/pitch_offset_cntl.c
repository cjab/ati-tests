/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

// These tests are R128-specific as they test the separate DEFAULT_OFFSET
// and DEFAULT_PITCH registers. On R100, these are combined into a single
// DEFAULT_PITCH_OFFSET register with different bit layouts.

bool
test_src_pitch_offset_cntl_latching(ati_device_t *dev)
{
    uint32_t r128_dp_gui_master_cntl;

    // Setting initial state
    wr_r128_dp_gui_master_cntl(dev, 0x0);
    wr_r128_default_offset(dev, 0x000000aa);
    wr_r128_default_pitch(dev, 0x000000bb);
    wr_src_offset(dev, 0x00000011);
    wr_src_pitch(dev, 0x00000022);

    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_r128_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_r128_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_src_offset(dev), 0x00000010);
    ASSERT_EQ(rd_src_pitch(dev), 0x00000022);

    // Set GMC_SRC_PITCH_OFFSET_CNTL to default
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl & ~0x1);

    // src_offset and src_pitch should latch default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), r128_dp_gui_master_cntl & ~0x1);
    ASSERT_EQ(rd_r128_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_r128_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_src_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_src_pitch(dev), 0x000000bb);

    // Set GMC_SRC_PITCH_OFFSET_CNTL to leave alone
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl | 0x1);

    // src_offset and src_pitch should remain default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), r128_dp_gui_master_cntl | 0x1);
    ASSERT_EQ(rd_r128_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_r128_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_src_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_src_pitch(dev), 0x000000bb);

    return true;
}

bool
test_dst_pitch_offset_cntl_latching(ati_device_t *dev)
{
    if (ati_get_chip_family(dev) != CHIP_R128) {
        printf("  (skipped - R128 only)\n");
        return true;
    }

    uint32_t r128_dp_gui_master_cntl;

    // Setting initial state
    wr_r128_dp_gui_master_cntl(dev, 0x0);
    wr_r128_default_offset(dev, 0x000000a0);
    wr_r128_default_pitch(dev, 0x000000bb);
    wr_dst_offset(dev, 0x00000010);
    wr_dst_pitch(dev, 0x00000022);

    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_r128_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_r128_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_dst_offset(dev), 0x00000010);
    ASSERT_EQ(rd_dst_pitch(dev), 0x00000022);

    // Set GMC_DST_PITCH_OFFSET_CNTL to default
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl & ~0x2);

    // dst_offset and dst_pitch should latch default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), r128_dp_gui_master_cntl & ~0x2);
    ASSERT_EQ(rd_r128_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_r128_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_dst_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_dst_pitch(dev), 0x000000bb);

    // Set GMC_DST_PITCH_OFFSET_CNTL to leave alone
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl | 0x2);

    // dst_offset and dst_pitch should remain default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), r128_dp_gui_master_cntl | 0x2);
    ASSERT_EQ(rd_r128_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_r128_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_dst_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_dst_pitch(dev), 0x000000bb);

    return true;
}

bool
test_dst_clipping_latching(ati_device_t *dev)
{
    uint32_t r128_dp_gui_master_cntl;

    // Setting initial state
    wr_r128_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x22221111);
    wr_sc_top_left(dev, 0x000a000b);
    wr_sc_bottom_right(dev, 0x000c000d);

    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_sc_top(dev), 0x0000000a);
    ASSERT_EQ(rd_sc_left(dev), 0x0000000b);
    ASSERT_EQ(rd_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_sc_right(dev), 0x0000000d);

    // Set GMC_DST_CLIPPING to default
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl & ~R128_GMC_DST_CLIPPING);

    // sc_top, sc_left, sc_bottom and sc_right should latch default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_sc_top(dev), 0x00000000);
    ASSERT_EQ(rd_sc_left(dev), 0x00000000);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00002222);
    ASSERT_EQ(rd_sc_right(dev), 0x00001111);

    // Set GMC_DST_CLIPPING to leave alone
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl |
                               R128_GMC_DST_CLIPPING);

    // src_offset and src_pitch should remain default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev),
              r128_dp_gui_master_cntl | R128_GMC_DST_CLIPPING);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_sc_top(dev), 0x00000000);
    ASSERT_EQ(rd_sc_left(dev), 0x00000000);
    ASSERT_EQ(rd_sc_bottom(dev), 0x00002222);
    ASSERT_EQ(rd_sc_right(dev), 0x00001111);

    return true;
}

bool
test_src_clipping_latching(ati_device_t *dev)
{
    uint32_t r128_dp_gui_master_cntl;

    // Setting initial state
    wr_r128_dp_gui_master_cntl(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x22221111);
    wr_src_sc_bottom_right(dev, 0x000c000d);

    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x0000000c);
    ASSERT_EQ(rd_src_sc_right(dev), 0x0000000d);

    // Set GMC_DST_CLIPPING to default
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl & ~R128_GMC_DST_CLIPPING);

    // sc_top, sc_left, sc_bottom and sc_right should latch default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00002222);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00001111);

    // Set GMC_DST_CLIPPING to leave alone
    r128_dp_gui_master_cntl = rd_r128_dp_gui_master_cntl(dev);
    wr_r128_dp_gui_master_cntl(dev, r128_dp_gui_master_cntl |
                               R128_GMC_DST_CLIPPING);

    // src_offset and src_pitch should remain default
    ASSERT_EQ(rd_r128_dp_gui_master_cntl(dev),
              r128_dp_gui_master_cntl | R128_GMC_DST_CLIPPING);
    ASSERT_EQ(rd_default_sc_bottom_right(dev), 0x22221111);
    ASSERT_EQ(rd_src_sc_bottom(dev), 0x00002222);
    ASSERT_EQ(rd_src_sc_right(dev), 0x00001111);

    return true;
}

void
register_r128_pitch_offset_cntl_tests(void)
{
    REGISTER_TEST_FOR(test_src_pitch_offset_cntl_latching,
                  "SRC pitch offset latches", CHIP_R128);
    REGISTER_TEST_FOR(test_dst_pitch_offset_cntl_latching,
                  "DST pitch offset latches", CHIP_R128);
    REGISTER_TEST_FOR(test_dst_clipping_latching,
                  "DST clipping latches", CHIP_R128);
    REGISTER_TEST_FOR(test_src_clipping_latching,
                  "SRC clipping latches", CHIP_R128);
}
