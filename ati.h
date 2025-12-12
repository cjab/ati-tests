/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef ATI_H
#define ATI_H

#include <stdbool.h>
#include <stdint.h>

// IWYU pragma: begin_exports
#include "platform/platform.h"
// IWYU pragma: end_exports
//

#define X_RES 640
#define Y_RES 480
#define BPP 32
#define BYPP (BPP / 8)
#define FIFO_MAX 64
#define VRAM_NOT_FOUND UINT64_MAX

typedef struct ati_device ati_device_t;

ati_device_t *ati_device_init(platform_pci_device_t *pci_dev);
void ati_device_destroy(ati_device_t *dev);

uint32_t ati_reg_read(ati_device_t *dev, uint32_t offset);
void ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value);
uint32_t ati_vram_read(ati_device_t *dev, uint32_t offset);
void ati_vram_write(ati_device_t *dev, uint32_t offset, uint32_t value);
uint64_t ati_vram_search(ati_device_t *dev, uint32_t needle);
void ati_vram_clear(ati_device_t *dev);
void ati_screen_clear(ati_device_t *dev);
void ati_vram_dump(ati_device_t *dev, const char *filename);
void ati_screen_dump(ati_device_t *dev, const char *filename);
void ati_vram_memcpy(ati_device_t *dev, uint32_t dst_offset, const void *src,
                     size_t size);
bool ati_screen_async_compare_fixture(ati_device_t *dev,
                                      const char *fixture_name);
bool ati_screen_compare_fixture(ati_device_t *dev, const char *fixture_name);
void ati_dump_mode(ati_device_t *dev);

void ati_dump_all_registers(ati_device_t *dev);
void ati_dump_registers(ati_device_t *dev, int count, ...);
void ati_set_display_mode(ati_device_t *dev);
void ati_init_gui_engine(ati_device_t *dev);
void ati_engine_flush(ati_device_t *dev);
void ati_engine_reset(ati_device_t *dev);
void ati_reset_for_test(ati_device_t *dev);
void ati_wait_for_fifo(ati_device_t *dev, uint32_t entries);
void ati_wait_for_idle(ati_device_t *dev);
void ati_init_cce_engine(ati_device_t *dev);
void ati_top_cce_engine(ati_device_t *dev);

// clang-format off
#define DUMP_REGISTERS(dev, ...) \
  ati_dump_registers(dev, \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t), \
    __VA_ARGS__)

// ============================================================================
// Register Field Infrastructure
// ============================================================================

typedef struct {
    const char *name;
    uint8_t shift;
    uint8_t width;
} field_entry_t;

// ----------------------------------------------------------------------------
// Register field  macros
//
// To define fields for a register:
//   1. Define REGNAME_FIELDS(F, X) using F(name, bit) for flags and
//      X(name, shift, width) for multi-bit fields
//   2. Invoke DEFINE_REG_FIELDS(regname, REGNAME_FIELDS)
//   3. Reference regname_fields in ATI_REGISTERS
// ----------------------------------------------------------------------------

// Enum generators (for constants usable in code)
#define _ENUM_FLAG(name, bit)           name = (1u << (bit)),
#define _ENUM_FIELD(name, shift, width) name##_SHIFT = (shift), \
                                        name##_MASK  = (((1u << (width)) - 1) << (shift)),

// Table generators (for REPL field display)
#define _TABLE_FLAG(name, bit)           {#name, bit, 1},
#define _TABLE_FIELD(name, shift, width) {#name, shift, width},

// Generate both enum constants and field table from a FIELDS macro
#define DEFINE_REG_FIELDS(name, fields_macro) \
    enum { fields_macro(_ENUM_FLAG, _ENUM_FIELD) }; \
    static const field_entry_t name##_fields[] = { \
        fields_macro(_TABLE_FLAG, _TABLE_FIELD) \
        {NULL, 0, 0} \
    };

