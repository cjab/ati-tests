#include "../ati.h"
#include "../common.h"

void test_src_clipping(ati_device_t *dev) {
  uint32_t dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  uint32_t default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  uint32_t src_sc_bottom = ati_reg_read(dev, SRC_SC_BOTTOM);
  uint32_t src_sc_right = ati_reg_read(dev, SRC_SC_RIGHT);

  printf("Test SRC clipping\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_SC_BOTTOM_RIGHT to 0x0 **\n");
  printf("** Initializing SRC_SC_BOTTOM to 0x0 **\n");
  printf("** Initializing SRC_SC_RIGHT to 0x0 **\n");

  ati_reg_write(dev, DEFAULT_SC_BOTTOM_RIGHT, 0x0);
  ati_reg_write(dev, SRC_SC_BOTTOM, 0x0);
  ati_reg_write(dev, SRC_SC_RIGHT, 0x0);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = ati_reg_read(dev, SRC_SC_BOTTOM);
  src_sc_right = ati_reg_read(dev, SRC_SC_RIGHT);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");

  printf("** Setting DEFAULT_SC_BOTTOM_RIGHT to 0x0aaa0bbb **\n");
  printf("** Setting SRC_SC_BOTTOM to 0x111 **\n");
  printf("** Setting SRC_SC_RIGHT to 0x222 **\n");
  ati_reg_write(dev, DEFAULT_SC_BOTTOM_RIGHT, 0x0aaa0bbb);
  ati_reg_write(dev, SRC_SC_BOTTOM, 0x00000111);
  ati_reg_write(dev, SRC_SC_RIGHT, 0x00000222);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = ati_reg_read(dev, SRC_SC_BOTTOM);
  src_sc_right = ati_reg_read(dev, SRC_SC_RIGHT);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");

  printf("** Setting GMC_SRC_CLIPPING to default **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x4);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = ati_reg_read(dev, SRC_SC_BOTTOM);
  src_sc_right = ati_reg_read(dev, SRC_SC_RIGHT);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");

  printf("** Setting GMC_SRC_CLIPPING to leave alone **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x4);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = ati_reg_read(dev, SRC_SC_BOTTOM);
  src_sc_right = ati_reg_read(dev, SRC_SC_RIGHT);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");
}

void test_dst_clipping(ati_device_t *dev) {
  uint32_t dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  uint32_t default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  uint32_t sc_bottom = ati_reg_read(dev, SC_BOTTOM);
  uint32_t sc_right = ati_reg_read(dev, SC_RIGHT);
  uint32_t sc_top = ati_reg_read(dev, SC_TOP);
  uint32_t sc_left = ati_reg_read(dev, SC_LEFT);

  printf("Test DST clipping\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_SC_BOTTOM_RIGHT to 0x0 **\n");
  printf("** Initializing SC_BOTTOM to 0x0 **\n");
  printf("** Initializing SC_RIGHT to 0x0 **\n");
  printf("** Initializing SC_TOP to 0x0 **\n");
  printf("** Initializing SC_LEFT to 0x0 **\n");

  ati_reg_write(dev, DEFAULT_SC_BOTTOM_RIGHT, 0x0);
  ati_reg_write(dev, SC_BOTTOM, 0x0);
  ati_reg_write(dev, SC_RIGHT, 0x0);
  ati_reg_write(dev, SC_TOP, 0x0);
  ati_reg_write(dev, SC_LEFT, 0x0);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = ati_reg_read(dev, SC_BOTTOM);
  sc_right = ati_reg_read(dev, SC_RIGHT);
  sc_top = ati_reg_read(dev, SC_TOP);
  sc_left = ati_reg_read(dev, SC_LEFT);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SC_BOTTOM:               0x%08x\n", sc_bottom);
  printf("SC_RIGHT:                0x%08x\n", sc_right);
  printf("SC_TOP:                  0x%08x\n", sc_top);
  printf("SC_LEFT:                 0x%08x\n", sc_left);
  printf("\n");

  printf("** Setting DEFAULT_SC_BOTTOM_RIGHT to 0x0aaa0bbb **\n");
  printf("** Setting SC_BOTTOM to 0x111 **\n");
  printf("** Setting SC_RIGHT to 0x222 **\n");
  printf("** SETTING SC_TOP to 0x333 **\n");
  printf("** SETTING SC_LEFT to 0x444 **\n");
  ati_reg_write(dev, DEFAULT_SC_BOTTOM_RIGHT, 0x0aaa0bbb);
  ati_reg_write(dev, SC_BOTTOM, 0x00000111);
  ati_reg_write(dev, SC_RIGHT, 0x00000222);
  ati_reg_write(dev, SC_TOP, 0x00000333);
  ati_reg_write(dev, SC_LEFT, 0x00000444);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = ati_reg_read(dev, SC_BOTTOM);
  sc_right = ati_reg_read(dev, SC_RIGHT);
  sc_top = ati_reg_read(dev, SC_TOP);
  sc_left = ati_reg_read(dev, SC_LEFT);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SC_BOTTOM:               0x%08x\n", sc_bottom);
  printf("SC_RIGHT:                0x%08x\n", sc_right);
  printf("SC_TOP:                  0x%08x\n", sc_top);
  printf("SC_LEFT:                 0x%08x\n", sc_left);
  printf("\n");

  printf("** Setting GMC_DST_CLIPPING to default **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x8);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = ati_reg_read(dev, SC_BOTTOM);
  sc_right = ati_reg_read(dev, SC_RIGHT);
  sc_top = ati_reg_read(dev, SC_TOP);
  sc_left = ati_reg_read(dev, SC_LEFT);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SC_BOTTOM:               0x%08x\n", sc_bottom);
  printf("SC_RIGHT:                0x%08x\n", sc_right);
  printf("SC_TOP:                  0x%08x\n", sc_top);
  printf("SC_LEFT:                 0x%08x\n", sc_left);
  printf("\n");

  printf("** Setting GMC_DST_CLIPPING to leave alone **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x8);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = ati_reg_read(dev, SC_BOTTOM);
  sc_right = ati_reg_read(dev, SC_RIGHT);
  sc_top = ati_reg_read(dev, SC_TOP);
  sc_left = ati_reg_read(dev, SC_LEFT);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SC_BOTTOM:               0x%08x\n", sc_bottom);
  printf("SC_RIGHT:                0x%08x\n", sc_right);
  printf("SC_TOP:                  0x%08x\n", sc_top);
  printf("SC_LEFT:                 0x%08x\n", sc_left);
  printf("\n");
}

void register_clipping_tests(void) {
  register_test("SRC clipping", test_src_clipping);
  register_test("DST clipping", test_dst_clipping);
}
