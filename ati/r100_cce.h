#ifndef R100_CCE_H
#define R100_CCE_H

#include "ati.h"

void ati_r100_init_cce_engine(ati_device_t *dev);
void ati_r100_start_cce_engine(ati_device_t *dev);
void ati_r100_stop_cce_engine(ati_device_t *dev);

void ati_r100_dump_microcode(ati_device_t *dev, uint32_t *out);
uint64_t ati_r100_read_microcode(ati_device_t *dev, uint8_t addr);
void ati_r100_write_microcode(ati_device_t *dev, uint8_t addr, uint64_t inst);

void ati_r100_cce_pio_submit(ati_device_t *dev, uint32_t *packets, size_t dwords);
int ati_r100_cce_wait_for_idle(ati_device_t *dev);
int ati_r100_flush_pixcache(ati_device_t *dev);

#endif
