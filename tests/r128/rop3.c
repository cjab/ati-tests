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

static bool
do_mem_clipping(ati_device_t *dev, int size, int border,
                horiz_dir_t hdir, vert_dir_t vdir)
{
    char fixture[128];
    const char *hdir_str = (hdir == LEFT_TO_RIGHT) ? "ltr" : "rtl";
    const char *vdir_str = (vdir == TOP_TO_BOTTOM) ? "ttb" : "btt";
    int margin = 10;
    int clip = 3;
    int src_x = 100;
    int src_y = 100;

    int top            = margin;
    int left           = margin;
    int bottom         = margin + size - 1;
    int right          = margin + size - 1;
    int top_clipped    = margin + clip;
    int left_clipped   = margin + clip;
    int bottom_clipped = margin + size - 1 - clip;
    int right_clipped  = margin + size - 1 - clip;

    uint32_t dp_cntl = (hdir == LEFT_TO_RIGHT ? 0x1 : 0x0) |
                       (vdir == TOP_TO_BOTTOM ? 0x2 : 0x0);
    int sx = (hdir == LEFT_TO_RIGHT) ? src_x : src_x + size - 1;
    int sy = (vdir == TOP_TO_BOTTOM) ? src_y : src_y + size - 1;
    int dx = (hdir == LEFT_TO_RIGHT) ? margin : margin + size - 1;
    int dy = (vdir == TOP_TO_BOTTOM) ? margin : margin + size - 1;
    uint32_t dst_width_height = (size << 16) | size;

    struct { const char *name; int top; int left; int bottom; int right; } cases[] = {
        {"no_clip",         top,         left,         bottom,         right},
        {"clip_right",      top,         left,         bottom,         right_clipped},
        {"clip_left",       top,         left_clipped, bottom,         right},
        {"clip_left_right", top,         left_clipped, bottom,         right_clipped},
        {"clip_top",        top_clipped, left,         bottom,         right},
        {"clip_bottom",     top,         left,         bottom_clipped, right},
        {"clip_top_bottom", top_clipped, left,         bottom_clipped, right},
        {"clip_all",        top_clipped, left_clipped, bottom_clipped, right_clipped},
    };

    wr_dp_cntl(dev, dp_cntl);

    /* Completely clipped — scissor far from destination */
    ati_screen_clear(dev, 0);
    draw_box(dev, size, border, src_x, src_y);
    wr_sc_top_left(dev, ((bottom + 100) << 16) | (right + 100));
    wr_sc_bottom_right(dev, ((bottom + 200) << 16) | (right + 200));
    wr_src_x_y(dev, (sx << 16) | sy);
    wr_dst_x(dev, dx);
    wr_dst_y(dev, dy);
    wr_dst_width_height(dev, dst_width_height);
    snprintf(fixture, sizeof(fixture),
             "mem_%s_%s_completely_clipped_%dx%d",
             hdir_str, vdir_str, size, size);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, fixture));

    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
        ati_screen_clear(dev, 0);
        draw_box(dev, size, border, src_x, src_y);
        wr_sc_top_left(dev, (cases[i].top << 16) | cases[i].left);
        wr_sc_bottom_right(dev, (cases[i].bottom << 16) | cases[i].right);
        wr_src_x_y(dev, (sx << 16) | sy);
        wr_dst_x(dev, dx);
        wr_dst_y(dev, dy);
        wr_dst_width_height(dev, dst_width_height);
        snprintf(fixture, sizeof(fixture),
                 "mem_%s_%s_%s_%dx%d",
                 hdir_str, vdir_str, cases[i].name, size, size);
        ASSERT_TRUE(ati_screen_compare_fixture(dev, fixture));
    }

    return true;
}

bool
test_r128_mem_blit_clipping(ati_device_t *dev)
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

    ASSERT_TRUE(do_mem_clipping(dev, size, border,
                                LEFT_TO_RIGHT, TOP_TO_BOTTOM));
    ASSERT_TRUE(do_mem_clipping(dev, size, border,
                                LEFT_TO_RIGHT, BOTTOM_TO_TOP));
    ASSERT_TRUE(do_mem_clipping(dev, size, border,
                                RIGHT_TO_LEFT, TOP_TO_BOTTOM));
    ASSERT_TRUE(do_mem_clipping(dev, size, border,
                                RIGHT_TO_LEFT, BOTTOM_TO_TOP));

    return true;
}

void
register_r128_rop3_tests(void)
{
    REGISTER_TEST(test_r128_rop3_16x16, "r128 rop3 16x16");
    REGISTER_TEST(test_r128_overlapping_mem_blit, "r128 overlapping mem blit");
    REGISTER_TEST(test_r128_mem_blit_clipping, "r128 mem blit clipping");
}
