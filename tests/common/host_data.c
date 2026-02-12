/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../ati/ati.h"
#include "../test.h"

// clang-format off
#define GMC_SRC_DATATYPE_COLOR          0x00030000
#define GMC_SRC_SOURCE_HOST_DATA        0x03000000
#define GMC_DST_32BPP                   0x00000600
#define GMC_ROP3_SRCCOPY                0x00cc0000
#define GMC_BRUSH_NONE                  0x000000f0
#define GMC_BRUSH_SOLIDCOLOR            0x000000d0
#define GMC_DST_PITCH_OFFSET_LEAVE      0x00000002
#define GMC_BYTE_LSB_TO_MSB             0x00004000
// clang-format on

static uint32_t
mono_box_word(int size, int border, int word_index)
{
    int bit_start = word_index * 32;
    int inset = border * 2;
    int content_start = inset;
    int content_end = size - inset;
    int content_size = content_end - content_start;
    int marker_size = border;
    uint32_t word = 0;

    for (int b = 0; b < 32; b++) {
        int pixel = bit_start + b;
        int y = pixel / size;
        int x = pixel % size;

        bool is_border = (y < border || y >= size - border ||
                          x < border || x >= size - border);

        int cx = x - content_start;
        int cy = y - content_start;
        bool in_content = (x >= content_start && x < content_end &&
                           y >= content_start && y < content_end);

        bool is_triangle = in_content && cx <= cy;
        bool is_marker = (in_content &&
                          cx >= content_size - marker_size &&
                          cx < content_size &&
                          cy >= 0 && cy < marker_size);

        if (is_border || is_triangle || is_marker)
            word |= (1u << b);
    }
    return word;
}

static void
draw_mono_box(ati_device_t *dev, int size, int border)
{
    int total_words = (size * size + 31) / 32;
    for (int i = 0; i < total_words - 1; i++)
        ati_reg_write(dev, HOST_DATA0 + (i % 8) * 4,
                      mono_box_word(size, border, i));
    ati_reg_write(dev, HOST_DATA_LAST,
                  mono_box_word(size, border, total_words - 1));
}

bool
test_host_data_32x32(ati_device_t *dev)
{
    uint32_t red = 0x00ff0000;
    uint32_t green = 0x0000ff00;
    unsigned width = 32;
    unsigned height = 32;

    ati_screen_clear(dev, 0);

    wr_dp_src_frgd_clr(dev, red);
    wr_dp_src_bkgd_clr(dev, green);

    wr_r128_default_offset(dev, 0x0);
    wr_r128_default_pitch(dev, 0x50);
    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_dp_write_msk(dev, 0xffffffff);

    wr_dp_gui_master_cntl(dev, GMC_BRUSH_NONE | GMC_DST_32BPP |
        GMC_BYTE_LSB_TO_MSB | GMC_ROP3_SRCCOPY | GMC_SRC_SOURCE_HOST_DATA);

    wr_dst_x_y(dev, 0x0);
    wr_dst_width_height(dev, (width << 16) | height);

    wr_host_data0(dev, 0x00000000);
    wr_host_data1(dev, 0x00000000);
    wr_host_data2(dev, 0x00000000);
    wr_host_data3(dev, 0x00000000);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_1"));
    ASSERT_EQ(rd_dst_x(dev), 0x0);
    ASSERT_EQ(rd_dst_y(dev), 0x0);

    wr_host_data4(dev, 0x0ffffff0);
    wr_host_data5(dev, 0x0ffffff0);
    wr_host_data6(dev, 0x0ffffff0);
    wr_host_data7(dev, 0x0ffffff0);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_2"));

    wr_host_data0(dev, 0x0ffffff0);
    wr_host_data1(dev, 0x0ffffff0);
    wr_host_data2(dev, 0x0ffffff0);
    wr_host_data3(dev, 0x0ffffff0);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_3"));

    wr_host_data4(dev, 0x0ffffff0);
    wr_host_data5(dev, 0x0ffffff0);
    wr_host_data6(dev, 0x0ffffff0);
    wr_host_data7(dev, 0x0ffffff0);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_4"));

    wr_host_data0(dev, 0x0ffffff0);
    wr_host_data1(dev, 0x0ffffff0);
    wr_host_data2(dev, 0x0ffffff0);
    wr_host_data3(dev, 0x0ffffff0);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_5"));

    wr_host_data4(dev, 0x0ffffff0);
    wr_host_data5(dev, 0x0ffffff0);
    wr_host_data6(dev, 0x0ffffff0);
    wr_host_data7(dev, 0x0ffffff0);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_6"));

    wr_host_data0(dev, 0x0ffffff0);
    wr_host_data1(dev, 0x0ffffff0);
    wr_host_data2(dev, 0x0ffffff0);
    wr_host_data3(dev, 0x0ffffff0);
    ASSERT_TRUE(ati_screen_async_compare_fixture(
        dev, "host_data_mono_32x32_partial_7"));

    wr_host_data4(dev, 0x00000000);
    wr_host_data5(dev, 0x00000000);
    wr_host_data6(dev, 0x00000000);
    wr_host_data_last(dev, 0x00000000);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, "host_data_mono_32x32"));

    /* surprisingly, dst_x and dst_y are NOT updated after the blit */
    ASSERT_EQ(rd_dst_x(dev), 0x0);
    ASSERT_EQ(rd_dst_y(dev), 0x0);

    return true;
}

