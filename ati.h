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

// ============================================================================
// Register Definitions
// ============================================================================
#define ATI_REGISTERS \
  /* Datapath / Drawing Engine Registers */ \
  X(dp_gui_master_cntl,       DP_GUI_MASTER_CNTL,      0x146c, RW, NULL) \
  X(dp_datatype,              DP_DATATYPE,             0x16c4, RW, NULL) \
  X(dp_mix,                   DP_MIX,                  0x16c8, RW, NULL) \
  X(dp_write_msk,             DP_WRITE_MSK,            0x16cc, RW, NULL) \
  X(dp_cntl,                  DP_CNTL,                 0x16c0, RW, NULL) \
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
  X(default_sc_bottom_right,  DEFAULT_SC_BOTTOM_RIGHT, 0x16e8, RW, NULL) \
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
  X(default_offset,           DEFAULT_OFFSET,          0x16e0, RW, NULL) \
  X(default_pitch,            DEFAULT_PITCH,           0x16e4, RW, NULL) \
  \
  /* CRTC Registers */ \
  X(crtc_h_total_disp,        CRTC_H_TOTAL_DISP,       0x0200, RW, NULL) \
  X(crtc_h_sync_strt_wid,     CRTC_H_SYNC_STRT_WID,    0x0204, RW, NULL) \
  X(crtc_v_total_disp,        CRTC_V_TOTAL_DISP,       0x0208, RW, NULL) \
  X(crtc_v_sync_strt_wid,     CRTC_V_SYNC_STRT_WID,    0x020c, RW, NULL) \
  X(crtc_gen_cntl,            CRTC_GEN_CNTL,           0x0050, RW, NULL) \
  X(crtc_ext_cntl,            CRTC_EXT_CNTL,           0x0054, RW, NULL) \
  X(crtc_offset,              CRTC_OFFSET,             0x0224, RW, NULL) \
  X(crtc_offset_cntl,         CRTC_OFFSET_CNTL,        0x0228, RW, NULL) \
  X(crtc_pitch,               CRTC_PITCH,              0x022c, RW, NULL) \
  \
  /* Reset & Engine Control Registers */ \
  X(gen_reset_cntl,           GEN_RESET_CNTL,          0x00f0, RW, NULL) \
  X(pc_ngui_ctlstat,          PC_NGUI_CTLSTAT,         0x0184, RW, NULL) \
  \
  /* DAC Registers */ \
  X(dac_cntl,                 DAC_CNTL,                0x0058, RW, NULL) \
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
  X(gen_int_cntl,             GEN_INT_CNTL,            0x0040, RW, NULL) \
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
  X(pm4_buffer_cntl,          PM4_BUFFER_CNTL,         0x0704, RW, NULL) \
  X(pm4_buffer_wm_cntl,       PM4_BUFFER_WM_CNTL,      0x0708, RW, NULL) \
  X(pm4_buffer_dl_rptr_addr,  PM4_BUFFER_DL_RPTR_ADDR, 0x070c, RW, NULL) \
  X(pm4_buffer_dl_rptr,       PM4_BUFFER_DL_RPTR,      0x0710, RW, NULL) \
  X(pm4_buffer_dl_wptr,       PM4_BUFFER_DL_WPTR,      0x0714, RW, NULL) \
  X(pm4_buffer_dl_wptr_delay, PM4_BUFFER_DL_WPTR_DELAY,0x0718, RW, NULL) \
  X(pm4_buffer_addr,          PM4_BUFFER_ADDR,         0x07f0, RW, NULL) \
  X(pm4_micro_cntl,           PM4_MICRO_CNTL,          0x07fc, RW, NULL) \
  X(pm4_fifo_data_even,       PM4_FIFO_DATA_EVEN,      0x1000, WO, NULL) \
  X(pm4_fifo_data_odd,        PM4_FIFO_DATA_ODD,       0x1004, WO, NULL) \
  X(pm4_microcode_addr,       PM4_MICROCODE_ADDR,      0x07d4, RW, NULL) \
  X(pm4_microcode_raddr,      PM4_MICROCODE_RADDR,     0x07d8, RW, NULL) \
  X(pm4_microcode_datah,      PM4_MICROCODE_DATAH,     0x07dc, RW, NULL) \
  X(pm4_microcode_datal,      PM4_MICROCODE_DATAL,     0x07e0, RW, NULL) \
  X(pm4_stat,                 PM4_STAT,                0x07b8, RO, NULL)

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
// Register Fields
// ============================================================================

