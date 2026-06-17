/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../test.h"

static volatile uint32_t gart_mem[1024] __attribute__((aligned(4096)));
static volatile uint32_t mem[1024] __attribute__((aligned(0x08000000)));
static uint32_t page_table[32] __attribute__((aligned(4096)));

bool
test_r100_scratch_wb_to_pci_gart(ati_device_t *dev)
{
    // M6 mobility register guide state that MC_FB_LOCATION defaults to
    // 0x003f0000 but hardware testing show it defaults to 0xffff0000.
    // ASSERT_EQ(rd_r100_mc_fb_location(dev), 0xffff0000);

    // Get the framebuffer and gart locations in
    // the linear aperture address space
    uint32_t fb_location = (rd_r100_mc_fb_location(dev) & 0xffff) << 16;
    uint32_t gart_vm_start = fb_location + rd_r100_config_aper_size(dev);

    // Initialize the PCI GART page table
    memset((void *)page_table, 0, sizeof(page_table));
    page_table[0] = (uint32_t)gart_mem;
    // and GART memory
    memset((void *)gart_mem, 0, sizeof(gart_mem));

    // Shrink the framebuffer to make room for the GART
    wr_r100_mc_fb_location(dev, ((gart_vm_start - 1) & 0xffff0000) | fb_location >> 16);

    // Enable bus mastering. It should be enabled by default but...
    wr_r100_bus_cntl(dev, rd_r100_bus_cntl(dev) & ~R100_BUS_MASTER_DIS);

    // Enable PCI GART
    wr_r100_aic_ctrl(dev, R100_TRANSLATE_EN);
    wr_r100_aic_pt_base(dev, (uint32_t) page_table);
    wr_r100_aic_lo_addr(dev, gart_vm_start);
    wr_r100_aic_hi_addr(dev, gart_vm_start);

    // Not entirely sure this is necessary but the Linux DRM driver
    // does this to disable the AGP GART.
    wr_r100_mc_agp_location(dev, 0xffffffc0);
    wr_r100_agp_command(dev, 0);

    // Enable scratch writeback
    wr_r100_scratch_addr(dev, gart_vm_start);
    wr_r100_scratch_umsk(dev, R100_SCRATCH0_EN | R100_SCRATCH2_EN |
                              R100_SCRATCH5_EN);

    // Test scratch writeback
    wr_gui_scratch_reg5(dev, 0x11111111);
    wr_gui_scratch_reg4(dev, 0x22222222);
    wr_gui_scratch_reg3(dev, 0x33333333);
    wr_gui_scratch_reg2(dev, 0xcafebeef);
    wr_gui_scratch_reg1(dev, 0x1337beef);
    wr_gui_scratch_reg0(dev, 0xdeadbeef);
    for (int i = 0; i < 10000000; i++) {
        // Reg 0 _is_ enabled in umsk, it should write back
        if (gart_mem[0] == 0xdeadbeef) {
            // Reg 1 is not enabled in scratch umsk it should not
            // have been written back
            ASSERT_NEQ(gart_mem[1], 0x1337beef);
            // Reg 2 _is_ enabled
            ASSERT_EQ(gart_mem[2], 0xcafebeef);
            ASSERT_NEQ(gart_mem[3], 0x33333333);
            ASSERT_NEQ(gart_mem[4], 0x22222222);
            // Reg 5 _is_ enabled
            ASSERT_EQ(gart_mem[5], 0x11111111);
            return true;
        }
        udelay(1);
    }

    return false;
}

