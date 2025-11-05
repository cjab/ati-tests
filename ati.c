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

/* Register accessor functions */

uint32_t rd_dp_gui_master_cntl(ati_device_t *dev) {
  return ati_reg_read(dev, DP_GUI_MASTER_CNTL);
}

void wr_dp_gui_master_cntl(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_GUI_MASTER_CNTL, val);
}

void wr_src_sc_bottom_right(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SRC_SC_BOTTOM_RIGHT, val);
}

uint32_t rd_src_sc_bottom(ati_device_t *dev) {
  return ati_reg_read(dev, SRC_SC_BOTTOM);
}

void wr_src_sc_bottom(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SRC_SC_BOTTOM, val);
}

uint32_t rd_src_sc_right(ati_device_t *dev) {
  return ati_reg_read(dev, SRC_SC_RIGHT);
}

void wr_src_sc_right(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SRC_SC_RIGHT, val);
}

uint32_t rd_default_sc_bottom_right(ati_device_t *dev) {
  return ati_reg_read(dev, DEFAULT_SC_BOTTOM_RIGHT);
}

void wr_default_sc_bottom_right(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DEFAULT_SC_BOTTOM_RIGHT, val);
}

uint32_t rd_sc_left(ati_device_t *dev) {
  return ati_reg_read(dev, SC_LEFT);
}

void wr_sc_left(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SC_LEFT, val);
}

uint32_t rd_sc_top(ati_device_t *dev) {
  return ati_reg_read(dev, SC_TOP);
}

void wr_sc_top(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SC_TOP, val);
}

void wr_sc_bottom_right(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SC_BOTTOM_RIGHT, val);
}

uint32_t rd_sc_right(ati_device_t *dev) {
  return ati_reg_read(dev, SC_RIGHT);
}

void wr_sc_right(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SC_RIGHT, val);
}

uint32_t rd_sc_bottom(ati_device_t *dev) {
  return ati_reg_read(dev, SC_BOTTOM);
}

void wr_sc_bottom(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SC_BOTTOM, val);
}

uint32_t rd_dst_offset(ati_device_t *dev) {
  return ati_reg_read(dev, DST_OFFSET);
}

void wr_dst_offset(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_OFFSET, val);
}

uint32_t rd_dst_pitch(ati_device_t *dev) {
  return ati_reg_read(dev, DST_PITCH);
}

void wr_dst_pitch(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_PITCH, val);
}

uint32_t rd_default_offset(ati_device_t *dev) {
  return ati_reg_read(dev, DEFAULT_OFFSET);
}

void wr_default_offset(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DEFAULT_OFFSET, val);
}

uint32_t rd_default_pitch(ati_device_t *dev) {
  return ati_reg_read(dev, DEFAULT_PITCH);
}

void wr_default_pitch(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DEFAULT_PITCH, val);
}

uint32_t rd_src_offset(ati_device_t *dev) {
  return ati_reg_read(dev, SRC_OFFSET);
}

void wr_src_offset(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SRC_OFFSET, val);
}

uint32_t rd_src_pitch(ati_device_t *dev) {
  return ati_reg_read(dev, SRC_PITCH);
}

void wr_src_pitch(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SRC_PITCH, val);
}
