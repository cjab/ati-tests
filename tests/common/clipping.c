/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

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
    REGISTER_TEST(test_reserved_scissor_bits, "reserved scissor bits");
}
