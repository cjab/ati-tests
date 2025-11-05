#include "../common.h"
#include "../ati.h"

void test_src_pitch_offset_cntl(ati_device_t *dev) {
  printf("Test SRC pitch and offset\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_OFFSET to 0x0 **\n");
  wr_default_offset(dev, 0x0);
  printf("** Initializing DEFAULT_PITCH to 0x0 **\n");
  wr_default_pitch(dev, 0x0);
  printf("** Initializing SRC_OFFSET to 0x0 **\n");
  wr_src_offset(dev, 0x0);
  printf("** Initializing SRC_PITCH to 0x0 **\n");
  wr_src_pitch(dev, 0x0);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("SRC_OFFSET:              0x%08x\n", rd_src_offset(dev));
  printf("SRC_PITCH:               0x%08x\n", rd_src_pitch(dev));
  printf("\n");

  printf("** Setting DEFAULT_OFFSET to 0x000000aa **\n");
  wr_default_offset(dev, 0x000000aa);
  printf("** Setting DEFAULT_PITCH to 0x000000bb **\n");
  wr_default_pitch(dev, 0x000000bb);
  printf("** Setting SRC_OFFSET to 0x11 **\n");
  wr_src_offset(dev, 0x00000011);
  printf("** Setting SRC_PITCH to 0x22 **\n");
  wr_src_pitch(dev, 0x00000022);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("SRC_OFFSET:              0x%08x\n", rd_src_offset(dev));
  printf("SRC_PITCH:               0x%08x\n", rd_src_pitch(dev));
  printf("\n");

  printf("** Setting GMC_SRC_PITCH_OFFSET_CNTL to default **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) & ~0x1);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("SRC_OFFSET:              0x%08x\n", rd_src_offset(dev));
  printf("SRC_PITCH:               0x%08x\n", rd_src_pitch(dev));
  printf("\n");

  printf("** Setting GMC_SRC_PITCH_OFFSET_CNTL to leave alone **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) | 0x1);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("SRC_OFFSET:              0x%08x\n", rd_src_offset(dev));
  printf("SRC_PITCH:               0x%08x\n", rd_src_pitch(dev));
  printf("\n");
}

void test_dst_pitch_offset_cntl(ati_device_t *dev) {
  printf("Test DST pitch and offset\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_OFFSET to 0x0 **\n");
  printf("** Initializing DEFAULT_PITCH to 0x0 **\n");
  printf("** Initializing DST_OFFSET to 0x0 **\n");
  printf("** Initializing DST_PITCH to 0x0 **\n");

  wr_default_offset(dev, 0x0);
  wr_default_pitch(dev, 0x0);
  wr_dst_offset(dev, 0x0);
  wr_dst_pitch(dev, 0x0);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("DST_OFFSET:              0x%08x\n", rd_dst_offset(dev));
  printf("DST_PITCH:               0x%08x\n", rd_dst_pitch(dev));
  printf("\n");

  printf("** Setting DEFAULT_OFFSET to 0x000000aa **\n");
  wr_default_offset(dev, 0x000000aa);
  printf("** Setting DEFAULT_PITCH to 0x000000bb **\n");
  wr_default_pitch(dev, 0x000000bb);
  printf("** Setting DST_OFFSET to 0x11 **\n");
  wr_dst_offset(dev, 0x00000011);
  printf("** Setting DST_PITCH to 0x22 **\n");
  wr_dst_pitch(dev, 0x00000022);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("DST_OFFSET:              0x%08x\n", rd_dst_offset(dev));
  printf("DST_PITCH:               0x%08x\n", rd_dst_pitch(dev));
  printf("\n");

  printf("** Setting GMC_DST_PITCH_OFFSET_CNTL to default **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) & ~0x2);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("DST_OFFSET:              0x%08x\n", rd_dst_offset(dev));
  printf("DST_PITCH:               0x%08x\n", rd_dst_pitch(dev));
  printf("\n");

  printf("** Setting GMC_DST_PITCH_OFFSET_CNTL to leave alone **\n");
  wr_dp_gui_master_cntl(dev, rd_dp_gui_master_cntl(dev) | 0x2);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", rd_dp_gui_master_cntl(dev));
  printf("DEFAULT_OFFSET:          0x%08x\n", rd_default_offset(dev));
  printf("DEFAULT_PITCH:           0x%08x\n", rd_default_pitch(dev));
  printf("DST_OFFSET:              0x%08x\n", rd_dst_offset(dev));
  printf("DST_PITCH:               0x%08x\n", rd_dst_pitch(dev));
  printf("\n");
}

void register_pitch_offset_cntl_tests(void) {
  register_test("SRC pitch offset", test_src_pitch_offset_cntl);
  register_test("DST pitch offset", test_dst_pitch_offset_cntl);
}