// ----------------------------------------------------------------------------
// GUI_STAT fields
// ----------------------------------------------------------------------------
#define GUI_STAT_FIELDS(F, X) \
    X(GUI_FIFO_CNT,    0, 12) \
    F(PM4_BUSY,        16) \
    F(MICRO_BUSY,      17) \
    F(FPU_BUSY,        18) \
    F(VC_BUSY,         19) \
    F(IDCT_BUSY,       20) \
    F(ENG_EV_BUSY,     21) \
    F(SETUP_BUSY,      22) \
    F(EDGEWALK_BUSY,   23) \
    F(ADDRESSING_BUSY, 24) \
    F(ENG_3D_BUSY,     25) \
    F(ENG_2D_SM_BUSY,  26) \
    F(ENG_2D_BUSY,     27) \
    F(GUI_WB_BUSY,     28) \
    F(CACHE_BUSY,      29) \
    F(GUI_ACTIVE,      31)

DEFINE_REG_FIELDS(gui_stat, GUI_STAT_FIELDS)

// ----------------------------------------------------------------------------
// CRTC_GEN_CNTL fields
// ----------------------------------------------------------------------------
#define CRTC_GEN_CNTL_FIELDS(F, X) \
    F(CRTC_DBL_SCAN_EN,   0) \
    F(CRTC_INTERLACE_EN,  1) \
    F(CRTC_C_SYNC_EN,     4) \
    X(CRTC_PIX_WIDTH,     8, 3) \
    F(CRTC_CUR_EN,       16) \
    X(CRTC_CUR_MODE,     17, 3) \
    F(CRTC_EXT_DISP_EN,  24) \
    F(CRTC_EN,           25) \
    F(CRTC_DISP_REQ_EN_B,26)

