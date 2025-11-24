#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>

#define ATI_VENDOR_ID 0x1002

typedef struct platform_pci_device platform_pci_device_t;

platform_pci_device_t *platform_pci_init(void);
void platform_pci_destroy(platform_pci_device_t *dev);
void platform_pci_get_name(platform_pci_device_t *dev, char *buf, size_t len);

void *platform_pci_map_bar(platform_pci_device_t *dev, int bar_idx);
void platform_pci_unmap_bar(platform_pci_device_t *dev, void *addr,
                            int bar_idx);
size_t platform_pci_get_bar_size(platform_pci_device_t *dev, int bar_idx);

#endif
