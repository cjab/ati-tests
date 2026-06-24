/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "r100.h"
#include "registers/r100_regs_gen.h"

// ============================================================================
// Display Mode Setup for Radeon R100
// ============================================================================

void
r100_set_display_mode(ati_device_t *dev)
{
    // Disable display while programming CRTC registers
    uint32_t crtc_ext_cntl = rd_crtc_ext_cntl(dev);
    wr_crtc_ext_cntl(dev, crtc_ext_cntl | CRTC_HSYNC_DIS | CRTC_VSYNC_DIS |
                              CRTC_DISPLAY_DIS | VGA_ATI_LINEAR |
                              VGA_XCRT_CNT_EN | CRTC_CRT_ON);

    // Clear common registers that could interfere with CRTC settings
    wr_ovr_clr(dev, 0x0);
    wr_ovr_wid_left_right(dev, 0x0);
    wr_ovr_wid_top_bottom(dev, 0x0);
    wr_ov0_scale_cntl(dev, 0x0);
    wr_mpp_tb_config(dev, 0x0);
    wr_mpp_gp_config(dev, 0x0);
    wr_subpic_cntl(dev, 0x0);
    wr_r100_viph_control(dev, 0x0);
    wr_i2c_cntl_1(dev, 0x0);
    wr_gen_int_cntl(dev, 0x0);
    wr_cap0_trig_cntl(dev, 0x0);
    wr_cap1_trig_cntl(dev, 0x0);

    uint32_t dac_cntl = rd_dac_cntl(dev);
    wr_dac_cntl(dev,
                (dac_cntl & ~DAC_TVO_EN & ~DAC_VGA_ADR_EN & ~DAC_MASK_MASK) |
                    DAC_8BIT_EN | (0xff << DAC_MASK_SHIFT));

    // Enable acceleration, ensure interlace/double-scan are disabled
    uint32_t crtc_gen_cntl = rd_r100_crtc_gen_cntl(dev);
    wr_r100_crtc_gen_cntl(dev, (crtc_gen_cntl & ~R100_CRTC_PIX_WIDTH_MASK & ~R100_CRTC_CUR_EN &
                               ~R100_CRTC_C_SYNC_EN & ~R100_CRTC_DBL_SCAN_EN & ~R100_CRTC_INTERLACE_EN) |
                               R100_CRTC_EXT_DISP_EN | R100_CRTC_EN |
                               R100_CRTC_PIX_WIDTH_32BPP);

    // VESA 640x480@60Hz standard timings
    // Horizontal: 640 visible + 16 front porch + 96 sync + 48 back porch = 800
    // total Vertical: 480 visible + 10 front porch + 2 sync + 33 back porch =
    // 525 total Pixel clock: 25.175 MHz

    // Horizontal timing (in 8-pixel characters)
    uint32_t h_disp = ((640 / 8) - 1);  // 79 (640 pixels / 8 - 1)
    uint32_t h_total = ((800 / 8) - 1); // 99 (800 pixels / 8 - 1)
    wr_crtc_h_total_disp(dev, (h_disp << CRTC_H_DISP_SHIFT) |
                                  (h_total << CRTC_H_TOTAL_SHIFT));

    // Horizontal sync: starts at 640 + 16 = 656, width = 96 pixels
    uint32_t h_sync_strt = (640 + 16) / 8; // 82 characters
    uint32_t h_sync_wid = 96 / 8;          // 12 characters
    // Negative sync polarity for 640x480@60Hz
    wr_crtc_h_sync_strt_wid(dev, (h_sync_strt << CRTC_H_SYNC_STRT_CHAR_SHIFT) |
                                     (h_sync_wid << CRTC_H_SYNC_WID_SHIFT) |
                                     CRTC_H_SYNC_POL);

    // Vertical timing (in lines)
    uint32_t v_disp = 480 - 1;  // 479
    uint32_t v_total = 525 - 1; // 524
    wr_crtc_v_total_disp(dev, (v_disp << CRTC_V_DISP_SHIFT) |
                                  (v_total << CRTC_V_TOTAL_SHIFT));

    // Vertical sync: starts at 480 + 10 = 490, width = 2 lines
    uint32_t v_sync_strt = 480 + 10; // 490
    uint32_t v_sync_wid = 2;         // 2 lines
    // Negative sync polarity for 640x480@60Hz
    wr_crtc_v_sync_strt_wid(dev, (v_sync_strt << CRTC_V_SYNC_STRT_SHIFT) |
                                     (v_sync_wid << CRTC_V_SYNC_WID_SHIFT) |
                                     CRTC_V_SYNC_POL);

    wr_crtc_offset(dev, 0x0);
    wr_crtc_offset_cntl(dev, 0x0);

    // R100 has CRTC_PITCH (bits 10:0) and CRTC_PITCH_RIGHT (bits 26:16)
    // Linux driver sets both to the same value even in non-stereo mode
    uint32_t crtc_pitch = X_RES / 8;
    crtc_pitch |= (crtc_pitch << 16);

    // R100: Display address = CRTC_OFFSET + DISPLAY_BASE_ADDR
    // DISPLAY_BASE_ADDR must match MC_FB_LOCATION.MC_FB_START << 16
    // so that CRTC reads from the same place CPU writes to via BAR0
    uint32_t mc_fb_loc = rd_r100_mc_fb_location(dev);
    uint32_t fb_start = (mc_fb_loc & 0xFFFF) << 16;
    wr_r100_display_base_addr(dev, fb_start);

    wr_r100_crtc_pitch(dev, crtc_pitch);

    // R100: GRPH_BUFFER_CNTL-based display FIFO
    // Value from Linux radeon driver (radeonfb)
    wr_r100_grph_buffer_cntl(dev, 0x20117c7c);

    // Initialize linear palette for 32bpp gamma correction
    // In 32bpp mode, each color component (R,G,B) is looked up through
    // the palette. Entry N should map to (N,N,N) for correct colors.
    wr_palette_index(dev, 0); // Start at entry 0
    for (int i = 0; i < 256; i++) {
        uint32_t val = (i << 16) | (i << 8) | i; // R=G=B=i
        wr_palette_data(dev, val);
    }

    // Re-enable the display
    crtc_ext_cntl = rd_crtc_ext_cntl(dev);
    wr_crtc_ext_cntl(dev, (crtc_ext_cntl & ~CRTC_HSYNC_DIS & ~CRTC_VSYNC_DIS &
                           ~CRTC_DISPLAY_DIS) |
                              VGA_ATI_LINEAR | VGA_XCRT_CNT_EN | CRTC_CRT_ON);
}