// This proves that monochrome HOST_DATA is bit-packed and
// not aligned with either the 128-bit accumulator or the
// 32-bit writes to HOST_DATA.
bool
test_host_data_mono_is_bit_packed(ati_device_t *dev)
{
    uint32_t red = 0x00ff0000;
    uint32_t green = 0x0000ff00;
    unsigned width = 2;
    unsigned height = 2;

    ati_screen_clear(dev, 0);

    wr_dp_src_frgd_clr(dev, red);
    wr_dp_src_bkgd_clr(dev, green);

    wr_r128_default_offset(dev, 0x0);
    wr_r128_default_pitch(dev, 0x50);
    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_dp_write_msk(dev, 0xffffffff);

    wr_dp_gui_master_cntl(dev, GMC_BRUSH_NONE | GMC_DST_32BPP |
        GMC_BYTE_LSB_TO_MSB | GMC_ROP3_SRCCOPY | GMC_SRC_SOURCE_HOST_DATA);

    wr_dst_x_y(dev, 0x0);
    wr_dst_width_height(dev, (width << 16) | height);

    wr_host_data_last(dev, 0x00000033);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, "host_data_mono_bitpacked"));

    return true;
}

bool
test_host_data_morphos(ati_device_t *dev)
{
    ati_screen_clear(dev, 0);
    // Adjusted from MorphOS output for 640x480 screen
    wr_dst_offset(dev, 0x0);
    wr_dst_pitch(dev, 0x50);  // 640 pixels at 32bpp
    wr_dp_datatype(dev, 0x6);  // 32bpp
    wr_dp_mix(dev, 0xcc0300);  // SRCCOPY

    wr_sc_top_left(dev, 0x0);
    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_sc_top_left(dev, 0x0);
    wr_sc_bottom_right(dev, 0x1fff1fff);

    wr_dp_cntl(dev, 0x3);  // L->R, T->B
    wr_dp_src_frgd_clr(dev, 0xff000000);   // black
    wr_dp_src_bkgd_clr(dev, 0xffececec);   // light gray
    wr_dst_y_x(dev, (100 << 16) | 100);    // y=100, x=100
    wr_src_y_x(dev, 0x0);
    wr_dst_width_height(dev, 0x200008);    // 32x8 = 256 pixels
    // 8 writes to HOST_DATA0, no HOST_DATA_LAST
    // MorphOS style, all to same register
    wr_host_data0(dev, 0xce007c);
    wr_host_data0(dev, 0x38de00ce);
    wr_host_data0(dev, 0xf638de);
    wr_host_data0(dev, 0xe600f6);
    wr_host_data0(dev, 0xc600e6);
    wr_host_data0(dev, 0x187c00c6);
    wr_host_data0(dev, 0x7c00187c);
    wr_host_data0(dev, 0x7c00);
    ASSERT_TRUE(ati_screen_compare_fixture(dev, "host_data_morphos_all_data0"));

    return true;
}

