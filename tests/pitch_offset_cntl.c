#include "../common.h"
#include "../ati.h"

void test_src_pitch_offset_cntl(ati_device_t *dev) {
  uint32_t dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  uint32_t default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  uint32_t default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  uint32_t src_offset = ati_reg_read(dev, SRC_OFFSET);
  uint32_t src_pitch = ati_reg_read(dev, SRC_PITCH);

  printf("Test SRC pitch and offset\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_OFFSET to 0x0 **\n");
  printf("** Initializing DEFAULT_PITCH to 0x0 **\n");
  printf("** Initializing SRC_OFFSET to 0x0 **\n");
  printf("** Initializing SRC_PITCH to 0x0 **\n");

  ati_reg_write(dev, DEFAULT_OFFSET, 0x0);
  ati_reg_write(dev, DEFAULT_PITCH, 0x0);
  ati_reg_write(dev, SRC_OFFSET, 0x0);
  ati_reg_write(dev, SRC_PITCH, 0x0);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  src_offset = ati_reg_read(dev, SRC_OFFSET);
  src_pitch = ati_reg_read(dev, SRC_PITCH);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("SRC_OFFSET:              0x%08x\n", src_offset);
  printf("SRC_PITCH:               0x%08x\n", src_pitch);
  printf("\n");

  printf("** Setting DEFAULT_OFFSET to 0x000000aa **\n");
  printf("** Setting DEFAULT_PITCH to 0x000000bb **\n");
  printf("** Setting SRC_OFFSET to 0x11 **\n");
  printf("** Setting SRC_PITCH to 0x22 **\n");
  ati_reg_write(dev, DEFAULT_OFFSET, 0x000000aa);
  ati_reg_write(dev, DEFAULT_PITCH, 0x000000bb);
  ati_reg_write(dev, SRC_OFFSET, 0x00000011);
  ati_reg_write(dev, SRC_PITCH, 0x00000022);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  src_offset = ati_reg_read(dev, SRC_OFFSET);
  src_pitch = ati_reg_read(dev, SRC_PITCH);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("SRC_OFFSET:              0x%08x\n", src_offset);
  printf("SRC_PITCH:               0x%08x\n", src_pitch);
  printf("\n");

  printf("** Setting GMC_SRC_PITCH_OFFSET_CNTL to default **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x1);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  src_offset = ati_reg_read(dev, SRC_OFFSET);
  src_pitch = ati_reg_read(dev, SRC_PITCH);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("SRC_OFFSET:              0x%08x\n", src_offset);
  printf("SRC_PITCH:               0x%08x\n", src_pitch);
  printf("\n");

  printf("** Setting GMC_SRC_PITCH_OFFSET_CNTL to leave alone **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x1);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  src_offset = ati_reg_read(dev, SRC_OFFSET);
  src_pitch = ati_reg_read(dev, SRC_PITCH);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("SRC_OFFSET:              0x%08x\n", src_offset);
  printf("SRC_PITCH:               0x%08x\n", src_pitch);
  printf("\n");
}

void test_dst_pitch_offset_cntl(ati_device_t *dev) {
  uint32_t dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  uint32_t default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  uint32_t default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  uint32_t dst_offset = ati_reg_read(dev, DST_OFFSET);
  uint32_t dst_pitch = ati_reg_read(dev, DST_PITCH);

  printf("Test DST pitch and offset\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_OFFSET to 0x0 **\n");
  printf("** Initializing DEFAULT_PITCH to 0x0 **\n");
  printf("** Initializing DST_OFFSET to 0x0 **\n");
  printf("** Initializing DST_PITCH to 0x0 **\n");

  ati_reg_write(dev, DEFAULT_OFFSET, 0x0);
  ati_reg_write(dev, DEFAULT_PITCH, 0x0);
  ati_reg_write(dev, DST_OFFSET, 0x0);
  ati_reg_write(dev, DST_PITCH, 0x0);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  dst_offset = ati_reg_read(dev, DST_OFFSET);
  dst_pitch = ati_reg_read(dev, DST_PITCH);

  printf("\n");
  printf("Initial State\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("DST_OFFSET:              0x%08x\n", dst_offset);
  printf("DST_PITCH:               0x%08x\n", dst_pitch);
  printf("\n");

  printf("** Setting DEFAULT_OFFSET to 0x000000aa **\n");
  printf("** Setting DEFAULT_PITCH to 0x000000bb **\n");
  printf("** Setting DST_OFFSET to 0x11 **\n");
  printf("** Setting DST_PITCH to 0x22 **\n");
  ati_reg_write(dev, DEFAULT_OFFSET, 0x000000aa);
  ati_reg_write(dev, DEFAULT_PITCH, 0x000000bb);
  ati_reg_write(dev, DST_OFFSET, 0x00000011);
  ati_reg_write(dev, DST_PITCH, 0x00000022);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  dst_offset = ati_reg_read(dev, DST_OFFSET);
  dst_pitch = ati_reg_read(dev, DST_PITCH);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("DST_OFFSET:              0x%08x\n", dst_offset);
  printf("DST_PITCH:               0x%08x\n", dst_pitch);
  printf("\n");

  printf("** Setting GMC_DST_PITCH_OFFSET_CNTL to default **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x1);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  dst_offset = ati_reg_read(dev, DST_OFFSET);
  dst_pitch = ati_reg_read(dev, DST_PITCH);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("DST_OFFSET:              0x%08x\n", dst_offset);
  printf("DST_PITCH:               0x%08x\n", dst_pitch);
  printf("\n");

  printf("** Setting GMC_DST_PITCH_OFFSET_CNTL to leave alone **\n");
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x1);
  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = ati_reg_read(dev, DP_GUI_MASTER_CNTL);
  default_offset = ati_reg_read(dev, DEFAULT_OFFSET);
  default_pitch = ati_reg_read(dev, DEFAULT_PITCH);
  dst_offset = ati_reg_read(dev, DST_OFFSET);
  dst_pitch = ati_reg_read(dev, DST_PITCH);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_OFFSET:          0x%08x\n", default_offset);
  printf("DEFAULT_PITCH:           0x%08x\n", default_pitch);
  printf("DST_OFFSET:              0x%08x\n", dst_offset);
  printf("DST_PITCH:               0x%08x\n", dst_pitch);
  printf("\n");
}

void register_pitch_offset_cntl_tests(void) {
  register_test("SRC pitch offset", test_src_pitch_offset_cntl);
  register_test("DST pitch offset", test_dst_pitch_offset_cntl);
}
