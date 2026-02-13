/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef ATI_H
#define ATI_H

#include <stdbool.h>
#include <stdint.h>

// IWYU pragma: begin_exports
#include "../platform/platform.h"
// IWYU pragma: end_exports

// ============================================================================
// Chip Family Detection
// ============================================================================

typedef enum {
    CHIP_UNKNOWN = 0,
    CHIP_R128    = (1 << 0),
    CHIP_R100    = (1 << 1),
} ati_chip_family_t;

#define CHIP_ALL (CHIP_R128 | CHIP_R100)

// Get string name for chip family
const char *ati_chip_family_name(ati_chip_family_t family);

// ============================================================================
// Device Constants
// ============================================================================

#define X_RES 640
#define Y_RES 480
#define BPP 32
#define BYPP (BPP / 8)
#define FIFO_MAX 64
#define VRAM_NOT_FOUND UINT64_MAX

// ============================================================================
// Device Structure
// ============================================================================

typedef struct ati_device ati_device_t;

// Get chip family for a device
ati_chip_family_t ati_get_chip_family(const ati_device_t *dev);

// ============================================================================
// Device Lifecycle
// ============================================================================

ati_device_t *ati_device_init(platform_pci_device_t *pci_dev);
void ati_device_destroy(ati_device_t *dev);

// ============================================================================
// Register and VRAM Access
// ============================================================================

uint32_t ati_reg_read(ati_device_t *dev, uint32_t offset);
void ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value);
uint32_t ati_vram_read(ati_device_t *dev, uint32_t offset);
void ati_vram_write(ati_device_t *dev, uint32_t offset, uint32_t value);
uint64_t ati_vram_search(ati_device_t *dev, uint32_t needle);
void ati_vram_clear(ati_device_t *dev);
void ati_screen_clear(ati_device_t *dev, uint32_t color);
void ati_vram_dump(ati_device_t *dev, const char *filename);
void ati_screen_dump(ati_device_t *dev, const char *filename);
void ati_vram_memcpy(ati_device_t *dev, uint32_t dst_offset, const void *src,
                     size_t size);
bool ati_screen_async_compare_fixture(ati_device_t *dev,
                                      const char *fixture_name);
bool ati_screen_compare_fixture(ati_device_t *dev, const char *fixture_name);
void ati_dump_mode(ati_device_t *dev);
void ati_print_info(ati_device_t *dev);

// ============================================================================
// Engine Control
// ============================================================================

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

// Common registers - shared between R128 and R100
#include "registers/common_regs_gen.h"

// Chip-specific registers
#include "registers/r128_regs_gen.h"
#include "registers/r100_regs_gen.h"

// ============================================================================
// Read/Write Function Declarations - Common Registers
// ============================================================================
// These use the COMMON_REGISTERS macro from common_regs_gen.h.
// Common register accessors have no prefix.

// Read functions for common registers
#define X(func_name, const_name, offset, flags, fields, aliases) \
  uint32_t rd_##func_name(ati_device_t *dev);
COMMON_REGISTERS
#undef X

// Write functions for common registers
#define X(func_name, const_name, offset, flags, fields, aliases) \
  void wr_##func_name(ati_device_t *dev, uint32_t val);
COMMON_REGISTERS
#undef X

// ============================================================================
// Read/Write Function Declarations - R128-Specific Registers
// ============================================================================
// R128-specific register accessors have r128_ prefix.
// They will assert if called on non-R128 hardware.

#ifdef R128_REGISTERS
#define X(func_name, const_name, offset, flags, fields, aliases) \
  uint32_t rd_##func_name(ati_device_t *dev);
R128_REGISTERS
#undef X

#define X(func_name, const_name, offset, flags, fields, aliases) \
  void wr_##func_name(ati_device_t *dev, uint32_t val);
R128_REGISTERS
#undef X
#endif

// ============================================================================
// Read/Write Function Declarations - R100-Specific Registers
// ============================================================================
// R100-specific register accessors have r100_ prefix.
// They will assert if called on non-R100 hardware.

#ifdef R100_REGISTERS
#define X(func_name, const_name, offset, flags, fields, aliases) \
  uint32_t rd_##func_name(ati_device_t *dev);
R100_REGISTERS
#undef X

#define X(func_name, const_name, offset, flags, fields, aliases) \
  void wr_##func_name(ati_device_t *dev, uint32_t val);
R100_REGISTERS
#undef X
#endif

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