// ----------------------------------------------------------------------------
// CRTC_GEN_CNTL
// ----------------------------------------------------------------------------
enum {
    CRTC_DBL_SCAN_EN      = (1 <<  0),
    CRTC_INTERLACE_EN     = (1 <<  1),
    CRTC_C_SYNC_EN        = (1 <<  4),
    CRTC_PIX_WIDTH_SHIFT  = 8,
    CRTC_PIX_WIDTH_MASK   = (7 <<  8),
    CRTC_CUR_EN           = (1 << 16),
    CRTC_CUR_MODE_SHIFT   = 17,
    CRTC_CUR_MODE_MASK    = (7 << 17),
    CRTC_EXT_DISP_EN      = (1 << 24),
    CRTC_EN               = (1 << 25),
    CRTC_DISP_REQ_EN_B    = (1 << 26),
};

// Pix width values
enum {
    CRTC_PIX_WIDTH_4BPP  = 1,
    CRTC_PIX_WIDTH_8BPP  = 2,
    CRTC_PIX_WIDTH_15BPP = 3,
    CRTC_PIX_WIDTH_16BPP = 4,
    CRTC_PIX_WIDTH_24BPP = 5,
    CRTC_PIX_WIDTH_32BPP = 6,
};

// Cursor mode values
enum {
    CRTC_CUR_MODE_64X64_MONO = 0,
    // All others reserved, the register guide unfortunately seems to have
    // an error here that confusingly lists unrelated (I think) values.
};


// ----------------------------------------------------------------------------
// CRTC_EXT_CNTL
// ----------------------------------------------------------------------------
enum {
    CRTC_VGA_XOVERSCAN     = (1 << 0),
    VGA_BLINK_RATE_SHIFT   = 1,
    VGA_BLINK_RATE_MASK    = (3 <<  1),
    VGA_ATI_LINEAR         = (1 <<  3),
    VGA_128KAP_PAGING      = (1 <<  4),
    VGA_TEXT_132           = (1 <<  5),
    VGA_XCRT_CNT_EN        = (1 <<  6),
    CRTC_HSYNC_DIS         = (1 <<  8),
    CRTC_VSYNC_DIS         = (1 <<  9),
    CRTC_DISPLAY_DIS       = (1 << 10),
    CRTC_SYNC_TRISTATE     = (1 << 11),
    CRTC_HSYNC_TRISTATE    = (1 << 12),
    CRTC_VSYNC_TRISTATE    = (1 << 13),
    CRTC_CRT_ON            = (1 << 15), // Undocumented but used in r128 xorg driver
    VGA_CUR_B_TEST         = (1 << 17),
    VGA_PACK_DIS           = (1 << 18),
    VGA_MEM_PS_EN          = (1 << 19),
    VGA_READ_PREFETCH_DIS  = (1 << 20),
    DFIFO_EXTSENSE         = (1 << 21),
    FP_OUT_EN              = (1 << 22),
    FP_ACTIVE              = (1 << 23),
    VCRTC_IDX_MASTER_SHIFT = 24,
    VCRTC_IDX_MASTER_MASK  = (0x7f << 24),
};

// ----------------------------------------------------------------------------
// DAC_CNTL
// ----------------------------------------------------------------------------
enum {
    DAC_RANGE_CNTL_SHIFT   = 0,
    DAC_RANGE_CNTL_MASK    = (3 <<  0),
    DAC_BLANKING           = (1 <<  2),
    DAC_CMP_EN             = (1 <<  3),
    DAC_CMP_OUTPUT         = (1 <<  7),
    DAC_8BIT_EN            = (1 <<  8),
    DAC_4BPP_PIX_ORDER     = (1 <<  9),
    DAC_TVO_EN             = (1 << 10),
    DAC_TVO_OVR_EXCL       = (1 << 11),
    DAC_TVO_16BPP_DITH_EN  = (1 << 12),
    DAC_VGA_ADR_EN         = (1 << 13),
    DAC_PDWN               = (1 << 15),
    DAC_CRC_EN             = (1 << 19),
    DAC_MASK_SHIFT         = 24,
    DAC_MASK_MASK          = (0xff << 24),
};

// ----------------------------------------------------------------------------
// GEN_RESET_CNTL
// ----------------------------------------------------------------------------
enum {
    SOFT_RESET_GUI = (1 << 0),
};

// ----------------------------------------------------------------------------
// PC_NGUI_CTLSTAT
// ----------------------------------------------------------------------------
enum {
    PC_FLUSH_ALL = 0x00ff,
    PC_BUSY      = (1u << 31),
};

// ----------------------------------------------------------------------------
// DP_DATATYPE
// ----------------------------------------------------------------------------
enum {
    HOST_BIG_ENDIAN_EN = (1 << 29),
};

