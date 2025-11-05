/*
 * ATI Rage 128 Pro Clipping Mode Hardware Test
 *
 * Tests whether clipping mode bits exhibit latching or dynamic behavior
 *
 * Build: gcc -std=c99 -o test test.c -lpci
 * Requirements: libpci-dev, root privileges, ATI Rage 128 Pro hardware
 * Note: Run with X.org idle (SSH session recommended)
 */
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pci/pci.h>

#define ATI_VENDOR_ID            0x1002
#define MAX_ATI_DEVICES          10

#define DP_GUI_MASTER_CNTL       0x146c

#define SRC_SC_BOTTOM_RIGHT      0x16f4
#define SRC_SC_BOTTOM            0x165C
#define SRC_SC_RIGHT             0x1654
#define DEFAULT_SC_BOTTOM_RIGHT  0x16e8

#define SC_TOP_LEFT              0x1640
#define SC_LEFT                  0x1640
#define SC_TOP                   0x1648

#define SC_BOTTOM_RIGHT          0x1644
#define SC_RIGHT                 0x1644
#define SC_BOTTOM                0x164C

#define DST_OFFSET               0x1404
#define DST_PITCH                0x1408
#define DP_BRUSH_FRGD_CLR        0x147c
#define DP_WRITE_MASK            0x16cc
#define DST_HEIGHT               0x1410
#define DST_X_Y                  0x1594
#define DST_WIDTH_X              0x1588

#define DEFAULT_OFFSET           0x16e0
#define DEFAULT_PITCH            0x16e4
#define SRC_OFFSET               0x15ac
#define SRC_PITCH                0x15b0

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

void run_tests(void *bar2);
struct pci_dev *find_device(struct pci_access *pacc,
                            char *name_out, int name_len);
void print_devices(struct pci_access *pacc);
void *map_bar(struct pci_dev *dev, int bar_idx);
static inline uint32_t reg_read(void *base, uint32_t offset);
static inline uint32_t reg_write(void *base, uint32_t offset, uint32_t value);

int main(int argc, char **argv) {
  struct pci_access *pacc = pci_alloc();
  char name[256];
  struct pci_dev *dev = find_device(pacc, name, sizeof(name));
  void *bar2 = map_bar(dev, 2);

  run_tests(bar2);

  return 0;
}

void test_dst_clipping(void *bar2) {
  uint32_t dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  uint32_t default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  uint32_t sc_bottom = reg_read(bar2, SC_BOTTOM);
  uint32_t sc_right = reg_read(bar2, SC_RIGHT);
  uint32_t sc_top = reg_read(bar2, SC_TOP);
  uint32_t sc_left = reg_read(bar2, SC_LEFT);

  printf("Test DST clipping\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_SC_BOTTOM_RIGHT to 0x0 **\n");
  printf("** Initializing SC_BOTTOM to 0x0 **\n");
  printf("** Initializing SC_RIGHT to 0x0 **\n");
  printf("** Initializing SC_TOP to 0x0 **\n");
  printf("** Initializing SC_LEFT to 0x0 **\n");

  reg_write(bar2, DEFAULT_SC_BOTTOM_RIGHT, 0x0);
  reg_write(bar2, SC_BOTTOM, 0x0);
  reg_write(bar2, SC_RIGHT, 0x0);
  reg_write(bar2, SC_TOP, 0x0);
  reg_write(bar2, SC_LEFT, 0x0);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = reg_read(bar2, SC_BOTTOM);
  sc_right = reg_read(bar2, SC_RIGHT);
  sc_top = reg_read(bar2, SC_TOP);
  sc_left = reg_read(bar2, SC_LEFT);

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
  reg_write(bar2, DEFAULT_SC_BOTTOM_RIGHT, 0x0aaa0bbb);
  reg_write(bar2, SC_BOTTOM, 0x00000111);
  reg_write(bar2, SC_RIGHT, 0x00000222);
  reg_write(bar2, SC_TOP, 0x00000333);
  reg_write(bar2, SC_LEFT, 0x00000444);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = reg_read(bar2, SC_BOTTOM);
  sc_right = reg_read(bar2, SC_RIGHT);
  sc_top = reg_read(bar2, SC_TOP);
  sc_left = reg_read(bar2, SC_LEFT);

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
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x8);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = reg_read(bar2, SC_BOTTOM);
  sc_right = reg_read(bar2, SC_RIGHT);
  sc_top = reg_read(bar2, SC_TOP);
  sc_left = reg_read(bar2, SC_LEFT);

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
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x8);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  sc_bottom = reg_read(bar2, SC_BOTTOM);
  sc_right = reg_read(bar2, SC_RIGHT);
  sc_top = reg_read(bar2, SC_TOP);
  sc_left = reg_read(bar2, SC_LEFT);

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

