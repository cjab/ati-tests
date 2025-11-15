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
void ati_dump_mode(ati_device_t *dev);

void ati_dump_all_registers(ati_device_t *dev);
void ati_dump_registers(ati_device_t *dev, int count, ...);

#define DUMP_REGISTERS(dev, ...) \
  ati_dump_registers(dev, \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t), \
    __VA_ARGS__)

#define ATI_REGISTERS \
  /* Datapath / Drawing Engine Registers */ \
  X(dp_gui_master_cntl,       DP_GUI_MASTER_CNTL,      0x146c, RW) \
  X(dp_datatype,              DP_DATATYPE,             0x16c4, RW) \
  X(dp_mix,                   DP_MIX,                  0x16c8, RW) \
  X(dp_write_msk,             DP_WRITE_MSK,            0x16cc, RW) \
  X(dp_cntl,                  DP_CNTL,                 0x16c0, RW) \
  X(dp_src_frgd_clr,          DP_SRC_FRGD_CLR,         0x15d8, RW) \
  X(dp_src_bkgd_clr,          DP_SRC_BKGD_CLR,         0x15dc, RW) \
  \
  /* Scissor / Clipping Registers */ \
  X(sc_left,                  SC_LEFT,                 0x1640, RW) \
  X(sc_top,                   SC_TOP,                  0x1648, RW) \
  X(sc_right,                 SC_RIGHT,                0x1644, RW) \
  X(sc_bottom,                SC_BOTTOM,               0x164c, RW) \
  X(sc_top_left,              SC_TOP_LEFT,             0x16ec, RW) \
  X(sc_bottom_right,          SC_BOTTOM_RIGHT,         0x16f0, RW) \
  X(src_sc_bottom,            SRC_SC_BOTTOM,           0x165c, RW) \
  X(src_sc_right,             SRC_SC_RIGHT,            0x1654, RW) \
  X(src_sc_bottom_right,      SRC_SC_BOTTOM_RIGHT,     0x16f4, RW) \
  X(default_sc_bottom_right,  DEFAULT_SC_BOTTOM_RIGHT, 0x16e8, RW) \
  \
  /* Destination Registers */ \
  X(dst_offset,               DST_OFFSET,              0x1404, RW) \
  X(dst_pitch,                DST_PITCH,               0x1408, RW) \
  X(dst_x,                    DST_X,                   0x141c, RW) \
  X(dst_y,                    DST_Y,                   0x1420, RW) \
  X(dst_x_y,                  DST_X_Y,                 0x1594, WO) \
  X(dst_y_x,                  DST_Y_X,                 0x1438, WO) \
  X(dst_width,                DST_WIDTH,               0x140c, RW) \
  X(dst_height,               DST_HEIGHT,              0x1410, RW) \
  X(dst_width_height,         DST_WIDTH_HEIGHT,        0x1598, RW) \
  \
  /* Source Registers */ \
  X(src_offset,               SRC_OFFSET,              0x15ac, RW) \
  X(src_pitch,                SRC_PITCH,               0x15b0, RW) \
  X(src_x,                    SRC_X,                   0x141c, RW) \
  X(src_y,                    SRC_Y,                   0x1420, RW) \
  X(src_x_y,                  SRC_X_Y,                 0x1594, WO) \
  X(src_y_x,                  SRC_Y_X,                 0x1438, WO) \
  \
  /* Default Registers */ \
  X(default_offset,           DEFAULT_OFFSET,          0x16e0, RW) \
  X(default_pitch,            DEFAULT_PITCH,           0x16e4, RW) \
  \
  /* CRTC Registers */ \
  X(crtc_h_total_disp,        CRTC_H_TOTAL_DISP,       0x0200, RW) \
  X(crtc_h_sync_strt_wid,     CRTC_H_SYNC_STRT_WID,    0x0204, RW) \
  X(crtc_v_total_disp,        CRTC_V_TOTAL_DISP,       0x0208, RW) \
  X(crtc_v_sync_strt_wid,     CRTC_V_SYNC_STRT_WID,    0x020c, RW) \
  X(crtc_gen_cntl,            CRTC_GEN_CNTL,           0x0050, RW) \
  X(crtc_ext_cntl,            CRTC_EXT_CNTL,           0x0054, RW) \
  X(crtc_offset,              CRTC_OFFSET,             0x0224, RW) \
  X(crtc_offset_cntl,         CRTC_OFFSET_CNTL,        0x0228, RW) \
  X(crtc_pitch,               CRTC_PITCH,              0x022c, RW) \
  \
  /* DAC Registers */ \
  X(dac_cntl,                 DAC_CNTL,                0x0058, RW) \
  \
  /* Status Registers */ \
  X(gui_stat,                 GUI_STAT,                0x1740, RO) \
  \
  /* Host Data Registers */ \
  X(host_data0,               HOST_DATA0,              0x17c0, WO) \
  X(host_data1,               HOST_DATA1,              0x17c4, WO) \
  X(host_data2,               HOST_DATA2,              0x17c8, WO) \
  X(host_data3,               HOST_DATA3,              0x17cc, WO) \
  X(host_data4,               HOST_DATA4,              0x17d0, WO) \
  X(host_data5,               HOST_DATA5,              0x17d4, WO) \
  X(host_data6,               HOST_DATA6,              0x17d8, WO) \
  X(host_data7,               HOST_DATA7,              0x17dc, WO) \
  X(host_data_last,           HOST_DATA_LAST,          0x17e0, WO)

// Register offset enum
#define X(func_name, const_name, offset, mode) const_name = offset,
  enum {
      ATI_REGISTERS
  };
#undef X

// Read functions
#define X(func_name, const_name, offset, mode) \
  X_##mode##_READ_DECL(func_name)

  #define X_RW_READ_DECL(func_name) \
    uint32_t rd_##func_name(ati_device_t *dev);

  #define X_RO_READ_DECL(func_name) \
    uint32_t rd_##func_name(ati_device_t *dev);

  #define X_WO_READ_DECL(func_name)  /* No read for write-only */
  ATI_REGISTERS

#undef X
#undef X_RW_READ_DECL
#undef X_RO_READ_DECL
#undef X_WO_READ_DECL

// Write functions
#define X(func_name, const_name, offset, mode) \
  X_##mode##_WRITE_DECL(func_name)

  #define X_RW_WRITE_DECL(func_name) \
    void wr_##func_name(ati_device_t *dev, uint32_t val);

  #define X_WO_WRITE_DECL(func_name) \
    void wr_##func_name(ati_device_t *dev, uint32_t val);

  #define X_RO_WRITE_DECL(func_name)  /* No write for read-only */
  ATI_REGISTERS

#undef X
#undef X_RW_WRITE_DECL
#undef X_WO_WRITE_DECL
#undef X_RO_WRITE_DECL

#endif
