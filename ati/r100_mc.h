#ifndef R100_MC_H
#define R100_MC_H

#include "ati.h"

extern uint32_t page_table[32];
extern volatile uint32_t gart_mem[1024];

uint32_t ati_r100_init_pci_gart(ati_device_t *dev);
void ati_r100_disable_pci_gart(ati_device_t *dev);

#endif
