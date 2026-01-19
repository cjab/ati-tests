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
    wr_viph_control(dev, 0x0);
    wr_i2c_cntl_1(dev, 0x0);
    wr_gen_int_cntl(dev, 0x0);
    wr_cap0_trig_cntl(dev, 0x0);
    wr_cap1_trig_cntl(dev, 0x0);

    uint32_t dac_cntl = rd_dac_cntl(dev);
    wr_dac_cntl(dev,
                (dac_cntl & ~DAC_TVO_EN & ~DAC_VGA_ADR_EN & ~DAC_MASK_MASK) |
                    DAC_8BIT_EN | (0xff << DAC_MASK_SHIFT));

    // Enable acceleration, ensure interlace/double-scan are disabled
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    wr_crtc_gen_cntl(dev, (crtc_gen_cntl & ~CRTC_PIX_WIDTH_MASK & ~CRTC_CUR_EN &
                           ~CRTC_C_SYNC_EN & ~CRTC_DBL_SCAN_EN & ~CRTC_INTERLACE_EN) |
                              CRTC_EXT_DISP_EN | CRTC_EN |
                              CRTC_PIX_WIDTH_32BPP);

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