bool
do_clipping(ati_device_t *dev, int margin, int size, int border,
            const char *dir)
{
    uint32_t dst_width_height = (size << 16) | size;
    char fixture[64];

    /* Completely clipped */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (100 << 16) | 100);
    wr_sc_bottom_right(dev, (200 << 16) | 200);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_completely_clipped", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* No clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (margin << 16) | margin);
    wr_sc_bottom_right(dev, (41 << 16) | 41);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_no_clip", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* Right clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (margin << 16) | margin);
    wr_sc_bottom_right(dev, (41 << 16) | 38);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_right", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* Left clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (10 << 16) | 13);
    wr_sc_bottom_right(dev, (41 << 16) | 41);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_left", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* Left and right clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (10 << 16) | 13);
    wr_sc_bottom_right(dev, (41 << 16) | 38);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_left_right", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* Top clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (13 << 16) | 10);
    wr_sc_bottom_right(dev, (41 << 16) | 41);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_top", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* Bottom clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (10 << 16) | 10);
    wr_sc_bottom_right(dev, (38 << 16) | 41);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_bottom", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* Top and bottom clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (13 << 16) | 10);
    wr_sc_bottom_right(dev, (38 << 16) | 41);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_top_bottom", dir);
    ati_screen_compare_fixture(dev, fixture);

    /* All sides clipping */
    ati_screen_clear(dev, 0);
    wr_sc_top_left(dev, (13 << 16) | 13);
    wr_sc_bottom_right(dev, (38 << 16) | 38);
    wr_dst_width_height(dev, dst_width_height);
    draw_mono_box(dev, size, border);
    snprintf(fixture, sizeof(fixture), "host_data_%s_clip_all", dir);
    ati_screen_compare_fixture(dev, fixture);

    return true;
}

bool
test_host_data_clipping(ati_device_t *dev)
{
    static const uint32_t BORDER = 0x00cc3355;
    static const uint32_t TRIANGLE_FILL = 0x0055cc33;

    int margin = 10;
    int size = 32;
    int border = 4;

    /* Common setup */
    wr_dst_offset(dev, 0x0);
    wr_dst_pitch(dev, 0x50);           /* 640 pixels / 8 = 80 = 0x50 */
    wr_dp_datatype(dev, 0x40000006);   /* 32bpp + LSB_TO_MSB byte order */
    wr_dp_mix(dev, 0xcc0300);          /* SRCCOPY + HOST_DATA source */
    wr_dp_src_frgd_clr(dev, BORDER);
    wr_dp_src_bkgd_clr(dev, TRIANGLE_FILL);

    wr_r128_default_offset(dev, 0x0);
    wr_r128_default_pitch(dev, 0x50);
    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_dp_write_msk(dev, 0xffffffff);

    // Top to bottom, left to right
    wr_dp_cntl(dev, 0x3);              /* L->R, T->B */
    wr_dst_y_x(dev, (margin << 16) | margin);
    do_clipping(dev, margin, size, border, "ltr_ttb");

    // Bottom to top, left to right
    wr_dp_cntl(dev, 0x1);              /* L->R, B->T */
    wr_dst_y_x(dev, ((margin + size - 1) << 16) | margin);
    do_clipping(dev, margin, size, border, "ltr_btt");

    /* Right to left seems to produce undefined behavior for HOST_DATA
     * Dumps are maintained in the fixtures directory for reference */
    // Top to bottom, right to left
    //wr_dp_cntl(dev, 0x2);              /* R->L, T->B */
    //wr_dst_y_x(dev, (margin << 16) | (margin + size - 2));
    //do_clipping(dev, margin, size, border, "rtl_ttb");

    /* Right to left seems to produce undefined behavior for HOST_DATA
     * Dumps are maintained in the fixtures directory for reference */
    // Bottom to top, right to left
    //wr_dp_cntl(dev, 0x0);              /* R->L, B->T */
    //wr_dst_y_x(dev, ((margin + size - 1) << 16) | (margin + size - 1));
    //do_clipping(dev, margin, size, border, "rtl_btt");

    return true;
}

void
register_host_data_tests(void)
{
    REGISTER_TEST(test_host_data_32x32, "host_data 32x32");
    REGISTER_TEST(test_host_data_mono_is_bit_packed,
                  "host_data mono is bit packed");
    REGISTER_TEST(test_host_data_morphos, "test host data morphos");
    REGISTER_TEST(test_host_data_clipping, "host_data clipping");
}