void test_src_clipping(void *bar2) {
  uint32_t dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  uint32_t default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  uint32_t src_sc_bottom = reg_read(bar2, SRC_SC_BOTTOM);
  uint32_t src_sc_right = reg_read(bar2, SRC_SC_RIGHT);

  printf("Test SRC clipping\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_SC_BOTTOM_RIGHT to 0x0 **\n");
  printf("** Initializing SRC_SC_BOTTOM to 0x0 **\n");
  printf("** Initializing SRC_SC_RIGHT to 0x0 **\n");

  reg_write(bar2, DEFAULT_SC_BOTTOM_RIGHT, 0x0);
  reg_write(bar2, SRC_SC_BOTTOM, 0x0);
  reg_write(bar2, SRC_SC_RIGHT, 0x0);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = reg_read(bar2, SRC_SC_BOTTOM);
  src_sc_right = reg_read(bar2, SRC_SC_RIGHT);

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
  reg_write(bar2, DEFAULT_SC_BOTTOM_RIGHT, 0x0aaa0bbb);
  reg_write(bar2, SRC_SC_BOTTOM, 0x00000111);
  reg_write(bar2, SRC_SC_RIGHT, 0x00000222);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = reg_read(bar2, SRC_SC_BOTTOM);
  src_sc_right = reg_read(bar2, SRC_SC_RIGHT);

  printf("\n");
  printf("State After Setting\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");

  printf("** Setting GMC_SRC_CLIPPING to default **\n");
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x4);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = reg_read(bar2, SRC_SC_BOTTOM);
  src_sc_right = reg_read(bar2, SRC_SC_RIGHT);

  printf("\n");
  printf("State After Setting Default\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");

  printf("** Setting GMC_SRC_CLIPPING to leave alone **\n");
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x4);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_sc_bottom_right = reg_read(bar2, DEFAULT_SC_BOTTOM_RIGHT);
  src_sc_bottom = reg_read(bar2, SRC_SC_BOTTOM);
  src_sc_right = reg_read(bar2, SRC_SC_RIGHT);

  printf("\n");
  printf("State After Setting Leave Alone\n");
  printf("------------------------------------\n");
  printf("DP_GUI_MASTER_CNTL:      0x%08x\n", dp_gui_master_cntl);
  printf("DEFAULT_SC_BOTTOM_RIGHT: 0x%08x\n", default_sc_bottom_right);
  printf("SRC_SC_BOTTOM:           0x%08x\n", src_sc_bottom);
  printf("SRC_SC_RIGHT:            0x%08x\n", src_sc_right);
  printf("\n");
}

void test_src_pitch_offset_cntl(void *bar2) {
  uint32_t dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  uint32_t default_offset = reg_read(bar2, DEFAULT_OFFSET);
  uint32_t default_pitch = reg_read(bar2, DEFAULT_PITCH);
  uint32_t src_offset = reg_read(bar2, SRC_OFFSET);
  uint32_t src_pitch = reg_read(bar2, SRC_PITCH);

  printf("Test SRC pitch and offset\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_OFFSET to 0x0 **\n");
  printf("** Initializing DEFAULT_PITCH to 0x0 **\n");
  printf("** Initializing SRC_OFFSET to 0x0 **\n");
  printf("** Initializing SRC_PITCH to 0x0 **\n");

  reg_write(bar2, DEFAULT_OFFSET, 0x0);
  reg_write(bar2, DEFAULT_PITCH, 0x0);
  reg_write(bar2, SRC_OFFSET, 0x0);
  reg_write(bar2, SRC_PITCH, 0x0);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  src_offset = reg_read(bar2, SRC_OFFSET);
  src_pitch = reg_read(bar2, SRC_PITCH);

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
  reg_write(bar2, DEFAULT_OFFSET, 0x000000aa);
  reg_write(bar2, DEFAULT_PITCH, 0x000000bb);
  reg_write(bar2, SRC_OFFSET, 0x00000011);
  reg_write(bar2, SRC_PITCH, 0x00000022);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  src_offset = reg_read(bar2, SRC_OFFSET);
  src_pitch = reg_read(bar2, SRC_PITCH);

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
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x1);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  src_offset = reg_read(bar2, SRC_OFFSET);
  src_pitch = reg_read(bar2, SRC_PITCH);

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
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x1);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  src_offset = reg_read(bar2, SRC_OFFSET);
  src_pitch = reg_read(bar2, SRC_PITCH);

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

