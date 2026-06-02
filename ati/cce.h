#ifndef CCE_H
#define CCE_H

#include "ati.h"

bool ati_init_cce_engine(ati_device_t *dev);
bool ati_start_cce_engine(ati_device_t *dev);
bool ati_stop_cce_engine(ati_device_t *dev);

bool ati_dump_microcode(ati_device_t *dev, uint32_t *out);
bool ati_read_microcode(ati_device_t *dev, uint8_t addr, uint64_t *out);
bool ati_write_microcode(ati_device_t *dev, uint8_t addr, uint64_t inst);

#endif