DEFINE_REG_FIELDS(crtc_gen_cntl, CRTC_GEN_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// CRTC_EXT_CNTL fields
// ----------------------------------------------------------------------------
#define CRTC_EXT_CNTL_FIELDS(F, X) \
    F(CRTC_VGA_XOVERSCAN,    0) \
    X(VGA_BLINK_RATE,        1, 2) \
    F(VGA_ATI_LINEAR,        3) \
    F(VGA_128KAP_PAGING,     4) \
    F(VGA_TEXT_132,          5) \
    F(VGA_XCRT_CNT_EN,       6) \
    F(CRTC_HSYNC_DIS,        8) \
    F(CRTC_VSYNC_DIS,        9) \
    F(CRTC_DISPLAY_DIS,     10) \
    F(CRTC_SYNC_TRISTATE,   11) \
    F(CRTC_HSYNC_TRISTATE,  12) \
    F(CRTC_VSYNC_TRISTATE,  13) \
    F(CRTC_CRT_ON,          15) \
    F(VGA_CUR_B_TEST,       17) \
    F(VGA_PACK_DIS,         18) \
    F(VGA_MEM_PS_EN,        19) \
    F(VGA_READ_PREFETCH_DIS,20) \
    F(DFIFO_EXTSENSE,       21) \
    F(FP_OUT_EN,            22) \
    F(FP_ACTIVE,            23) \
    X(VCRTC_IDX_MASTER,     24, 7)

DEFINE_REG_FIELDS(crtc_ext_cntl, CRTC_EXT_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// DAC_CNTL fields
// ----------------------------------------------------------------------------
#define DAC_CNTL_FIELDS(F, X) \
    X(DAC_RANGE_CNTL,        0, 2) \
    F(DAC_BLANKING,          2) \
    F(DAC_CMP_EN,            3) \
    F(DAC_CMP_OUTPUT,        7) \
    F(DAC_8BIT_EN,           8) \
    F(DAC_4BPP_PIX_ORDER,    9) \
    F(DAC_TVO_EN,           10) \
    F(DAC_TVO_OVR_EXCL,     11) \
    F(DAC_TVO_16BPP_DITH_EN,12) \
    F(DAC_VGA_ADR_EN,       13) \
    F(DAC_PDWN,             15) \
    F(DAC_CRC_EN,           19) \
    X(DAC_MASK,             24, 8)

DEFINE_REG_FIELDS(dac_cntl, DAC_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// GEN_RESET_CNTL fields
// ----------------------------------------------------------------------------
#define GEN_RESET_CNTL_FIELDS(F, X) \
    F(SOFT_RESET_GUI, 0)

DEFINE_REG_FIELDS(gen_reset_cntl, GEN_RESET_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// PC_NGUI_CTLSTAT fields
// ----------------------------------------------------------------------------
#define PC_NGUI_CTLSTAT_FIELDS(F, X) \
    X(PC_FLUSH_ALL,  0, 8) \
    F(PC_BUSY,      31)

DEFINE_REG_FIELDS(pc_ngui_ctlstat, PC_NGUI_CTLSTAT_FIELDS)

// ----------------------------------------------------------------------------
// DP_DATATYPE fields
// ----------------------------------------------------------------------------
#define DP_DATATYPE_FIELDS(F, X) \
    F(HOST_BIG_ENDIAN_EN, 29)

DEFINE_REG_FIELDS(dp_datatype, DP_DATATYPE_FIELDS)

// ----------------------------------------------------------------------------
// DP_CNTL fields
// ----------------------------------------------------------------------------
#define DP_CNTL_FIELDS(F, X) \
    F(DST_X_LEFT_TO_RIGHT, 0) \
    F(DST_Y_TOP_TO_BOTTOM, 1) \
    F(DST_Y_MAJOR,         2)

DEFINE_REG_FIELDS(dp_cntl, DP_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// CRTC_H_TOTAL_DISP fields
// ----------------------------------------------------------------------------
#define CRTC_H_TOTAL_DISP_FIELDS(F, X) \
    X(CRTC_H_TOTAL,  0, 9) \
    X(CRTC_H_DISP,  16, 8)

DEFINE_REG_FIELDS(crtc_h_total_disp, CRTC_H_TOTAL_DISP_FIELDS)

// ----------------------------------------------------------------------------
// CRTC_H_SYNC_STRT_WID fields
// ----------------------------------------------------------------------------
#define CRTC_H_SYNC_STRT_WID_FIELDS(F, X) \
    X(CRTC_H_SYNC_STRT_PIX,   0, 3) \
    X(CRTC_H_SYNC_STRT_CHAR,  3, 9) \
    X(CRTC_H_SYNC_WID,       16, 6) \
    F(CRTC_H_SYNC_POL,       23)

DEFINE_REG_FIELDS(crtc_h_sync_strt_wid, CRTC_H_SYNC_STRT_WID_FIELDS)

// ----------------------------------------------------------------------------
// CRTC_V_TOTAL_DISP fields
// ----------------------------------------------------------------------------
#define CRTC_V_TOTAL_DISP_FIELDS(F, X) \
    X(CRTC_V_TOTAL,  0, 11) \
    X(CRTC_V_DISP,  16, 11)

DEFINE_REG_FIELDS(crtc_v_total_disp, CRTC_V_TOTAL_DISP_FIELDS)

// ----------------------------------------------------------------------------
// CRTC_V_SYNC_STRT_WID fields
// ----------------------------------------------------------------------------
#define CRTC_V_SYNC_STRT_WID_FIELDS(F, X) \
    X(CRTC_V_SYNC_STRT,  0, 11) \
    X(CRTC_V_SYNC_WID,  16, 5) \
    F(CRTC_V_SYNC_POL,  23)

DEFINE_REG_FIELDS(crtc_v_sync_strt_wid, CRTC_V_SYNC_STRT_WID_FIELDS)

// ----------------------------------------------------------------------------
// DEFAULT_OFFSET fields
// ----------------------------------------------------------------------------
#define DEFAULT_OFFSET_FIELDS(F, X) \
    X(DEFAULT_OFFSET, 0, 26)

DEFINE_REG_FIELDS(default_offset, DEFAULT_OFFSET_FIELDS)

// ----------------------------------------------------------------------------
// DEFAULT_PITCH fields
// ----------------------------------------------------------------------------
#define DEFAULT_PITCH_FIELDS(F, X) \
    X(DEFAULT_PITCH,  0, 10) \
    F(DEFAULT_TILE,  16)

DEFINE_REG_FIELDS(default_pitch, DEFAULT_PITCH_FIELDS)

// ----------------------------------------------------------------------------
// DEFAULT_SC_BOTTOM_RIGHT fields
// ----------------------------------------------------------------------------
#define DEFAULT_SC_BOTTOM_RIGHT_FIELDS(F, X) \
    X(DEFAULT_SC_RIGHT,   0, 14) \
    X(DEFAULT_SC_BOTTOM, 16, 14)

DEFINE_REG_FIELDS(default_sc_bottom_right, DEFAULT_SC_BOTTOM_RIGHT_FIELDS)

// ----------------------------------------------------------------------------
// DP_GUI_MASTER_CNTL fields
// ----------------------------------------------------------------------------
#define DP_GUI_MASTER_CNTL_FIELDS(F, X) \
    F(GMC_SRC_PITCH_OFFSET_CNTL,  0) \
    F(GMC_DST_PITCH_OFFSET_CNTL,  1) \
    F(GMC_SRC_CLIPPING,           2) \
    F(GMC_DST_CLIPPING,           3) \
    X(GMC_BRUSH_DATATYPE,         4, 4) \
    X(GMC_DST_DATATYPE,           8, 4) \
    X(GMC_SRC_DATATYPE,          12, 2) \
    F(GMC_BYTE_PIX_ORDER,        14) \
    F(GMC_CONVERSION_TEMP,       15) \
    X(GMC_ROP3,                  16, 8) \
    X(GMC_SRC_SOURCE,            24, 3) \
    F(GMC_3D_FCN_EN,             27) \
    F(GMC_CLR_CMP_CNTL_DIS,      28) \
    F(GMC_AUX_CLIP_DIS,          29) \
    F(GMC_WR_MSK_DIS,            30) \
    F(GMC_LD_BRUSH_Y_X,          31)

DEFINE_REG_FIELDS(dp_gui_master_cntl, DP_GUI_MASTER_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// PM4_STAT fields
// ----------------------------------------------------------------------------
#define PM4_STAT_FIELDS(F, X) \
    X(PM4_FIFOCNT,  0, 12)

DEFINE_REG_FIELDS(pm4_stat, PM4_STAT_FIELDS)

// ----------------------------------------------------------------------------
// PM4_MICRO_CNTL fields
// ----------------------------------------------------------------------------
#define PM4_MICRO_CNTL_FIELDS(F, X) \
    F(PM4_MICRO_FREERUN, 30)

DEFINE_REG_FIELDS(pm4_micro_cntl, PM4_MICRO_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// PM4_BUFFER_CNTL fields
// ----------------------------------------------------------------------------
#define PM4_BUFFER_CNTL_FIELDS(F, X) \
    F(PM4_BUFFER_CNTL_NOUPDATE, 27) \
    X(PM4_BUFFER_MODE,         28, 4)

DEFINE_REG_FIELDS(pm4_buffer_cntl, PM4_BUFFER_CNTL_FIELDS)

// ----------------------------------------------------------------------------
// GEN_INT_CNTL fields
// ----------------------------------------------------------------------------
#define GEN_INT_CNTL_FIELDS(F, X) \
    F(CRTC_VBLANK_INT_EN,    0) \
    F(CRTC_VLINE_INT_EN,     1) \
    F(CRTC_VSYNC_INT_EN,     2) \
    F(SNAPSHOT_INT_EN,       3) \
    F(FP_DETECT_INT_EN,     10) \
    F(BUSMASTER_EOL_INT_EN, 16) \
    F(I2C_INT_EN,           17) \
    F(MPP_GP_INT_EN,        18) \
    F(GUI_IDLE_INT_EN,      19) \
    F(VIPH_INT_EN,          24)

DEFINE_REG_FIELDS(gen_int_cntl, GEN_INT_CNTL_FIELDS)

// ============================================================================
// Register Definitions
// ============================================================================
#define ATI_REGISTERS \
  /* Datapath / Drawing Engine Registers */ \
  X(dp_gui_master_cntl,       DP_GUI_MASTER_CNTL,      0x146c, RW, dp_gui_master_cntl_fields) \
  X(dp_datatype,              DP_DATATYPE,             0x16c4, RW, dp_datatype_fields) \
  X(dp_mix,                   DP_MIX,                  0x16c8, RW, NULL) \
  X(dp_write_msk,             DP_WRITE_MSK,            0x16cc, RW, NULL) \
  X(dp_cntl,                  DP_CNTL,                 0x16c0, RW, dp_cntl_fields) \
  X(dp_brush_bkgd_clr,        DP_BRUSH_BKGD_CLR,       0x15dc, RW, NULL) \
  X(dp_brush_frgd_clr,        DP_BRUSH_FRGD_CLR,       0x1578, RW, NULL) \
  X(dp_src_bkgd_clr,          DP_SRC_BKGD_CLR,         0x15dc, RW, NULL) \
  X(dp_src_frgd_clr,          DP_SRC_FRGD_CLR,         0x15d8, RW, NULL) \
  \
  /* GUI Scratch Registers */ \
  X(gui_scratch_reg0,         GUI_SCRATCH_REG0,        0x15e0, RW, NULL) \
  X(gui_scratch_reg1,         GUI_SCRATCH_REG1,        0x15e4, RW, NULL) \
  X(gui_scratch_reg2,         GUI_SCRATCH_REG2,        0x15e8, RW, NULL) \
  X(gui_scratch_reg3,         GUI_SCRATCH_REG3,        0x15ec, RW, NULL) \
  X(gui_scratch_reg4,         GUI_SCRATCH_REG4,        0x15f0, RW, NULL) \
  X(gui_scratch_reg5,         GUI_SCRATCH_REG5,        0x15f4, RW, NULL) \
  \
  /* Scissor / Clipping Registers */ \
  X(sc_left,                  SC_LEFT,                 0x1640, RW, NULL) \
  X(sc_top,                   SC_TOP,                  0x1648, RW, NULL) \
  X(sc_right,                 SC_RIGHT,                0x1644, RW, NULL) \
  X(sc_bottom,                SC_BOTTOM,               0x164c, RW, NULL) \
  X(sc_top_left,              SC_TOP_LEFT,             0x16ec, RW, NULL) \
  X(sc_bottom_right,          SC_BOTTOM_RIGHT,         0x16f0, RW, NULL) \
  X(src_sc_bottom,            SRC_SC_BOTTOM,           0x165c, RW, NULL) \
  X(src_sc_right,             SRC_SC_RIGHT,            0x1654, RW, NULL) \
  X(src_sc_bottom_right,      SRC_SC_BOTTOM_RIGHT,     0x16f4, RW, NULL) \
  X(default_sc_bottom_right,  DEFAULT_SC_BOTTOM_RIGHT, 0x16e8, RW, default_sc_bottom_right_fields) \
  X(aux_sc_cntl,              AUX_SC_CNTL,             0x1660, RW, NULL) \
  \
  /* Destination Registers */ \
  X(dst_offset,               DST_OFFSET,              0x1404, RW, NULL) \
  X(dst_pitch,                DST_PITCH,               0x1408, RW, NULL) \
  X(dst_x,                    DST_X,                   0x141c, RW, NULL) \
  X(dst_y,                    DST_Y,                   0x1420, RW, NULL) \
  X(dst_x_y,                  DST_X_Y,                 0x1594, WO, NULL) \
  X(dst_y_x,                  DST_Y_X,                 0x1438, WO, NULL) \
  X(dst_width,                DST_WIDTH,               0x140c, RW, NULL) \
  X(dst_height,               DST_HEIGHT,              0x1410, RW, NULL) \
  X(dst_width_height,         DST_WIDTH_HEIGHT,        0x1598, RW, NULL) \
  X(dst_bres_err,             DST_BRES_ERR,            0x1628, RW, NULL) \
  X(dst_bres_inc,             DST_BRES_INC,            0x162c, RW, NULL) \
  X(dst_bres_dec,             DST_BRES_DEC,            0x1630, RW, NULL) \
  \
  /* Source Registers */ \
  X(src_offset,               SRC_OFFSET,              0x15ac, RW, NULL) \
  X(src_pitch,                SRC_PITCH,               0x15b0, RW, NULL) \
  X(src_x,                    SRC_X,                   0x141c, RW, NULL) \
  X(src_y,                    SRC_Y,                   0x1420, RW, NULL) \
  X(src_x_y,                  SRC_X_Y,                 0x1590, WO, NULL) \
  X(src_y_x,                  SRC_Y_X,                 0x1438, WO, NULL) \
  \
  /* Default Registers */ \
  X(default_offset,           DEFAULT_OFFSET,          0x16e0, RW, default_offset_fields) \
  X(default_pitch,            DEFAULT_PITCH,           0x16e4, RW, default_pitch_fields) \
  \
  /* CRTC Registers */ \
  X(crtc_h_total_disp,        CRTC_H_TOTAL_DISP,       0x0200, RW, crtc_h_total_disp_fields) \
  X(crtc_h_sync_strt_wid,     CRTC_H_SYNC_STRT_WID,    0x0204, RW, crtc_h_sync_strt_wid_fields) \
  X(crtc_v_total_disp,        CRTC_V_TOTAL_DISP,       0x0208, RW, crtc_v_total_disp_fields) \
  X(crtc_v_sync_strt_wid,     CRTC_V_SYNC_STRT_WID,    0x020c, RW, crtc_v_sync_strt_wid_fields) \
  X(crtc_gen_cntl,            CRTC_GEN_CNTL,           0x0050, RW, crtc_gen_cntl_fields) \
  X(crtc_ext_cntl,            CRTC_EXT_CNTL,           0x0054, RW, crtc_ext_cntl_fields) \
  X(crtc_offset,              CRTC_OFFSET,             0x0224, RW, NULL) \
  X(crtc_offset_cntl,         CRTC_OFFSET_CNTL,        0x0228, RW, NULL) \
  X(crtc_pitch,               CRTC_PITCH,              0x022c, RW, NULL) \
  \
  /* Reset & Engine Control Registers */ \
  X(gen_reset_cntl,           GEN_RESET_CNTL,          0x00f0, RW, gen_reset_cntl_fields) \
  X(pc_ngui_ctlstat,          PC_NGUI_CTLSTAT,         0x0184, RW, pc_ngui_ctlstat_fields) \
  \
  /* DAC Registers */ \
  X(dac_cntl,                 DAC_CNTL,                0x0058, RW, dac_cntl_fields) \
  \
  /* Palette Registers */ \
  X(palette_index,            PALETTE_INDEX,           0x00b0, RW, NULL) \
  X(palette_data,             PALETTE_DATA,            0x00b4, RW, NULL) \
  \
  /* DDA Registers (Display FIFO Arbitration) */ \
  X(dda_config,               DDA_CONFIG,              0x02e0, RW, NULL) \
  X(dda_on_off,               DDA_ON_OFF,              0x02e4, RW, NULL) \
  \
  /* Overscan Registers */ \
  X(ovr_clr,                  OVR_CLR,                 0x0230, RW, NULL) \
  X(ovr_wid_left_right,       OVR_WID_LEFT_RIGHT,      0x0234, RW, NULL) \
  X(ovr_wid_top_bottom,       OVR_WID_TOP_BOTTOM,      0x0238, RW, NULL) \
  \
  /* Interrupt & Control Registers */ \
  X(gen_int_cntl,             GEN_INT_CNTL,            0x0040, RW, gen_int_cntl_fields) \
  \
  /* Overlay & Video Registers */ \
  X(ov0_scale_cntl,           OV0_SCALE_CNTL,          0x0420, RW, NULL) \
  \
  /* Multimedia Port Processor Registers */ \
  X(mpp_tb_config,            MPP_TB_CONFIG,           0x01c0, RW, NULL) \
  X(mpp_gp_config,            MPP_GP_CONFIG,           0x01c8, RW, NULL) \
  \
  /* MPEG/DVD Registers */ \
  X(subpic_cntl,              SUBPIC_CNTL,             0x0540, RW, NULL) \
  \
  /* VIP & I2C Registers */ \
  X(viph_control,             VIPH_CONTROL,            0x01d0, RW, NULL) \
  X(i2c_cntl_1,               I2C_CNTL_1,              0x0094, RW, NULL) \
  \
  /* Capture Registers */ \
  X(cap0_trig_cntl,           CAP0_TRIG_CNTL,          0x0950, RW, NULL) \
  X(cap1_trig_cntl,           CAP1_TRIG_CNTL,          0x09c0, RW, NULL) \
  \
  /* Status Registers */ \
  X(gui_stat,                 GUI_STAT,                0x1740, RO, gui_stat_fields) \
  \
  /* Host Data Registers */ \
  X(host_data0,               HOST_DATA0,              0x17c0, WO, NULL) \
  X(host_data1,               HOST_DATA1,              0x17c4, WO, NULL) \
  X(host_data2,               HOST_DATA2,              0x17c8, WO, NULL) \
  X(host_data3,               HOST_DATA3,              0x17cc, WO, NULL) \
  X(host_data4,               HOST_DATA4,              0x17d0, WO, NULL) \
  X(host_data5,               HOST_DATA5,              0x17d4, WO, NULL) \
  X(host_data6,               HOST_DATA6,              0x17d8, WO, NULL) \
  X(host_data7,               HOST_DATA7,              0x17dc, WO, NULL) \
  X(host_data_last,           HOST_DATA_LAST,          0x17e0, WO, NULL) \
  \
  /* 3D Registers */ \
  X(scale_3d_cntl,            SCALE_3D_CNTL,           0x1a00, RW, NULL) \
  \
  /* PM4/CCE Registers */ \
  X(pm4_buffer_offset,        PM4_BUFFER_OFFSET,       0x0700, RW, NULL) \
  X(pm4_buffer_cntl,          PM4_BUFFER_CNTL,         0x0704, RW, pm4_buffer_cntl_fields) \
  X(pm4_buffer_wm_cntl,       PM4_BUFFER_WM_CNTL,      0x0708, RW, NULL) \
  X(pm4_buffer_dl_rptr_addr,  PM4_BUFFER_DL_RPTR_ADDR, 0x070c, RW, NULL) \
  X(pm4_buffer_dl_rptr,       PM4_BUFFER_DL_RPTR,      0x0710, RW, NULL) \
  X(pm4_buffer_dl_wptr,       PM4_BUFFER_DL_WPTR,      0x0714, RW, NULL) \
  X(pm4_buffer_dl_wptr_delay, PM4_BUFFER_DL_WPTR_DELAY,0x0718, RW, NULL) \
  X(pm4_buffer_addr,          PM4_BUFFER_ADDR,         0x07f0, RW, NULL) \
  X(pm4_micro_cntl,           PM4_MICRO_CNTL,          0x07fc, RW, pm4_micro_cntl_fields) \
  X(pm4_fifo_data_even,       PM4_FIFO_DATA_EVEN,      0x1000, WO, NULL) \
  X(pm4_fifo_data_odd,        PM4_FIFO_DATA_ODD,       0x1004, WO, NULL) \
  X(pm4_microcode_addr,       PM4_MICROCODE_ADDR,      0x07d4, RW, NULL) \
  X(pm4_microcode_raddr,      PM4_MICROCODE_RADDR,     0x07d8, RW, NULL) \
  X(pm4_microcode_datah,      PM4_MICROCODE_DATAH,     0x07dc, RW, NULL) \
  X(pm4_microcode_datal,      PM4_MICROCODE_DATAL,     0x07e0, RW, NULL) \
  X(pm4_stat,                 PM4_STAT,                0x07b8, RO, pm4_stat_fields)

// Register offset enum
#define X(func_name, const_name, offset, mode, fields) const_name = offset,
  enum {
      ATI_REGISTERS
  };
#undef X

// Read functions
#define X(func_name, const_name, offset, mode, fields) \
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
#define X(func_name, const_name, offset, mode, fields) \
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

// ============================================================================
// Field Value Enumerations
// (Register field constants are generated by DEFINE_REG_FIELDS above,
//  these are specific values that can be assigned to multi-bit fields)
// ============================================================================

// CRTC_GEN_CNTL: CRTC_PIX_WIDTH values
enum {
    CRTC_PIX_WIDTH_4BPP  = 1,
    CRTC_PIX_WIDTH_8BPP  = 2,
    CRTC_PIX_WIDTH_15BPP = 3,
    CRTC_PIX_WIDTH_16BPP = 4,
    CRTC_PIX_WIDTH_24BPP = 5,
    CRTC_PIX_WIDTH_32BPP = 6,
};

// CRTC_GEN_CNTL: CRTC_CUR_MODE values
enum {
    CRTC_CUR_MODE_64X64_MONO = 0,
    // All others reserved
};

// Brush datatype values
enum {
    BRUSH_8X8_MONO         =  0,
    BRUSH_8X8_MONO_TRANS   =  1,
    BRUSH_8X1_MONO         =  2,
    BRUSH_8X1_MONO_TRANS   =  3,
    BRUSH_1X8_MONO         =  4,
    BRUSH_1X8_MONO_TRANS   =  5,
    BRUSH_32X1_MONO        =  6,
    BRUSH_32X1_MONO_TRANS  =  7,
    BRUSH_32X32_MONO       =  8,
    BRUSH_32X32_MONO_TRANS =  9,
    BRUSH_8X8_COLOR        = 10,
    BRUSH_8X1_COLOR        = 11,
    BRUSH_1X8_COLOR        = 12,
    BRUSH_SOLIDCOLOR       = 13,
    BRUSH_NONE             = 15,
};

// Src datatype values
enum {
    SRC_MONO       = 0,
    SRC_MONO_TRANS = 1,
    SRC_DST_COLOR  = 3,
};

// Dst datatype values
enum {
    DST_PSEUDO_COLOR_8  =  2,
    DST_ARGB_1555       =  3,
    DST_RGB_565         =  4,
    DST_RGB_888         =  5,
    DST_ARGB_8888       =  6,
    DST_RGB_332         =  7,
    DST_Y8_GREYSCALE    =  8,
    DST_RGB_8_GREYSCALE =  9,
    DST_YUV_422_VYUY    = 11,
    DST_YUV_422_YVYU    = 12,
    DST_AYUV_444_8888   = 14,
    DST_ARGB_4444       = 15,
};

// clang-format on
static inline uint32_t
ati_get_dst_datatype(int bpp)
{
    switch (bpp) {
    case 8:
        return DST_PSEUDO_COLOR_8;
    case 15:
        return DST_ARGB_1555;
    case 16:
        return DST_RGB_565;
    case 24:
        return DST_RGB_888;
    case 32:
        return DST_ARGB_8888;
    default:
        return DST_ARGB_8888;
    }
}
// clang-format off

// ROP3 values
enum {
    ROP3_SRCCOPY = 0xcc,
};

// Source values
enum {
    SOURCE_MEMORY            = 2,
    SOURCE_HOST_DATA         = 3,
    SOURCE_HOST_DATA_ALIGNED = 4,
};

// PM4_BUFFER_CNTL: PM4_BUFFER_MODE values
enum {
    PM4_NONPM4                 = 0, // CCE disabled, direct PIO
    PM4_192PIO                 = 1, // 192 DWORD CCE PIO FIFO
    PM4_192BM                  = 2, // 192 DWORD bus master
    PM4_128PIO_64INDBM         = 3,
    PM4_128BM_64INDBM          = 4,
    PM4_64PIO_128INDBM         = 5,
    PM4_64BM_128INDBM          = 6,
    PM4_64PIO_64VCBM_64INDBM   = 7,
    PM4_64BM_64VCBM_64INDBM    = 8,
    PM4_64PIO_64VCPIO_64INDPIO = 15,
};

// clang-format on
#endif