void test_dst_pitch_offset_cntl(void *bar2) {
  uint32_t dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  uint32_t default_offset = reg_read(bar2, DEFAULT_OFFSET);
  uint32_t default_pitch = reg_read(bar2, DEFAULT_PITCH);
  uint32_t dst_offset = reg_read(bar2, DST_OFFSET);
  uint32_t dst_pitch = reg_read(bar2, DST_PITCH);

  printf("Test DST pitch and offset\n");
  printf("====================================\n\n");

  printf("** Initializing DEFAULT_OFFSET to 0x0 **\n");
  printf("** Initializing DEFAULT_PITCH to 0x0 **\n");
  printf("** Initializing DST_OFFSET to 0x0 **\n");
  printf("** Initializing DST_PITCH to 0x0 **\n");

  reg_write(bar2, DEFAULT_OFFSET, 0x0);
  reg_write(bar2, DEFAULT_PITCH, 0x0);
  reg_write(bar2, DST_OFFSET, 0x0);
  reg_write(bar2, DST_PITCH, 0x0);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  dst_offset = reg_read(bar2, DST_OFFSET);
  dst_pitch = reg_read(bar2, DST_PITCH);

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
  reg_write(bar2, DEFAULT_OFFSET, 0x000000aa);
  reg_write(bar2, DEFAULT_PITCH, 0x000000bb);
  reg_write(bar2, DST_OFFSET, 0x00000011);
  reg_write(bar2, DST_PITCH, 0x00000022);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  dst_offset = reg_read(bar2, DST_OFFSET);
  dst_pitch = reg_read(bar2, DST_PITCH);

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
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl & ~0x1);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  dst_offset = reg_read(bar2, DST_OFFSET);
  dst_pitch = reg_read(bar2, DST_PITCH);

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
  reg_write(bar2, DP_GUI_MASTER_CNTL, dp_gui_master_cntl | 0x1);
  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);

  dp_gui_master_cntl = reg_read(bar2, DP_GUI_MASTER_CNTL);
  default_offset = reg_read(bar2, DEFAULT_OFFSET);
  default_pitch = reg_read(bar2, DEFAULT_PITCH);
  dst_offset = reg_read(bar2, DST_OFFSET);
  dst_pitch = reg_read(bar2, DST_PITCH);

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

void run_tests(void *bar2) {
  test_src_clipping(bar2);
  test_dst_clipping(bar2);

  test_src_pitch_offset_cntl(bar2);
  test_dst_pitch_offset_cntl(bar2);
}

struct pci_dev *find_device(struct pci_access *pacc,
                            char *name_out, int name_len) {
  struct pci_dev *dev, *it;
  int device_count = 0;

  pci_init(pacc);
  pci_scan_bus(pacc);

  for (it = pacc->devices; it; it = it->next) {
    if (it->vendor_id == ATI_VENDOR_ID) {
      if (device_count == 0) {
        dev = it;
      }
      device_count += 1;
    }
  }

  if (device_count == 0) {
    printf("No ATI devices found\n");
    exit(1);
  }

  if (device_count > 1) {
    printf("Found multiple ATI devices:\n");
    print_devices(pacc);
  }

  pci_lookup_name(pacc, name_out, name_len,
                  PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
                  dev->vendor_id, dev->device_id);

  printf("# %s\n\n", name_out);

  return dev;
}

void print_devices(struct pci_access *pacc) {
  struct pci_dev *dev;
  char name[256];

  for (dev = pacc->devices; dev; dev = dev->next) {
    if (dev->vendor_id != ATI_VENDOR_ID) continue;
    pci_lookup_name(pacc, name, sizeof(name),
                    PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
                    dev->vendor_id, dev->device_id);
    printf("\t- %s\n", name);
  }
}

void *map_bar(struct pci_dev *dev, int bar_idx) {
  char pci_loc[32];
  sprintf(pci_loc, "%04x:%02x:%02x.%d", dev->domain, dev->bus,
          dev->dev, dev->func);

  char base_path[256];
  sprintf(base_path, "/sys/bus/pci/devices/%s", pci_loc);

  char bar_path[256];
  sprintf(bar_path, "%s/resource%d", base_path, bar_idx);

  int bar_fd = open(bar_path, O_RDWR | O_SYNC);
  if (bar_fd == -1) FATAL;

  void *bar = mmap(NULL, dev->size[bar_idx], PROT_READ | PROT_WRITE,
                   MAP_SHARED, bar_fd, 0);
  if (bar == (void *) -1) FATAL;

  return bar;
}

static inline uint32_t reg_read(void *base, uint32_t offset) {
  volatile uint32_t *reg = (volatile uint32_t *)(base + offset);
  return *reg;
}

static inline uint32_t reg_write(void *base, uint32_t offset, uint32_t value) {
  volatile uint32_t *reg = (volatile uint32_t *)(base + offset);
  *reg = value;
}
