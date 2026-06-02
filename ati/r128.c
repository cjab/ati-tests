/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "r128.h"

// ============================================================================
// Display Mode Setup for Rage 128
// ============================================================================

void
r128_set_display_mode(ati_device_t *dev)
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
    wr_r128_viph_control(dev, 0x0);
    wr_i2c_cntl_1(dev, 0x0);
    wr_gen_int_cntl(dev, 0x0);
    wr_cap0_trig_cntl(dev, 0x0);
    wr_cap1_trig_cntl(dev, 0x0);

    uint32_t dac_cntl = rd_dac_cntl(dev);
    wr_dac_cntl(dev,
                (dac_cntl & ~DAC_TVO_EN & ~DAC_VGA_ADR_EN & ~DAC_MASK_MASK) |
                    DAC_8BIT_EN | (0xff << DAC_MASK_SHIFT));

    // Enable acceleration, ensure interlace/double-scan are disabled
    uint32_t crtc_gen_cntl = rd_r128_crtc_gen_cntl(dev);
    wr_r128_crtc_gen_cntl(dev, (crtc_gen_cntl & ~R128_CRTC_PIX_WIDTH_MASK & ~R128_CRTC_CUR_EN &
                           ~R128_CRTC_C_SYNC_EN & ~R128_CRTC_DBL_SCAN_EN & ~R128_CRTC_INTERLACE_EN) |
                              R128_CRTC_EXT_DISP_EN | R128_CRTC_EN |
                              R128_CRTC_PIX_WIDTH_32BPP);

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

    // Vertical sync: starts at 480 + 8 = 490, width = 2 lines
    uint32_t v_sync_strt = 480 + 8; // 488
    uint32_t v_sync_wid = 2;         // 2 lines
    // Negative sync polarity for 640x480@60Hz
    wr_crtc_v_sync_strt_wid(dev, (v_sync_strt << CRTC_V_SYNC_STRT_SHIFT) |
                                     (v_sync_wid << CRTC_V_SYNC_WID_SHIFT) |
                                     CRTC_V_SYNC_POL);

    wr_crtc_offset(dev, 0x0);
    wr_crtc_offset_cntl(dev, 0x0);

    // R128: CRTC_PITCH is bits 9:0, pitch in (pixels * 8)
    uint32_t crtc_pitch = X_RES / 8;
    wr_r128_crtc_pitch(dev, crtc_pitch);

    // R128: DDA-based display FIFO arbitration
    // These values are calculated for:
    //   XCLK = 134 MHz (memory clock from BIOS)
    //   VCLK = 67 MHz (BIOS-configured pixel clock, not 25MHz)
    //   32bpp, 640x480, FIFO depth=32, FIFO width=128 bits
    // Using Linux's known-working values for now:
    wr_r128_dda_config(dev, 0x01060220);
    wr_r128_dda_on_off(dev, 0x05e03b80);

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
ati_r128_wait_for_fifo(ati_device_t *dev, uint32_t entries)
{
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t slots = rd_r128_gui_stat(dev) & R128_GUI_FIFO_CNT_MASK;
        if (slots >= entries) {
            return;
        }
    }
    printf("ati_wait_for_fifo timed out! (waiting for %d entries)\n", entries);
    // TODO: I'm not sure what should happen on a timeout here.
    //       It looks like the r128 driver resets the engine.
    //       We'll see if we can get away with not worrying about it...
}


void
ati_r128_wait_for_engine(ati_device_t *dev)
{
    // Wait for engine to be idle
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t status = rd_r128_gui_stat(dev);
        if ((status & R128_GUI_ACTIVE) == 0) {
            break;
        }
    }
    if (timeout == 0) {
        printf("ati_wait_for_idle timed out! GUI still active.\n");
    }
}


void
ati_r128_engine_flush(ati_device_t *dev)
{
    // Flush the pixel cache
    wr_r128_pc_ngui_ctlstat(dev, R128_PC_FLUSH_ALL_MASK);

    // Wait for flush to complete (PC_BUSY bit to clear)
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t status = rd_r128_pc_ngui_ctlstat(dev);
        if ((status & R128_PC_BUSY) == 0) {
            return;
        }
    }
    printf("ati_engine_flush timed out! Pixel cache still busy.\n");
}

void
ati_r128_engine_reset(ati_device_t *dev)
{
    // Flush pixel cache first
    ati_r128_engine_flush(dev);

    // Read current GEN_RESET_CNTL value
    uint32_t gen_reset_cntl = rd_r128_gen_reset_cntl(dev);

    // Assert soft reset
    wr_r128_gen_reset_cntl(dev, gen_reset_cntl | R128_SOFT_RESET_GUI);
    // Read back to ensure write completes
    rd_r128_gen_reset_cntl(dev);

    // Deassert soft reset
    wr_r128_gen_reset_cntl(dev, gen_reset_cntl & ~R128_SOFT_RESET_GUI);
    // Read back to ensure write completes
    rd_r128_gen_reset_cntl(dev);

    // Restore original value
    wr_r128_gen_reset_cntl(dev, gen_reset_cntl);
}

void
ati_r128_init_gui_engine(ati_device_t *dev)
{
    // Disable 3D scaling
    wr_r128_scale_3d_cntl(dev, 0x0);

    // Reset the engine
    ati_engine_reset(dev);

    // Wait for engine to be idle after reset
    ati_wait_for_idle(dev);

    // Set default offset and pitch - chip-specific register layouts
    //ati_chip_family_t chip = ati_get_chip_family(dev);
    //if (chip == CHIP_R100) {
    //    // R100: Combined register with pitch in bits 29:22, offset in bits 21:0
    //    // Pitch is in 64-byte units: (640 * 4) / 64 = 40
    //    // Offset is in 1KB units: 0
    //    uint32_t pitch_64 = (X_RES * BYPP) / 64;
    //    wr_r100_default_pitch_offset(dev, pitch_64 << 22);
    //} else {
    //}
    wr_r128_default_offset(dev, 0x0);
    wr_r128_default_pitch(dev, X_RES / 8);

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
    wr_dp_gui_master_cntl(
        dev, GMC_BRUSH_DATATYPE_SOLIDCOLOR |
                 ati_get_dst_datatype(BPP) |
                 GMC_SRC_DATATYPE_DST_COLOR |
                 GMC_BYTE_PIX_ORDER | // LSB to MSB
                 GMC_ROP3_SRCCOPY |
                 GMC_SRC_SOURCE_MEMORY |
                 GMC_CLR_CMP_CNTL_DIS | GMC_AUX_CLIP_DIS | GMC_WR_MSK_DIS);

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
ati_r128_get_bytes_per_pixel(ati_device_t *dev)
{
    uint32_t crtc_gen_cntl = rd_r128_crtc_gen_cntl(dev);
    uint32_t pix_width = crtc_gen_cntl & R128_CRTC_PIX_WIDTH_MASK;

    switch (pix_width) {
    case R128_CRTC_PIX_WIDTH_8BPP:
        return 1;
    case R128_CRTC_PIX_WIDTH_15BPP:
    case R128_CRTC_PIX_WIDTH_16BPP:
        return 2;
    case R128_CRTC_PIX_WIDTH_24BPP:
        return 3;
    case R128_CRTC_PIX_WIDTH_32BPP:
        return 4;
    default:
        return 1;
    }
}
