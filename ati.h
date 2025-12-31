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
void ati_wait_for_reg_value(ati_device_t *dev, uint32_t reg, uint32_t value);
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
// Generated Register Definitions
// ============================================================================
// Include generated register constants, field enums, and field tables.
// Field values are PRE-SHIFTED and can be ORed directly into register values.
// Example: reg |= GMC_BRUSH_DATATYPE_SOLIDCOLOR | GMC_ROP3_SRCCOPY;
#include "ati_regs_gen.h"

// ============================================================================
// Read/Write Function Declarations
// ============================================================================
// These use the ATI_REGISTERS macro from the generated header.
// All registers get both read and write declarations - the flags just
// indicate runtime behavior, not whether the functions should exist.

// Read functions
#define X(func_name, const_name, offset, flags, fields, aliases) \
  uint32_t rd_##func_name(ati_device_t *dev);
ATI_REGISTERS
#undef X

// Write functions
#define X(func_name, const_name, offset, flags, fields, aliases) \
  void wr_##func_name(ati_device_t *dev, uint32_t val);
ATI_REGISTERS
#undef X

// ============================================================================
// Helper Functions
// ============================================================================

// Get pre-shifted GMC_DST_DATATYPE value for a given BPP.
// Returns a value ready to OR into DP_GUI_MASTER_CNTL.
// clang-format on
static inline uint32_t
ati_get_dst_datatype(int bpp)
{
    switch (bpp) {
    case 8:
        return GMC_DST_DATATYPE_PSEUDO_COLOR_8;
    case 15:
        return GMC_DST_DATATYPE_ARGB_1555;
    case 16:
        return GMC_DST_DATATYPE_RGB_565;
    case 24:
        return GMC_DST_DATATYPE_RGB_888;
    case 32:
        return GMC_DST_DATATYPE_ARGB_8888;
    default:
        return GMC_DST_DATATYPE_ARGB_8888;
    }
}
// clang-format off

#endif
