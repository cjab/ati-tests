#ifndef CCE_H
#define CCE_H

#include "ati.h"

// CCE packet types (bits 31:30)
enum {
    CCE_PACKET0 = 0x00000000,
    CCE_PACKET0_ONE_REG = (1 << 15),
    CCE_PACKET1 = 0x40000000,
    CCE_PACKET2 = 0x80000000,
    CCE_PACKET3 = 0xC0000000,
};

// CCE packet construction macros
#define CCE_PKT0(reg, n) (CCE_PACKET0 | ((n - 1) << 16) | ((reg) >> 2))
#define CCE_PKT0_ONE(reg, n)                                                   \
    (CCE_PACKET0 | CCE_PACKET0_ONE_REG | ((n - 1) << 16) | ((reg) >> 2))
#define CCE_PKT1(reg0, reg1)                                                   \
    (CCE_PACKET1 | (((reg1) >> 2) << 11) | ((reg0) >> 2))
#define CCE_PKT2() (CCE_PACKET2)
#define CCE_PKT3(opcode, n) (CCE_PACKET3 | (opcode) | ((n - 1) << 16))

// Type-3 packet opcodes
enum {
    CCE_CNTL_PAINT_MULTI = 0x9A00,
};

bool ati_init_cce_engine(ati_device_t *dev, uint32_t mode);
bool ati_start_cce_engine(ati_device_t *dev, uint32_t mode);
bool ati_stop_cce_engine(ati_device_t *dev);

bool ati_send_packet(ati_device_t *dev, uint32_t *packets, size_t dwords);

bool ati_dump_microcode(ati_device_t *dev, uint32_t *out);
bool ati_read_microcode(ati_device_t *dev, uint8_t addr, uint64_t *out);
bool ati_write_microcode(ati_device_t *dev, uint8_t addr, uint64_t inst);

#endif
