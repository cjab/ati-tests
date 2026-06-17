#ifndef R128_CCE_H
#define R128_CCE_H

#include "ati.h"

// CCE engine functions
void ati_r128_init_cce_engine(ati_device_t *dev, uint32_t mode);
void ati_r128_start_cce_engine(ati_device_t *dev, uint32_t mode);
void ati_r128_stop_cce_engine(ati_device_t *dev);

void ati_r128_dump_microcode(ati_device_t *dev, uint32_t *out);
uint64_t ati_r128_read_microcode(ati_device_t *dev, uint8_t addr);
void ati_r128_write_microcode(ati_device_t *dev, uint8_t addr, uint64_t inst);

void ati_r128_cce_pio_submit(ati_device_t *dev, uint32_t *packets, size_t dwords);
int ati_r128_cce_wait_for_idle(ati_device_t *dev);
int ati_r128_flush_pixcache(ati_device_t *dev);

#endif
