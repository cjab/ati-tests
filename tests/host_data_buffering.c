#include "../ati.h"
#include "../common.h"
#include <unistd.h>

/* Register field definitions */
#define GMC_SRC_DATATYPE_COLOR          0x00030000
#define GMC_SRC_SOURCE_HOST_DATA        0x03000000
#define GMC_DST_32BPP                   0x00000600
#define GMC_ROP3_SRCCOPY                0x00cc0000
#define GMC_BRUSH_NONE                  0x000000f0
#define GMC_BRUSH_SOLIDCOLOR            0x000000d0
#define GMC_DST_PITCH_OFFSET_LEAVE      0x00000002
#define GMC_BYTE_LSB_TO_MSB             0x00004000

/* Helper to wait for GUI engine idle */
static void wait_gui_idle(ati_device_t *dev) {
  uint32_t stat;
  int timeout = 10000;
  while (timeout-- > 0) {
    stat = rd_gui_stat(dev);
    if ((stat & 0xFFF) == 0x40) { /* FIFO empty and GUI idle */
      break;
    }
    usleep(1);
  }
}

void print_state(ati_device_t *dev) {
  // Read current register values
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
  uint32_t red = 0x00ff0000;
  uint32_t green = 0x0000ff00;

  //printf("Clear VRAM\n");
  //ati_vram_clear(dev);
  ati_screen_clear(dev);

  //print_state(dev);

  wr_dp_src_frgd_clr(dev, red);
  wr_dp_src_bkgd_clr(dev, green);

  wr_default_pitch(dev, 0x50);
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

  /* Draw at top left of screen */
  //wr_dst_x_y(dev, 0x0);
  wr_dst_x(dev, 0x0);
  wr_dst_y(dev, 0x0);
  /* 8 pixels wide, 8 pixel tall */
  wr_dst_width_height(dev, (32 << 16) | 32);

  // Write 8 DWORDs of HOST_DATA
  wr_host_data0(dev, 0x00000000);
  wr_host_data1(dev, 0x00000000);
  wr_host_data2(dev, 0x00000000);
  wr_host_data3(dev, 0x00000000);
  wr_host_data4(dev, 0x0ffffff0);
  wr_host_data5(dev, 0x0ffffff0);
  wr_host_data6(dev, 0x0ffffff0);
  wr_host_data7(dev, 0x0ffffff0);

  wr_host_data0(dev, 0x0ffffff0);
  wr_host_data1(dev, 0x0ffffff0);
  wr_host_data2(dev, 0x0ffffff0);
  wr_host_data3(dev, 0x0ffffff0);
  wr_host_data4(dev, 0x0ffffff0);
  wr_host_data5(dev, 0x0ffffff0);
  wr_host_data6(dev, 0x0ffffff0);
  wr_host_data7(dev, 0x0ffffff0);

  wr_host_data0(dev, 0x0ffffff0);
  wr_host_data1(dev, 0x0ffffff0);
  wr_host_data2(dev, 0x0ffffff0);
  wr_host_data3(dev, 0x0ffffff0);
  wr_host_data4(dev, 0x0ffffff0);
  wr_host_data5(dev, 0x0ffffff0);
  wr_host_data6(dev, 0x0ffffff0);
  wr_host_data7(dev, 0x0ffffff0);

  wr_host_data0(dev, 0x0ffffff0);
  wr_host_data1(dev, 0x0ffffff0);
  wr_host_data2(dev, 0x0ffffff0);
  wr_host_data3(dev, 0x0ffffff0);
  wr_host_data4(dev, 0x00000000);
  wr_host_data5(dev, 0x00000000);
  wr_host_data6(dev, 0x00000000);

  wr_host_data_last(dev, 0x00000000);

  //wr_dst_offset(dev, 0x100000); /* Byte offset - NOT shifted */
  //wr_dst_pitch(dev, 640 / 8);
  //wr_dp_cntl(dev, 0x00000001 | 0x00000002); /* DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM */
  //wr_dst_y_x(dev, 0); /* (0, 0) */
  //wr_dp_write_msk(dev, 0xFFFFFFFF);

  //wr_sc_top_left(dev, 0);  // (0, 0)
  //wr_sc_bottom_right(dev, (1023 << 16) | 1023);

  //printf("** Writing DST_WIDTH_HEIGHT (operation starts) **\n");
  //wr_dst_width_height(dev, (1 << 16) | 8); /* 1 row, 8 pixels */

  // Write 8 DWORDs of HOST_DATA
  //wr_host_data0(dev, 0x11111111);
  //wr_host_data1(dev, 0x22222222);
  //wr_host_data2(dev, 0x33333333);
  //wr_host_data3(dev, 0x44444444);
  //wr_host_data4(dev, 0x55555555);
  //wr_host_data5(dev, 0x66666666);
  //wr_host_data6(dev, 0x77777777);
  //wr_host_data_last(dev, 0x88888888);

  //print_state(dev);

  //printf("** Waiting a moment... **\n");
  //usleep(10000); /* 10ms */

  //printf("\nDumping VRAM Contents:\n");
  //printf("----------------------------------------\n");
  //ati_vram_dump(dev, "vram.dmp");
  //printf("\nDumping Screen Contents:\n");
  //printf("----------------------------------------\n");
  //ati_screen_dump(dev, "vram.dmp");

  return ati_screen_compare_file(
    dev,
    "fixtures/640x480_bgra/host_data_mono_32x32.bin"
  );
}

