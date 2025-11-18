/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../ati.h"
#include "../common.h"

bool
test_src_pitch_offset_cntl_latching(ati_device_t *dev)
{
    uint32_t dp_gui_master_cntl;

    // Setting initial state
    wr_dp_gui_master_cntl(dev, 0x0);
    wr_default_offset(dev, 0x000000aa);
    wr_default_pitch(dev, 0x000000bb);
    wr_src_offset(dev, 0x00000011);
    wr_src_pitch(dev, 0x00000022);

    ASSERT_EQ(rd_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_src_offset(dev), 0x00000010);
    ASSERT_EQ(rd_src_pitch(dev), 0x00000022);

    // Set GMC_SRC_PITCH_OFFSET_CNTL to default
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl & ~0x1);

    // src_offset and src_pitch should latch default
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl & ~0x1);
    ASSERT_EQ(rd_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_src_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_src_pitch(dev), 0x000000bb);

    // Set GMC_SRC_PITCH_OFFSET_CNTL to leave alone
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl | 0x1);

    // src_offset and src_pitch should remain default
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl | 0x1);
    ASSERT_EQ(rd_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_src_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_src_pitch(dev), 0x000000bb);

    return true;
}

bool
test_dst_pitch_offset_cntl_latching(ati_device_t *dev)
{
    uint32_t dp_gui_master_cntl;

    // Setting initial state
    wr_dp_gui_master_cntl(dev, 0x0);
    wr_default_offset(dev, 0x000000a0);
    wr_default_pitch(dev, 0x000000bb);
    wr_dst_offset(dev, 0x00000010);
    wr_dst_pitch(dev, 0x00000022);

    ASSERT_EQ(rd_dp_gui_master_cntl(dev), 0x00000000);
    ASSERT_EQ(rd_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_dst_offset(dev), 0x00000010);
    ASSERT_EQ(rd_dst_pitch(dev), 0x00000022);

    // Set GMC_DST_PITCH_OFFSET_CNTL to default
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl & ~0x2);

    // dst_offset and dst_pitch should latch default
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl & ~0x2);
    ASSERT_EQ(rd_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_dst_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_dst_pitch(dev), 0x000000bb);

    // Set GMC_DST_PITCH_OFFSET_CNTL to leave alone
    dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
    wr_dp_gui_master_cntl(dev, dp_gui_master_cntl | 0x2);

    // dst_offset and dst_pitch should remain default
    ASSERT_EQ(rd_dp_gui_master_cntl(dev), dp_gui_master_cntl | 0x2);
    ASSERT_EQ(rd_default_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_default_pitch(dev), 0x000000bb);
    ASSERT_EQ(rd_dst_offset(dev), 0x000000a0);
    ASSERT_EQ(rd_dst_pitch(dev), 0x000000bb);

    return true;
}

void
register_pitch_offset_cntl_tests(void)
{
    register_test("SRC pitch offset latches",
                  test_src_pitch_offset_cntl_latching);
    register_test("DST pitch offset latches",
                  test_dst_pitch_offset_cntl_latching);
}
