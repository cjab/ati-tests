#include "../ati.h"
#include "../common.h"

void test_src_clipping(ati_device_t *dev) {
  printf("Test SRC clipping\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_SC_BOTTOM_RIGHT to 0x0 **\n");
  wr_default_sc_bottom_right(dev, 0x0);
  printf("** Initializing SRC_SC_BOTTOM to 0x0 **\n");
  wr_src_sc_bottom(dev, 0x0);
  printf("** Initializing SRC_SC_RIGHT to 0x0 **\n");
  wr_src_sc_right(dev, 0x0);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SRC_SC_BOTTOM:           0x%08x\n", rd_src_sc_bottom(dev));
  printf("SRC_SC_RIGHT:            0x%08x\n", rd_src_sc_right(dev));
  printf("\n");

  printf("** Setting DEFAULT_SC_BOTTOM_RIGHT to 0x0aaa0bbb **\n");
  printf("** Setting SRC_SC_BOTTOM to 0x111 **\n");
  printf("** Setting SRC_SC_RIGHT to 0x222 **\n");
  wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
  wr_src_sc_bottom(dev, 0x00000111);
  wr_src_sc_right(dev, 0x00000222);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SRC_SC_BOTTOM:           0x%08x\n", rd_src_sc_bottom(dev));
  printf("SRC_SC_RIGHT:            0x%08x\n", rd_src_sc_right(dev));
  printf("\n");

  printf("** Setting GMC_SRC_CLIPPING to default **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) & ~0x4);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SRC_SC_BOTTOM:           0x%08x\n", rd_src_sc_bottom(dev));
  printf("SRC_SC_RIGHT:            0x%08x\n", rd_src_sc_right(dev));
  printf("\n");

  printf("** Setting GMC_SRC_CLIPPING to leave alone **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) | 0x4);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SRC_SC_BOTTOM:           0x%08x\n", rd_src_sc_bottom(dev));
  printf("SRC_SC_RIGHT:            0x%08x\n", rd_src_sc_right(dev));
  printf("\n");
}

void test_dst_clipping(ati_device_t *dev) {
  printf("Test DST clipping\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_SC_BOTTOM_RIGHT to 0x0 **\n");
  wr_default_sc_bottom_right(dev, 0x0);
  printf("** Initializing SC_BOTTOM to 0x0 **\n");
  wr_sc_bottom(dev, 0x0);
  printf("** Initializing SC_RIGHT to 0x0 **\n");
  wr_sc_right(dev, 0x0);
  printf("** Initializing SC_TOP to 0x0 **\n");
  wr_sc_top(dev, 0x0);
  printf("** Initializing SC_LEFT to 0x0 **\n");
  wr_sc_left(dev, 0x0);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");

  printf("** Setting DEFAULT_SC_BOTTOM_RIGHT to 0x0aaa0bbb **\n");
  wr_default_sc_bottom_right(dev, 0x0aaa0bbb);
  printf("** Setting SC_BOTTOM to 0x111 **\n");
  wr_sc_bottom(dev, 0x00000111);
  printf("** Setting SC_RIGHT to 0x222 **\n");
  wr_sc_right(dev, 0x00000222);
  printf("** SETTING SC_TOP to 0x333 **\n");
  wr_sc_top(dev, 0x00000333);
  printf("** SETTING SC_LEFT to 0x444 **\n");
  wr_sc_left(dev, 0x00000444);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");

  printf("** Setting GMC_DST_CLIPPING to default **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) & ~0x8);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");

  printf("** Setting GMC_DST_CLIPPING to leave alone **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) | 0x8);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", rd_default_sc_bottom_right(dev));
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");
}

void test_reserved_scissor_bits(ati_device_t *dev) {
  printf("** Initializing SC_BOTTOM to 0x0 **\n");
  wr_sc_bottom(dev, 0x0);
  printf("** Initializing SC_RIGHT to 0x0 **\n");
  wr_sc_right(dev, 0x0);
  printf("** Initializing SC_TOP to 0x0 **\n");
  wr_sc_top(dev, 0x0);
  printf("** Initializing SC_LEFT to 0x0 **\n");
  wr_sc_left(dev, 0x0);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");

  printf("** Setting SC_BOTTOM to 0xffffffff **\n");
  wr_sc_bottom(dev, 0xffffffff);
  printf("** Setting SC_TOP to 0xffffffff **\n");
  wr_sc_top(dev, 0xffffffff);

  printf("\n");
  printf("After State\n");
  printf("------------------------------------\n");
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");

  printf("** Setting SC_RIGHT to 0xffffffff **\n");
  wr_sc_right(dev, 0xffffffff);
  printf("** Setting SC_LEFT to 0xffffffff **\n");
  wr_sc_left(dev, 0xffffffff);

  printf("\n");
  printf("After State\n");
  printf("------------------------------------\n");
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");

  printf("** Setting SC_BOTTOM_RIGHT to 0xfeeefeee **\n");
  wr_sc_bottom_right(dev, 0xfeeefeee);
  printf("** Setting SC_TOP_LEFT to 0xfeeefeee **\n");
  wr_sc_top_left(dev, 0xfeeefeee);

  printf("\n");
  printf("After State\n");
  printf("------------------------------------\n");
  printf("SC_BOTTOM:               0x%08x\n", rd_sc_bottom(dev));
  printf("SC_RIGHT:                0x%08x\n", rd_sc_right(dev));
  printf("SC_TOP:                  0x%08x\n", rd_sc_top(dev));
  printf("SC_LEFT:                 0x%08x\n", rd_sc_left(dev));
  printf("\n");
}

void register_clipping_tests(void) {
  register_test("SRC clipping", test_src_clipping);
  register_test("DST clipping", test_dst_clipping);
  register_test("reserved scissor bits", test_reserved_scissor_bits);
}