void
ati_r100_init_gui_engine(ati_device_t *dev)
{
    // Reset the engine
    ati_engine_reset(dev);

    // Wait for engine to be idle after reset
    ati_wait_for_idle(dev);

    // R100: Combined register with pitch in bits 29:22, offset in bits 21:0
    // Pitch is in 64-byte units: (640 * 4) / 64 = 40
    // Offset is in 1KB units: 0
    uint32_t pitch_64 = (X_RES * BYPP) / 64;
    wr_r100_default_pitch_offset(dev, pitch_64 << 22);

    // Disable auxiliary scissor
    wr_aux_sc_cntl(dev, 0x0);

    // Set scissor clipping to the max.
    // Range is -8192 to +8191 which is why these aren't just set to the mask.
    wr_default_sc_bottom_right(dev, (0x1fff << DEFAULT_SC_RIGHT_SHIFT) |
                                        (0x1fff << DEFAULT_SC_BOTTOM_SHIFT));
    // The docs say to set DEFAULT_SC_TOP_LEFT... That doesn't exist as far as I
    // can tell. So, set the actual scissor top left just to be safe.
    wr_sc_top_left(dev, 0x00000000);
    wr_sc_bottom_right(dev, (0x1fff << DEFAULT_SC_RIGHT_SHIFT) |
                                (0x1fff << DEFAULT_SC_BOTTOM_SHIFT));

    // Set blit direction to left-to-right, top-to-bottom
    wr_dp_cntl(dev, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);

    // Set GUI master control
    wr_r100_dp_gui_master_cntl(
        dev, R100_GMC_BRUSH_DATATYPE_SOLIDCOLOR |
                 ati_get_dst_datatype(BPP) |
                 R100_GMC_SRC_DATATYPE_DST_COLOR |
                 R100_GMC_BYTE_PIX_ORDER |
                 R100_GMC_ROP3_SRCCOPY |
                 R100_GMC_SRC_SOURCE_MEMORY |
                 R100_GMC_WR_MSK_DIS);

    // Clear the line drawing registers
    wr_dst_bres_err(dev, 0);
    wr_dst_bres_inc(dev, 0);
    wr_dst_bres_dec(dev, 0);

    // Set brush colors
    wr_dp_brush_frgd_clr(dev, 0xffffffff);
    wr_dp_brush_bkgd_clr(dev, 0x00000000);

    // Set source colors
    wr_dp_src_frgd_clr(dev, 0xffffffff);
    wr_dp_src_bkgd_clr(dev, 0x00000000);

    // Set write mask
    wr_dp_write_msk(dev, 0xffffffff);

    // Wait for idle to ensure initialization is complete
    ati_wait_for_idle(dev);
}

uint32_t
ati_r100_get_bytes_per_pixel(ati_device_t *dev)
{
    uint32_t crtc_gen_cntl = rd_r100_crtc_gen_cntl(dev);
    uint32_t pix_width = crtc_gen_cntl & R100_CRTC_PIX_WIDTH_MASK;

    switch (pix_width) {
    case R100_CRTC_PIX_WIDTH_8BPP:
        return 1;
    case R100_CRTC_PIX_WIDTH_15BPP:
    case R100_CRTC_PIX_WIDTH_16BPP_RGB:
    case R100_CRTC_PIX_WIDTH_16BPP_ARGB:
    case R100_CRTC_PIX_WIDTH_16BPP_AIDX:
        return 2;
    case R100_CRTC_PIX_WIDTH_24BPP:
        return 3;
    case R100_CRTC_PIX_WIDTH_32BPP:
        return 4;
    default:
        return 1;
    }
}