//void test_partial_write_new_operation(ati_device_t *dev) {
//  printf("Test: Partial write then new operation\n");
//  printf("=======================================\n\n");
//
//  uint32_t *vram = ati_vram_map(dev);
//  uint32_t test_offset = 0x110000 / 4; /* Different location */
//
//  printf("** Clearing test area **\n");
//  for (int i = 0; i < 32; i++) {
//    vram[test_offset + i] = 0xFFFFFFFF;
//  }
//
//  printf("** Setting up first blit for 16 pixels **\n");
//  wr_dp_gui_master_cntl(dev,
//    GMC_DST_PITCH_OFFSET_LEAVE |
//    GMC_SRC_DATATYPE_COLOR |
//    GMC_SRC_SOURCE_HOST_DATA |
//    GMC_DST_32BPP |
//    GMC_ROP3_SRCCOPY |
//    GMC_BRUSH_NONE);
//
//  wr_dst_offset(dev, 0x110000); /* Byte offset - NOT shifted */
//  wr_dst_pitch(dev, 128);
//  wr_dp_cntl(dev, 0x00000001 | 0x00000002); /* DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM */
//  wr_dst_y_x(dev, 0);
//  wr_dp_write_msk(dev, 0xFFFFFFFF);
//  wr_dst_width_height(dev, (1 << 16) | 16); /* Expecting 16 pixels */
//
//  printf("** Writing only 4 pixels (incomplete) **\n");
//  wr_host_data0(dev, 0xDEADBEEF);
//  wr_host_data1(dev, 0xCAFEBABE);
//  wr_host_data2(dev, 0xFEEDFACE);
//  wr_host_data3(dev, 0xDEADC0DE);
//
//  printf("** Starting NEW operation at different location **\n");
//  wr_dst_y_x(dev, (0 << 16) | 20); /* X=20, Y=0 */
//  wr_dst_width_height(dev, (1 << 16) | 4); /* New operation: 4 pixels */
//
//  wait_gui_idle(dev);
//
//  printf("\nChecking first operation area (pixels 0-15):\n");
//  printf("---------------------------------------------\n");
//  int first_op_data = 0;
//  for (int i = 0; i < 16; i++) {
//    uint32_t val = vram[test_offset + i];
//    if (val != 0xFFFFFFFF) {
//      printf("VRAM[%d] = 0x%08X <- Partial data committed!\n", i, val);
//      first_op_data = 1;
//    }
//  }
//  if (!first_op_data) {
//    printf("All pixels still 0xFFFFFFFF (partial data discarded)\n");
//  }
//
//  printf("\nChecking second operation area (pixels 20-23):\n");
//  printf("-----------------------------------------------\n");
//  for (int i = 20; i < 24; i++) {
//    printf("VRAM[%d] = 0x%08X\n", i, vram[test_offset + i]);
//  }
//
//  printf("\n");
//  ati_vram_unmap(dev, vram);
//}
//
//void test_incremental_visibility(ati_device_t *dev) {
//  printf("Test: Incremental data visibility\n");
//  printf("==================================\n\n");
//
//  uint32_t *vram = ati_vram_map(dev);
//  uint32_t test_offset = 0x120000 / 4;
//
//  printf("** Clearing test area **\n");
//  for (int i = 0; i < 16; i++) {
//    vram[test_offset + i] = 0xFFFFFFFF;
//  }
//
//  printf("** Setting up blit operation **\n");
//  wr_dp_gui_master_cntl(dev,
//    GMC_DST_PITCH_OFFSET_LEAVE |
//    GMC_SRC_DATATYPE_COLOR |
//    GMC_SRC_SOURCE_HOST_DATA |
//    GMC_DST_32BPP |
//    GMC_ROP3_SRCCOPY |
//    GMC_BRUSH_NONE);
//
//  wr_dst_offset(dev, 0x120000); /* Byte offset - NOT shifted */
//  wr_dst_pitch(dev, 128);
//  wr_dp_cntl(dev, 0x00000001 | 0x00000002); /* DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM */
//  wr_dst_y_x(dev, 0);
//  wr_dp_write_msk(dev, 0xFFFFFFFF);
//  wr_dst_width_height(dev, (1 << 16) | 8);
//
//  printf("\n** Writing HOST_DATA incrementally and checking VRAM **\n\n");
//
//  wr_host_data0(dev, 0x00000000);
//  usleep(100);
//  printf("After HOST_DATA0: VRAM[0]=0x%08X\n", vram[test_offset + 0]);
//
//  wr_host_data1(dev, 0x11111111);
//  usleep(100);
//  printf("After HOST_DATA1: VRAM[0]=0x%08X VRAM[1]=0x%08X\n",
//         vram[test_offset + 0], vram[test_offset + 1]);
//
//  wr_host_data2(dev, 0x22222222);
//  usleep(100);
//  printf("After HOST_DATA2: VRAM[0]=0x%08X VRAM[2]=0x%08X\n",
//         vram[test_offset + 0], vram[test_offset + 2]);
//
//  wr_host_data3(dev, 0x33333333);
//  wr_host_data4(dev, 0x44444444);
//  wr_host_data5(dev, 0x55555555);
//  wr_host_data6(dev, 0x66666666);
//  wr_host_data7(dev, 0x77777777);
//
//  printf("\n** Before HOST_DATA_LAST **\n");
//  for (int i = 0; i < 8; i++) {
//    printf("VRAM[%d] = 0x%08X\n", i, vram[test_offset + i]);
//  }
//
//  wr_host_data_last(dev, 0x88888888);
//  wait_gui_idle(dev);
//
//  printf("\n** After HOST_DATA_LAST **\n");
//  for (int i = 0; i < 9; i++) {
//    printf("VRAM[%d] = 0x%08X\n", i, vram[test_offset + i]);
//  }
//
//  printf("\n");
//  ati_vram_unmap(dev, vram);
//}
//
//void test_host_data_simple_visibility(ati_device_t *dev) {
//  printf("Test: Simple HOST_DATA visibility check\n");
//  printf("========================================\n\n");
//
//  uint32_t *vram = ati_vram_map(dev);
//  uint32_t test_offset = 0x130000 / 4;
//
//  printf("** Clearing test area **\n");
//  for (int i = 0; i < 4; i++) {
//    vram[test_offset + i] = 0xFFFFFFFF;
//  }
//
//  printf("** Setting up minimal blit (2 pixels) **\n");
//  wr_dp_gui_master_cntl(dev,
//    GMC_DST_PITCH_OFFSET_LEAVE |
//    GMC_SRC_DATATYPE_COLOR |
//    GMC_SRC_SOURCE_HOST_DATA |
//    GMC_DST_32BPP |
//    GMC_ROP3_SRCCOPY |
//    GMC_BRUSH_NONE);
//
//  wr_dst_offset(dev, 0x130000); /* Byte offset - NOT shifted */
//  wr_dst_pitch(dev, 128);
//  wr_dp_cntl(dev, 0x00000001 | 0x00000002); /* DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM */
//  wr_dst_y_x(dev, 0);
//  wr_dp_write_msk(dev, 0xFFFFFFFF);
//
//  printf("** Writing DST_WIDTH_HEIGHT (2 pixels) **\n");
//  wr_dst_width_height(dev, (1 << 16) | 2);
//
//  printf("** Writing first pixel **\n");
//  wr_host_data0(dev, 0xABCDEF01);
//
//  usleep(1000);
//  printf("VRAM after first write: [0]=0x%08X [1]=0x%08X\n",
//         vram[test_offset], vram[test_offset + 1]);
//
//  printf("** Writing second pixel with LAST **\n");
//  wr_host_data_last(dev, 0x12345678);
//
//  wait_gui_idle(dev);
//  printf("VRAM after LAST: [0]=0x%08X [1]=0x%08X\n",
//         vram[test_offset], vram[test_offset + 1]);
//
//  printf("\n");
//  ati_vram_unmap(dev, vram);
//}

void register_host_data_tests(void) {
  register_test("host_data 32x32", test_host_data_32x32);
  //register_test("HOST_DATA simple visibility", test_host_data_simple_visibility);
  //register_test("HOST_DATA partial write", test_partial_write_new_operation);
  //register_test("HOST_DATA incremental", test_incremental_visibility);
}
