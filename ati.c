#include <pci/pci.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "ati.h"
#include "common.h"

#define ATI_VENDOR_ID            0x1002
#define MAX_ATI_DEVICES          10
#define NUM_BARS 8

struct ati_device {
  struct pci_access *pacc;
  struct pci_dev *pci_dev;
  char name[256];
  void *bar[NUM_BARS];
};

void print_devices(struct pci_access *pacc);
struct pci_dev *find_device(struct pci_access *pacc, char *name_out,
                            int name_len);
void *map_bar(struct pci_dev *dev, int bar_idx);

ati_device_t *ati_device_init(void) {
  ati_device_t *ati = malloc(sizeof(ati_device_t));
  ati->pacc = pci_alloc();
  ati->pci_dev = find_device(ati->pacc, ati->name, sizeof(ati->name));
  ati->bar[2] = map_bar(ati->pci_dev, 2);
  return ati;
}

void ati_device_destroy(ati_device_t *dev) {
  if (!dev) return;
  for (int i = 0; i < NUM_BARS; i++) {
    if (dev->bar[i]) munmap(dev->bar[i], dev->pci_dev->size[i]);
  }
  if (dev->pacc) pci_cleanup(dev->pacc);
  free(dev);
}

static inline uint32_t reg_read(void *base, uint32_t offset) {
  volatile uint32_t *reg = (volatile uint32_t *)((char *)base + offset);
  return *reg;
}

static inline void reg_write(void *base, uint32_t offset, uint32_t value) {
  volatile uint32_t *reg = (volatile uint32_t *)((char *)base + offset);
  *reg = value;
}

uint32_t ati_reg_read(ati_device_t *dev, uint32_t offset) {
  return reg_read(dev->bar[2], offset);
}

void ati_reg_write(ati_device_t *dev, uint32_t offset, uint32_t value) {
  reg_write(dev->bar[2], offset, value);
}

struct pci_dev *find_device(struct pci_access *pacc, char *name_out,
                            int name_len) {
  struct pci_dev *dev, *it;
  int device_count = 0;

  pci_init(pacc);
  pci_scan_bus(pacc);

  for (it = pacc->devices; it; it = it->next) {
    if (it->vendor_id == ATI_VENDOR_ID) {
      if (device_count == 0) {
        dev = it;
      }
      device_count += 1;
    }
  }

  if (device_count == 0) {
    printf("No ATI devices found\n");
    exit(1);
  }

  if (device_count > 1) {
    printf("Found multiple ATI devices:\n");
    print_devices(pacc);
  }

  pci_lookup_name(pacc, name_out, name_len,
                  PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, dev->vendor_id,
                  dev->device_id);

  printf("# %s\n\n", name_out);

  return dev;
}

void print_devices(struct pci_access *pacc) {
  struct pci_dev *dev;
  char name[256];

  for (dev = pacc->devices; dev; dev = dev->next) {
    if (dev->vendor_id != ATI_VENDOR_ID)
      continue;
    pci_lookup_name(pacc, name, sizeof(name),
                    PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, dev->vendor_id,
                    dev->device_id);
    printf("\t- %s\n", name);
  }
}

void *map_bar(struct pci_dev *dev, int bar_idx) {
  char pci_loc[32];
  sprintf(pci_loc, "%04x:%02x:%02x.%d", dev->domain, dev->bus, dev->dev,
          dev->func);

  char base_path[256];
  sprintf(base_path, "/sys/bus/pci/devices/%s", pci_loc);

  char bar_path[512];
  sprintf(bar_path, "%s/resource%d", base_path, bar_idx);

  int bar_fd = open(bar_path, O_RDWR | O_SYNC);
  if (bar_fd == -1)
    FATAL;

  void *bar = mmap(NULL, dev->size[bar_idx], PROT_READ | PROT_WRITE, MAP_SHARED,
                   bar_fd, 0);
  if (bar == (void *)-1)
    FATAL;

  return bar;
}