bool
test_r100_scratch_wb_to_fb(ati_device_t *dev)
{
    // Initialize the PCI GART page table
    //memset((void *)page_table, 0, sizeof(page_table));
    //page_table[0] = (uint32_t)gart_mem;
    uint32_t low_mem = 0x1000;
    uint32_t high_mem = 0x08000000;

    // Framebuffer covers full memory space
    wr_r100_mc_fb_location(dev, 0xffff0000);

    // Enable scratch writeback
    // Write to somewhere, anywhere really, the entire address space is framebuffer
    wr_r100_scratch_addr(dev, low_mem);
    wr_r100_scratch_umsk(dev, R100_SCRATCH0_EN | R100_SCRATCH2_EN |
                              R100_SCRATCH5_EN);

    // Test scratch writeback
    wr_gui_scratch_reg5(dev, 0x11111111);
    wr_gui_scratch_reg4(dev, 0x22222222);
    wr_gui_scratch_reg3(dev, 0x33333333);
    wr_gui_scratch_reg2(dev, 0xcafebeef);
    wr_gui_scratch_reg1(dev, 0x1337beef);
    wr_gui_scratch_reg0(dev, 0xdeadbeef);
    for (int i = 0; i < 10000000; i++) {
        // Reg 0 _is_ enabled in umsk, it should write back
        if (ati_vram_read(dev, low_mem) == 0xdeadbeef) {
            // Reg 1 is not enabled in scratch umsk it should not
            // have been written back
            ASSERT_NEQ(ati_vram_read(dev, low_mem + 1 * sizeof(uint32_t)), 0x1337beef);
            // Reg 2 _is_ enabled
            ASSERT_EQ(ati_vram_read(dev, low_mem + 2 * sizeof(uint32_t)), 0xcafebeef);
            ASSERT_NEQ(ati_vram_read(dev, low_mem + 3 * sizeof(uint32_t)), 0x33333333);
            ASSERT_NEQ(ati_vram_read(dev, low_mem + 4 * sizeof(uint32_t)), 0x22222222);
            // Reg 5 _is_ enabled
            ASSERT_EQ(ati_vram_read(dev, low_mem + 5 * sizeof(uint32_t)), 0x11111111);
            break;
        }
        udelay(1);
    }

    // Set high memory address, it should wrap around in framebuffer
    wr_r100_scratch_addr(dev, high_mem);
    uint32_t aper_size = rd_r100_config_aper_size(dev);
    uint32_t mem = high_mem % aper_size;
    // Test scratch writeback
    wr_gui_scratch_reg5(dev, 0x11111111);
    wr_gui_scratch_reg4(dev, 0x22222222);
    wr_gui_scratch_reg3(dev, 0x33333333);
    wr_gui_scratch_reg2(dev, 0xcafebeef);
    wr_gui_scratch_reg1(dev, 0x1337beef);
    wr_gui_scratch_reg0(dev, 0xdeadbeef);
    for (int i = 0; i < 10000000; i++) {
        // Reg 0 _is_ enabled in umsk, it should write back
        if (ati_vram_read(dev, mem) == 0xdeadbeef) {
            // Reg 1 is not enabled in scratch umsk it should not
            // have been written back
            ASSERT_NEQ(ati_vram_read(dev, mem + 1 * sizeof(uint32_t)), 0x1337beef);
            // Reg 2 _is_ enabled
            ASSERT_EQ(ati_vram_read(dev, mem + 2 * sizeof(uint32_t)), 0xcafebeef);
            ASSERT_NEQ(ati_vram_read(dev, mem + 3 * sizeof(uint32_t)), 0x33333333);
            ASSERT_NEQ(ati_vram_read(dev, mem + 4 * sizeof(uint32_t)), 0x22222222);
            // Reg 5 _is_ enabled
            ASSERT_EQ(ati_vram_read(dev, mem + 5 * sizeof(uint32_t)), 0x11111111);
            return true;
        }
        udelay(1);
    }

    return false;
}

bool
test_r100_scratch_wb_to_sys(ati_device_t *dev)
{
    // Framebuffer covers minimal memory space (4MB)
    // sys_addr is _far_ outside of this
    wr_r100_mc_fb_location(dev, 0x0);

    // Enable scratch writeback
    wr_r100_scratch_addr(dev, (uint32_t)mem);
    wr_r100_scratch_umsk(dev, R100_SCRATCH0_EN | R100_SCRATCH2_EN |
                              R100_SCRATCH5_EN);

    // Test scratch writeback
    wr_gui_scratch_reg5(dev, 0x11111111);
    wr_gui_scratch_reg4(dev, 0x22222222);
    wr_gui_scratch_reg3(dev, 0x33333333);
    wr_gui_scratch_reg2(dev, 0xcafebeef);
    wr_gui_scratch_reg1(dev, 0x1337beef);
    wr_gui_scratch_reg0(dev, 0xdeadbeef);
    for (int i = 0; i < 10000000; i++) {
        // Reg 0 _is_ enabled in umsk, it should write back
        if (mem[0] == 0xdeadbeef) {
            // Reg 1 is not enabled in scratch umsk it should not
            // have been written back
            ASSERT_NEQ(mem[1], 0x1337beef);
            // Reg 2 _is_ enabled
            ASSERT_EQ(mem[2], 0xcafebeef);
            ASSERT_NEQ(mem[3], 0x33333333);
            ASSERT_NEQ(mem[4], 0x22222222);
            // Reg 5 _is_ enabled
            ASSERT_EQ(mem[5], 0x11111111);
            return true;
        }
        udelay(1);
    }

    return false;
}

void
register_r100_mc_tests(void)
{
    REGISTER_TEST_FOR(test_r100_scratch_wb_to_pci_gart, "Scratch writeback to PCI GART", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_scratch_wb_to_fb, "Scratch writeback to framebuffer", CHIP_R100);
    REGISTER_TEST_FOR(test_r100_scratch_wb_to_sys, "Scratch writeback to system memory", CHIP_R100);
}
