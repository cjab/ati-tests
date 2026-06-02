/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef R100_H
#define R100_H

#include "ati.h"

void r100_set_display_mode(ati_device_t *dev);
void ati_r100_init_gui_engine(ati_device_t *dev);
uint32_t ati_r100_get_bytes_per_pixel(ati_device_t *dev);

#endif
