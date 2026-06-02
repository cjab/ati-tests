/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef R128_H
#define R128_H

#include "ati.h"

void r128_set_display_mode(ati_device_t *dev);
void ati_r128_init_gui_engine(ati_device_t *dev);
void ati_r128_wait_for_fifo(ati_device_t *dev, uint32_t entries);
void ati_r128_wait_for_engine(ati_device_t *dev);
void ati_r128_engine_flush(ati_device_t *dev);
void ati_r128_engine_reset(ati_device_t *dev);
uint32_t ati_r128_get_bytes_per_pixel(ati_device_t *dev);

#endif