// ----------------------------------------------------------------------------
// DP_CNTL
// ----------------------------------------------------------------------------
enum {
    DST_X_LEFT_TO_RIGHT = (1 << 0),
    DST_Y_TOP_TO_BOTTOM = (1 << 1),
    DST_Y_MAJOR         = (1 << 2),
};

// ----------------------------------------------------------------------------
// CRTC_H_TOTAL_DISP
// ----------------------------------------------------------------------------
enum {
    CRTC_H_TOTAL_SHIFT = 0,
    CRTC_H_TOTAL_MASK  = (0x1ff <<  0),
    CRTC_H_DISP_SHIFT  = 16,
    CRTC_H_DISP_MASK   = (0xff  << 16),
};

// ----------------------------------------------------------------------------
// CRTC_H_SYNC_STRT_WID
// ----------------------------------------------------------------------------
enum {
    CRTC_H_SYNC_STRT_PIX_SHIFT  = 0,
    CRTC_H_SYNC_STRT_PIX_MASK   = (    7 <<  0),
    CRTC_H_SYNC_STRT_CHAR_SHIFT = 3,
    CRTC_H_SYNC_STRT_CHAR_MASK  = (0x1ff <<  3),
    CRTC_H_SYNC_WID_SHIFT       = 16,
    CRTC_H_SYNC_WID_MASK        = ( 0x3f << 16),
    CRTC_H_SYNC_POL             = (    1 << 23)
};

// ----------------------------------------------------------------------------
// CRTC_V_TOTAL_DISP
// ----------------------------------------------------------------------------
enum {
    CRTC_V_TOTAL_SHIFT = 0,
    CRTC_V_TOTAL_MASK  = (0x7ff <<  0),
    CRTC_V_DISP_SHIFT  = 16,
    CRTC_V_DISP_MASK   = (0x7ff  << 16),
};

// ----------------------------------------------------------------------------
// CRTC_V_SYNC_STRT_WID
// ----------------------------------------------------------------------------
enum {
    CRTC_V_SYNC_STRT_SHIFT = 0,
    CRTC_V_SYNC_STRT_MASK  = (0x7ff <<  0),
    CRTC_V_SYNC_WID_SHIFT  = 16,
    CRTC_V_SYNC_WID_MASK   = ( 0x1f << 16),
    CRTC_V_SYNC_POL        = (    1 << 23),
};

// ----------------------------------------------------------------------------
// DEFAULT_OFFSET
// ----------------------------------------------------------------------------
enum {
    DEFAULT_OFFSET_SHIFT = 0,
    DEFAULT_OFFSET_MASK  = (0x3ffffff << 0),
};

// ----------------------------------------------------------------------------
// DEFAULT_PITCH
// ----------------------------------------------------------------------------
enum {
    DEFAULT_PITCH_SHIFT = 0,
    DEFAULT_PITCH_MASK  = (0x3ff <<  0),
    DEFAULT_TILE_SHIFT  = 16,
    DEFAULT_TILE_MASK   = (    1 << 16),
};

// ----------------------------------------------------------------------------
// DEFAULT_SC_BOTTOM_RIGHT
// ----------------------------------------------------------------------------
enum {
    DEFAULT_SC_RIGHT_SHIFT  = 0,
    DEFAULT_SC_RIGHT_MASK   = (0x3fff << 0),
    DEFAULT_SC_BOTTOM_SHIFT = 16,
    DEFAULT_SC_BOTTOM_MASK  = (0x3fff << 16),
};

// ----------------------------------------------------------------------------
// DEFAULT_SC_TOP_LEFT
// ----------------------------------------------------------------------------
enum {
    DEFAULT_SC_LEFT_SHIFT  = 0,
    DEFAULT_SC_LEFT_MASK   = (0x3fff << 0),
    DEFAULT_SC_TOP_SHIFT   = 16,
    DEFAULT_SC_TOP_MASK    = (0x3fff << 16),
};

