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
    if (ati_get_chip_family(dev) != CHIP_R100) {
        printf("  (skipped - R100 only)\n");
        return true;
    }

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

void
register_r100_pitch_offset_cntl_tests(void)
{
    REGISTER_TEST_FOR(test_r100_src_pitch_offset_cntl_latching,
                  "SRC pitch offset latches", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_dst_pitch_offset_cntl_latching,
                  "DST pitch offset latches", CHIP_R100);
}
