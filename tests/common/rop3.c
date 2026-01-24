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

#define R 0x00ff0000
#define G 0x0000ff00

// clang-format off
static const uint32_t data[] = {
  R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
  R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
  R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
  R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
};
// clang-format on

bool
test_rop3_16x16(ati_device_t *dev)
{
    ati_screen_clear(dev, 0);

    unsigned rows = 16;
    unsigned cols = 16;
    uint32_t pitch = 0x50;
    for (unsigned i = 0; i < rows; i++) {
        unsigned offset = pitch * 8 * i * sizeof(uint32_t);
        ati_vram_memcpy(dev, offset, &data[i * cols], cols * sizeof(uint32_t));
    }

    wr_src_x_y(dev, 0x0);

    switch (ati_get_chip_family(dev)) {
    case CHIP_R128:
        wr_r128_default_offset(dev, 0x0);
        wr_r128_default_pitch(dev, pitch);
        break;
    case CHIP_R100:
        wr_r100_default_pitch_offset(dev, pitch << 22);
        break;
    default:

    }
    wr_default_sc_bottom_right(dev, 0x1fff1fff);
    wr_dp_write_msk(dev, 0xffffffff);

    wr_dp_gui_master_cntl(dev,
                          // SRC_OFFSET = DEFAULT_OFFSET
                          // SRC_PITCH = DEFAULT_PITCH
                          // DST_OFFSET = DEFAULT_OFFSET
                          // DST_PITCH = DEFAULT_PITCH
                          // SRC_CLIPPING = DEFAULT_SC_BOTTOM_RIGHT
                          // DST_CLIPPING = DEFAULT_SC_BOTTOM_RIGHT
                          // SC_TOP, SC_LEFT = (0, 0)
                          GMC_BRUSH_NONE |    // GMC_BRUSH_DATATYPE
                              GMC_DST_32BPP | // GMC_DST_DATATYPE
                              GMC_SRC_DATATYPE_COLOR |
                              // GMC_SRC_DATATYPE 0x0 (MONO)
                              GMC_BYTE_LSB_TO_MSB |   // GMC_BYTE_PIX_ORDER
                              GMC_ROP3_SRCCOPY |      // GMC_ROP3
                              GMC_SRC_SOURCE_MEMORY); // GMC_SRC_SOURCE

    wr_dst_x(dev, 0x10);
    wr_dst_y(dev, 0x10);
    wr_dst_width_height(dev, (16 << 16) | 16);

    ASSERT_TRUE(ati_screen_compare_fixture(dev, "rop3_color_16x16"));
    /* The dst_x and dst_y registers are not updated */
    ASSERT_EQ(rd_dst_x(dev), 0x10);
    ASSERT_EQ(rd_dst_y(dev), 0x10);

    return true;
}

void
register_rop3_tests(void)
{
    REGISTER_TEST(test_rop3_16x16, "rop3 16x16");
}
