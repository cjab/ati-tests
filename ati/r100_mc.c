#include "r100_mc.h"

volatile uint32_t gart_mem[1024] __attribute__((aligned(4096)));
uint32_t page_table[32] __attribute__((aligned(4096)));

uint32_t
ati_r100_init_pci_gart(ati_device_t *dev)
{
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

    return gart_vm_start;
}


void
ati_r100_disable_pci_gart(ati_device_t *dev)
{
    wr_r100_aic_ctrl(dev, 0);
}
