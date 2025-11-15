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

bool test_host_data_32x32(ati_device_t *dev) {
  uint32_t red = 0x00ff0000;
  uint32_t green = 0x0000ff00;
  unsigned width = 32;
  unsigned height = 32;

  ati_screen_clear(dev);

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
  wr_dst_width_height(dev, (width << 16) | height);

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

// This proves that monochrome HOST_DATA is bit-packed and
// not aligned with either the 128-bit accumulator or the
// 32-bit writes to HOST_DATA.
bool test_host_data_mono_is_bit_packed(ati_device_t *dev) {
  uint32_t red = 0x00ff0000;
  uint32_t green = 0x0000ff00;
  unsigned width = 2;
  unsigned height = 2;

  //ati_vram_clear(dev);
  ati_screen_clear(dev);

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
  wr_dst_width_height(dev, (width << 16) | height);

  wr_host_data_last(dev, 0x00000033);
  ASSERT_TRUE(ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_bitpacked.bin"
  ));

  return true;
}

void register_host_data_tests(void) {
  register_test("host_data 32x32", test_host_data_32x32);
  register_test("host_data mono is bit packed", test_host_data_mono_is_bit_packed);
}
