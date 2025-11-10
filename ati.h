#ifndef ATI_H
#define ATI_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct ati_device ati_device_t;

ati_device_t *ati_device_init(void);
void ati_device_destroy(ati_device_t *dev);

uint32_t ati_reg_read(ati_device_t *dev, uint32_t offset);
void ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value);
uint32_t ati_vram_read(ati_device_t *dev, uint32_t offset);
void ati_vram_write(ati_device_t *dev, uint32_t offset, uint32_t value);
#define VRAM_NOT_FOUND UINT64_MAX
uint64_t ati_vram_search(ati_device_t *dev, uint32_t needle);
void ati_vram_clear(ati_device_t *dev);
void ati_screen_clear(ati_device_t *dev);
void ati_vram_dump(ati_device_t *dev, const char *filename);
void ati_screen_dump(ati_device_t *dev, const char *filename);
void ati_vram_memcpy(ati_device_t *dev, uint32_t dst_offset,
                     const void *src, size_t size);
bool ati_screen_compare_file(ati_device_t *dev, const char *filename);

#define DP_GUI_MASTER_CNTL       0x146c
uint32_t rd_dp_gui_master_cntl(ati_device_t *dev);
void wr_dp_gui_master_cntl(ati_device_t *dev, uint32_t val);

#define SRC_SC_BOTTOM_RIGHT      0x16f4
void wr_src_sc_bottom_right(ati_device_t *dev, uint32_t val);

#define SRC_SC_BOTTOM            0x165C
uint32_t rd_src_sc_bottom(ati_device_t *dev);
void wr_src_sc_bottom(ati_device_t *dev, uint32_t val);

#define SRC_SC_RIGHT             0x1654
uint32_t rd_src_sc_right(ati_device_t *dev);
void wr_src_sc_right(ati_device_t *dev, uint32_t val);

#define DEFAULT_SC_BOTTOM_RIGHT  0x16e8
uint32_t rd_default_sc_bottom_right(ati_device_t *dev);
void wr_default_sc_bottom_right(ati_device_t *dev, uint32_t val);

#define SC_LEFT                  0x1640
uint32_t rd_sc_left(ati_device_t *dev);
void wr_sc_left(ati_device_t *dev, uint32_t val);

#define SC_TOP                   0x1648
uint32_t rd_sc_top(ati_device_t *dev);
void wr_sc_top(ati_device_t *dev, uint32_t val);

#define SC_BOTTOM_RIGHT          0x16f0
void wr_sc_bottom_right(ati_device_t *dev, uint32_t val);

#define SC_TOP_LEFT              0x16ec
void wr_sc_top_left(ati_device_t *dev, uint32_t val);

#define SC_RIGHT                 0x1644
uint32_t rd_sc_right(ati_device_t *dev);
void wr_sc_right(ati_device_t *dev, uint32_t val);

#define SC_BOTTOM                0x164C
uint32_t rd_sc_bottom(ati_device_t *dev);
void wr_sc_bottom(ati_device_t *dev, uint32_t val);

#define DST_OFFSET               0x1404
uint32_t rd_dst_offset(ati_device_t *dev);
void wr_dst_offset(ati_device_t *dev, uint32_t val);

#define DST_PITCH                0x1408
uint32_t rd_dst_pitch(ati_device_t *dev);
void wr_dst_pitch(ati_device_t *dev, uint32_t val);

#define DEFAULT_OFFSET           0x16e0
uint32_t rd_default_offset(ati_device_t *dev);
void wr_default_offset(ati_device_t *dev, uint32_t val);

#define DEFAULT_PITCH            0x16e4
uint32_t rd_default_pitch(ati_device_t *dev);
void wr_default_pitch(ati_device_t *dev, uint32_t val);

#define SRC_OFFSET               0x15ac
uint32_t rd_src_offset(ati_device_t *dev);
void wr_src_offset(ati_device_t *dev, uint32_t val);

#define SRC_PITCH                0x15b0
uint32_t rd_src_pitch(ati_device_t *dev);
void wr_src_pitch(ati_device_t *dev, uint32_t val);

#define DST_X_Y                  0x1594
void wr_dst_x_y(ati_device_t *dev, uint32_t val);

#define DST_Y_X                  0x1438
void wr_dst_y_x(ati_device_t *dev, uint32_t val);

#define DST_X                    0x141c
uint32_t rd_dst_x(ati_device_t *dev);
void wr_dst_x(ati_device_t *dev, uint32_t val);

#define DST_Y                    0x1420
uint32_t rd_dst_y(ati_device_t *dev);
void wr_dst_y(ati_device_t *dev, uint32_t val);

#define DST_WIDTH_HEIGHT         0x1598
void wr_dst_width_height(ati_device_t *dev, uint32_t val);

#define DST_WIDTH                0x140c
uint32_t rd_dst_width(ati_device_t *dev);
void wr_dst_width(ati_device_t *dev, uint32_t val);

#define DST_HEIGHT               0x1410
uint32_t rd_dst_height(ati_device_t *dev);
void wr_dst_height(ati_device_t *dev, uint32_t val);

#define DP_DATATYPE              0x16c4
uint32_t rd_dp_datatype(ati_device_t *dev);
void wr_dp_datatype(ati_device_t *dev, uint32_t val);

#define DP_MIX                   0x16c8
uint32_t rd_dp_mix(ati_device_t *dev);
void wr_dp_mix(ati_device_t *dev, uint32_t val);

#define DP_WRITE_MSK             0x16cc
uint32_t rd_dp_write_msk(ati_device_t *dev);
void wr_dp_write_msk(ati_device_t *dev, uint32_t val);

#define DP_SRC_FRGD_CLR          0x15d8
uint32_t rd_dp_src_frgd_clr(ati_device_t *dev);
void wr_dp_src_frgd_clr(ati_device_t *dev, uint32_t val);

#define DP_SRC_BKGD_CLR          0x15dc
uint32_t rd_dp_src_bkgd_clr(ati_device_t *dev);
void wr_dp_src_bkgd_clr(ati_device_t *dev, uint32_t val);

#define GUI_STAT                 0x1740
uint32_t rd_gui_stat(ati_device_t *dev);

#define DP_CNTL                  0x16c0
uint32_t rd_dp_cntl(ati_device_t *dev);
void wr_dp_cntl(ati_device_t *dev, uint32_t val);

#define HOST_DATA0               0x17c0
void wr_host_data0(ati_device_t *dev, uint32_t val);

#define HOST_DATA1               0x17c4
void wr_host_data1(ati_device_t *dev, uint32_t val);

#define HOST_DATA2               0x17c8
void wr_host_data2(ati_device_t *dev, uint32_t val);

#define HOST_DATA3               0x17cc
void wr_host_data3(ati_device_t *dev, uint32_t val);

#define HOST_DATA4               0x17d0
void wr_host_data4(ati_device_t *dev, uint32_t val);

#define HOST_DATA5               0x17d4
void wr_host_data5(ati_device_t *dev, uint32_t val);

#define HOST_DATA6               0x17d8
void wr_host_data6(ati_device_t *dev, uint32_t val);

#define HOST_DATA7               0x17dc
void wr_host_data7(ati_device_t *dev, uint32_t val);

#define HOST_DATA_LAST           0x17e0
void wr_host_data_last(ati_device_t *dev, uint32_t val);

#endif