// ----------------------------------------------------------------------------
// DP_GUI_MASTER_CNTL
// ----------------------------------------------------------------------------
enum {
    GMC_SRC_PITCH_OFFSET_CNTL = (   1 <<  0),
    GMC_DST_PITCH_OFFSET_CNTL = (   1 <<  1),
    GMC_SRC_CLIPPING          = (   1 <<  2),
    GMC_DST_CLIPPING          = (   1 <<  3),
    GMC_BRUSH_DATATYPE_SHIFT  = 4,
    GMC_BRUSH_DATATYPE_MASK   = ( 0xf <<  4),
    GMC_DST_DATATYPE_SHIFT    = 8,
    GMC_DST_DATATYPE_MASK     = ( 0xf <<  8),
    GMC_SRC_DATATYPE_SHIFT    = 12,
    GMC_SRC_DATATYPE_MASK     = ( 0x3 << 12),
    GMC_BYTE_PIX_ORDER        = (   1 << 14),
    GMC_CONVERSION_TEMP       = (   1 << 15),
    GMC_ROP3_SHIFT            = 16,
    GMC_ROP3_MASK             = (0xff << 16),
    GMC_SRC_SOURCE_SHIFT      = 24,
    GMC_SRC_SOURCE_MASK       = ( 0x7 << 24),
    GMC_3D_FCN_EN             = (   1 << 27),
    GMC_CLR_CMP_CNTL_DIS      = (   1 << 28),
    GMC_AUX_CLIP_DIS          = (   1 << 29),
    GMC_WR_MSK_DIS            = (   1 << 30),
    GMC_LD_BRUSH_Y_X          = (  1u << 31),
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

// ----------------------------------------------------------------------------
// PM4_STAT
// ----------------------------------------------------------------------------
enum {
    PM4_FIFOCNT_SHIFT = 0,
    PM4_FIFOCNT_MASK  = (0x0fff <<  0),
    // PM4_BUSY = Also defined by GUI_STAT (bit 16)
    // GUI_ACTIVE = Also defined by GUI_STAT (bit 31)
};

// ----------------------------------------------------------------------------
// PM4_MICRO_CNTL
// ----------------------------------------------------------------------------
enum {
    PM4_MICRO_FREERUN = (1 << 30),
};

// ----------------------------------------------------------------------------
// PM4_BUFFER_CNTL
// ----------------------------------------------------------------------------
enum {
    PM4_BUFFER_CNTL_NOUPDATE       = (1   << 27),
    PM4_BUFFER_MODE_SHIFT          = 28,
    PM4_BUFFER_MODE_MASK           = (0xf << 28),
};

// PM4_BUFFER_CNTL mode values
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

// ----------------------------------------------------------------------------
// DST_BRES_ERR
// ----------------------------------------------------------------------------
enum {
    DST_BRES_ERR_SHIFT = 0,
    DST_BRES_ERR_MASK  = (0xfffff << 0),
};

// ----------------------------------------------------------------------------
// DST_BRES_INC
// ----------------------------------------------------------------------------
enum {
    DST_BRES_INC_SHIFT = 0,
    DST_BRES_INC_MASK  = (0xfffff << 0),
};

// ----------------------------------------------------------------------------
// DST_BRES_DEC
// ----------------------------------------------------------------------------
enum {
    DST_BRES_DEC_SHIFT = 0,
    DST_BRES_DEC_MASK  = (0xfffff << 0),
};

// ----------------------------------------------------------------------------
// OVR_CLR
// ----------------------------------------------------------------------------
enum {
    OVR_CLR_B_SHIFT = 0,
    OVR_CLR_B_MASK  = (0xff <<  0),
    OVR_CLR_G_SHIFT = 8,
    OVR_CLR_G_MASK  = (0xff <<  8),
    OVR_CLR_R_SHIFT = 16,
    OVR_CLR_R_MASK  = (0xff << 16),
};

// ----------------------------------------------------------------------------
// OVR_WID_LEFT_RIGHT
// ----------------------------------------------------------------------------
enum {
    OVR_WID_RIGHT_SHIFT = 0,
    OVR_WID_RIGHT_MASK  = (0x3f <<  0),
    OVR_WID_LEFT_SHIFT  = 16,
    OVR_WID_LEFT_MASK   = (0x3f << 16),
};

// ----------------------------------------------------------------------------
// OVR_WID_TOP_BOTTOM
// ----------------------------------------------------------------------------
enum {
    OVR_WID_BOTTOM_SHIFT = 0,
    OVR_WID_BOTTOM_MASK  = (0x1ff <<  0),
    OVR_WID_TOP_SHIFT    = 16,
    OVR_WID_TOP_MASK     = (0x1ff << 16),
};

// ----------------------------------------------------------------------------
// GEN_INT_CNTL
// ----------------------------------------------------------------------------
enum {
    CRTC_VBLANK_INT_EN    = (1 <<  0),
    CRTC_VLINE_INT_EN     = (1 <<  1),
    CRTC_VSYNC_INT_EN     = (1 <<  2),
    SNAPSHOT_INT_EN       = (1 <<  3),
    FP_DETECT_INT_EN      = (1 << 10),
    BUSMASTER_EOL_INT_EN  = (1 << 16),
    I2C_INT_EN            = (1 << 17),
    MPP_GP_INT_EN         = (1 << 18),
    GUI_IDLE_INT_EN       = (1 << 19),
    VIPH_INT_EN           = (1 << 24),
};

// clang-format on
#endif
