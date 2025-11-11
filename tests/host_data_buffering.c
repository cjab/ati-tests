#include "../ati.h"
#include "../common.h"
#include <unistd.h>

#define GMC_SRC_DATATYPE_COLOR          0x00030000
#define GMC_SRC_SOURCE_HOST_DATA        0x03000000
#define GMC_DST_32BPP                   0x00000600
#define GMC_ROP3_SRCCOPY                0x00cc0000
#define GMC_BRUSH_NONE                  0x000000f0
#define GMC_BRUSH_SOLIDCOLOR            0x000000d0
#define GMC_DST_PITCH_OFFSET_LEAVE      0x00000002
#define GMC_BYTE_LSB_TO_MSB             0x00004000

void print_state(ati_device_t *dev) {
  uint32_t dp_gui_master_cntl = rd_dp_gui_master_cntl(dev);
  uint32_t default_offset = rd_default_offset(dev);
  uint32_t default_pitch = rd_default_pitch(dev);
  uint32_t default_sc_br = rd_default_sc_bottom_right(dev);
  uint32_t sc_top = rd_sc_top(dev);
  uint32_t sc_left = rd_sc_left(dev);
  uint32_t sc_bottom = rd_sc_bottom(dev);
  uint32_t sc_right = rd_sc_right(dev);
  uint32_t dp_datatype = rd_dp_datatype(dev);
  uint32_t dp_write_mask = rd_dp_write_msk(dev);
  uint32_t src_offset = rd_src_offset(dev);
  uint32_t dst_offset = rd_dst_offset(dev);
  uint32_t dst_pitch = rd_dst_pitch(dev);
  uint32_t dst_width = rd_dst_width(dev);
  uint32_t dst_height = rd_dst_height(dev);
  uint32_t dst_x = rd_dst_x(dev);
  uint32_t dst_y = rd_dst_y(dev);
  uint32_t dp_src_frgd_clr = rd_dp_src_frgd_clr(dev);
  uint32_t dp_src_bkgd_clr = rd_dp_src_bkgd_clr(dev);

  printf("\n");
  printf("┌──────  Current Hardware State  ──────┐\n");
  printf("│ DP_GUI_MASTER_CNTL:       0x%08x │\n", dp_gui_master_cntl);
  printf("│ DEFAULT_OFFSET:           0x%08x │\n", default_offset);
  printf("│ DEFAULT_PITCH:            0x%08x │\n", default_pitch);
  printf("│ DEFAULT_SC_BOTTOM_RIGHT:  0x%08x │\n", default_sc_br);
  printf("│ SC_TOP:                   0x%08x │\n", sc_top);
  printf("│ SC_LEFT:                  0x%08x │\n", sc_left);
  printf("│ SC_BOTTOM:                0x%08x │\n", sc_bottom);
  printf("│ SC_RIGHT:                 0x%08x │\n", sc_right);
  printf("│ DP_DATATYPE:              0x%08x │\n", dp_datatype);
  printf("│ DP_WRITE_MASK:            0x%08x │\n", dp_write_mask);
  printf("│ SRC_OFFSET:               0x%08x │\n", src_offset);
  printf("│ DST_OFFSET:               0x%08x │\n", dst_offset);
  printf("│ DST_PITCH:                0x%08x │\n", dst_pitch);
  printf("│ DST_WIDTH:                0x%08x │\n", dst_width);
  printf("│ DST_HEIGHT:               0x%08x │\n", dst_height);
  printf("│ DST_X:                    0x%08x │\n", dst_x);
  printf("│ DST_Y:                    0x%08x │\n", dst_y);
  printf("│ DP_SRC_FRGD_CLR:          0x%08x │\n", dp_src_frgd_clr);
  printf("│ DP_SRC_BKGD_CLR:          0x%08x │\n", dp_src_bkgd_clr);
  printf("└──────────────────────────────────────┘\n\n");
}

bool test_host_data_32x32(ati_device_t *dev) {
  bool pass = true;
  uint32_t red = 0x00ff0000;
  uint32_t green = 0x0000ff00;

  ati_vram_clear(dev);

  print_state(dev);

  wr_dp_src_frgd_clr(dev, red);
  wr_dp_src_bkgd_clr(dev, green);

  wr_default_pitch(dev, 0x50);
  wr_default_offset(dev, 0x0);
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
    GMC_BRUSH_NONE |           // GMC_BRUSH_DATATYPE
    GMC_DST_32BPP |            // GMC_DST_DATATYPE
    // GMC_SRC_DATATYPE 0x0 (MONO)
    GMC_BYTE_LSB_TO_MSB |      // GMC_BYTE_PIX_ORDER
    GMC_ROP3_SRCCOPY |         // GMC_ROP3
    GMC_SRC_SOURCE_HOST_DATA); // GMC_SRC_SOURCE

  wr_dst_x_y(dev, 0x0);
  wr_dst_width_height(dev, (32 << 16) | 32);

  wr_host_data0(dev, 0x00000000);
  wr_host_data1(dev, 0x00000000);
  wr_host_data2(dev, 0x00000000);
  wr_host_data3(dev, 0x00000000);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_1.bin"
  ));

  wr_host_data4(dev, 0x0ffffff0);
  wr_host_data5(dev, 0x0ffffff0);
  wr_host_data6(dev, 0x0ffffff0);
  wr_host_data7(dev, 0x0ffffff0);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_2.bin"
  ));

  wr_host_data0(dev, 0x0ffffff0);
  wr_host_data1(dev, 0x0ffffff0);
  wr_host_data2(dev, 0x0ffffff0);
  wr_host_data3(dev, 0x0ffffff0);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_3.bin"
  ));

  wr_host_data4(dev, 0x0ffffff0);
  wr_host_data5(dev, 0x0ffffff0);
  wr_host_data6(dev, 0x0ffffff0);
  wr_host_data7(dev, 0x0ffffff0);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_4.bin"
  ));

  wr_host_data0(dev, 0x0ffffff0);
  wr_host_data1(dev, 0x0ffffff0);
  wr_host_data2(dev, 0x0ffffff0);
  wr_host_data3(dev, 0x0ffffff0);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_5.bin"
  ));

  wr_host_data4(dev, 0x0ffffff0);
  wr_host_data5(dev, 0x0ffffff0);
  wr_host_data6(dev, 0x0ffffff0);
  wr_host_data7(dev, 0x0ffffff0);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_6.bin"
  ));

  wr_host_data0(dev, 0x0ffffff0);
  wr_host_data1(dev, 0x0ffffff0);
  wr_host_data2(dev, 0x0ffffff0);
  wr_host_data3(dev, 0x0ffffff0);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32_partial_7.bin"
  ));

  wr_host_data4(dev, 0x00000000);
  wr_host_data5(dev, 0x00000000);
  wr_host_data6(dev, 0x00000000);

  wr_host_data_last(dev, 0x00000000);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32.bin"
  ));

  return true;
}

void register_host_data_tests(void) {
  register_test("host_data 32x32", test_host_data_32x32);
}
