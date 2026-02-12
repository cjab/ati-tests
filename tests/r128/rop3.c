/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

// clang-format off
#define GMC_SRC_DATATYPE_COLOR          0x00003000
#define GMC_SRC_SOURCE_MEMORY           0x02000000
#define GMC_DST_32BPP                   0x00000600
#define GMC_ROP3_SRCCOPY                0x00cc0000
#define GMC_BRUSH_NONE                  0x000000f0
#define GMC_BRUSH_SOLIDCOLOR            0x000000d0
#define GMC_DST_PITCH_OFFSET_LEAVE      0x00000002
#define GMC_BYTE_LSB_TO_MSB             0x00004000
// clang-format on

static void
draw_box(ati_device_t *dev, int size, int border, int x0, int y0)
{
    static const uint32_t BORDER = 0x00cc3355;
    static const uint32_t TRIANGLE_FILL = 0x0055cc33;
    static const uint32_t BACKGROUND_FILL = 0x003355cc;

    int bypp = 4;
    int pitch = (rd_dst_pitch(dev) & 0x1ff) * 8 * bypp ;
    int marker_size = border;
    int marker_gap = border;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int offset = ((y0 + y) * pitch) + ((x0 + x) * sizeof(uint32_t));
            bool is_border = (y < border || y >= size - border ||
                              x < border || x >= size - border);
            bool is_triangle = x <= y;
            bool is_marker = (!is_border &&
                              x >= size - border - marker_size - marker_gap &&
                              x < size - border - marker_gap &&
                              y >= border + marker_gap &&
                              y < border + marker_size + marker_gap);
            uint32_t pixel =  is_border                ? BORDER :
                              is_triangle || is_marker ? TRIANGLE_FILL :
                                                         BACKGROUND_FILL;
            ati_vram_write(dev, offset, pixel);
        }
    }
}

bool
test_r128_rop3_16x16(ati_device_t *dev)
{
    ati_screen_clear(dev, 0);

    int size = 16;
    int border = 2;

    wr_src_x_y(dev, 0x0);

    wr_r128_default_offset(dev, 0x0);
    wr_r128_default_pitch(dev, 0x50);

    draw_box(dev, size, border, 0, 0);

    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_dp_write_msk(dev, 0xffffffff);

    wr_dp_gui_master_cntl(dev,
        GMC_BRUSH_NONE | GMC_DST_32BPP | GMC_SRC_DATATYPE_COLOR |
        GMC_BYTE_LSB_TO_MSB | GMC_ROP3_SRCCOPY | GMC_SRC_SOURCE_MEMORY);

    wr_dst_x(dev, 0x10);
    wr_dst_y(dev, 0x10);
    wr_dst_width_height(dev, (size << 16) | size);

    ASSERT_TRUE(ati_screen_compare_fixture(dev, "rop3_color_16x16"));
    /* The dst_x and dst_y registers are not updated */
    ASSERT_EQ(rd_dst_x(dev), 0x10);
    ASSERT_EQ(rd_dst_y(dev), 0x10);

    return true;
}

bool
test_r128_overlapping_mem_blit(ati_device_t *dev)
{
    int size = 32;
    int border = 4;

    wr_r128_default_offset(dev, 0x0);
    wr_r128_default_pitch(dev, 0x50);
    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_dp_write_msk(dev, 0xffffffff);

    wr_dp_gui_master_cntl(dev,
        GMC_BRUSH_NONE | GMC_DST_32BPP | GMC_SRC_DATATYPE_COLOR |
        GMC_BYTE_LSB_TO_MSB | GMC_ROP3_SRCCOPY | GMC_SRC_SOURCE_MEMORY);

    // Left to right, top to bottom
    ati_screen_clear(dev, 0);
    draw_box(dev, size, border, 0, 0);
    wr_dp_cntl(dev, 0x3);
    wr_src_x_y(dev, 0x0);
    wr_dst_x(dev, size / 2);
    wr_dst_y(dev, size / 2);
    wr_dst_width_height(dev, (size << 16) | size);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, "overlapping_ltr_ttb"));

    // Left to right, bottom to top
    ati_screen_clear(dev, 0);
    draw_box(dev, size, border, 0, 0);
    wr_dp_cntl(dev, 0x1);
    wr_src_x_y(dev, size - 1);
    wr_dst_x(dev, size / 2);
    wr_dst_y(dev, (size / 2) + size);
    wr_dst_width_height(dev, (size << 16) | size);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, "overlapping_ltr_btt"));

    // Right to left, bottom to top
    ati_screen_clear(dev, 0);
    draw_box(dev, size, border, 0, 0);
    wr_dp_cntl(dev, 0x0);
    wr_src_x_y(dev, ((size - 1) << 16)| (size - 1));
    wr_dst_x(dev, (size / 2) + size);
    wr_dst_y(dev, (size / 2) + size);
    wr_dst_width_height(dev, (size << 16) | size);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, "overlapping_rtl_btt"));

    // Right to left, top to bottom
    ati_screen_clear(dev, 0);
    draw_box(dev, size, border, 0, 0);
    wr_dp_cntl(dev, 0x2);
    wr_src_x_y(dev, ((size - 1) << 16) | 0);
    wr_dst_x(dev, (size / 2) + size);
    wr_dst_y(dev, (size / 2));
    wr_dst_width_height(dev, (size << 16) | size);
    // FIXME: This is broken in QEMU, meaning it should display a
    //        rendering artifact in the bottom right corner of the
    //        new box but it does not.
    //ASSERT_TRUE(ati_screen_compare_fixture(dev, "overlapping_rtl_ttb"));

    return true;
}

void
register_r128_rop3_tests(void)
{
    REGISTER_TEST(test_r128_rop3_16x16, "r128 rop3 16x16");
    REGISTER_TEST(test_r128_overlapping_mem_blit, "r128 overlapping mem blit");
}
