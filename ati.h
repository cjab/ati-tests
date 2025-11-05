#ifndef ATI_H
#define ATI_H

#include <stdint.h>

typedef struct ati_device ati_device_t;

ati_device_t *ati_device_init(void);
void ati_device_destroy(ati_device_t *dev);

uint32_t ati_reg_read(ati_device_t *dev, uint32_t offset);
void ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value);

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

#endif
