#ifndef ATI_H
#define ATI_H

#include <stdint.h>

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

typedef struct ati_device ati_device_t;

ati_device_t *ati_device_init(void);
void ati_device_destroy(ati_device_t *dev);

uint32_t ati_reg_read(ati_device_t *dev, uint32_t offset);
void ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value);

#endif
