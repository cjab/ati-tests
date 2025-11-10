#include <pci/pci.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "ati.h"
#include "common.h"

#define ATI_VENDOR_ID     0x1002
#define MAX_ATI_DEVICES       10
#define NUM_BARS               8

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
  ati->bar[0] = map_bar(ati->pci_dev, 0);
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

uint32_t ati_vram_read(ati_device_t *dev, uint32_t offset) {
  return reg_read(dev->bar[0], offset);
}

void ati_vram_write(ati_device_t *dev, uint32_t offset, uint32_t value) {
  reg_write(dev->bar[0], offset, value);
}

uint64_t ati_vram_search(ati_device_t *dev, uint32_t needle) {
  volatile uint32_t *vram = (volatile uint32_t *)dev->bar[0];
  size_t vram_size = dev->pci_dev->size[0];
  size_t dwords = vram_size / 4;

  for (size_t i = 0; i < dwords; i++) {
    if (vram[i] == needle) {
      return (uint64_t)i * 4;
    }
  }

  return VRAM_NOT_FOUND;
}

void ati_vram_memcpy(ati_device_t *dev, uint32_t dst_offset,
                         const void *src, size_t size) {
  size_t vram_size = dev->pci_dev->size[0];
  if (size > vram_size) {
    printf("Copying data larger than BAR0\n");
  }

  memcpy(dev->bar[0], src, size);
}

bool ati_screen_compare_file(ati_device_t *dev, const char *filename) {
      FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open fixture %s: %s\n",
                filename, strerror(errno));
        return false;
    }

    // FIXME: Do better
    // Get file size
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t screen_size = 640 * 480 * 4;
    if (file_size != screen_size) {
        fprintf(stderr, "Reference file size mismatch: expected %zu, got %zu\n",
                screen_size, file_size);
        fclose(f);
        return false;
    }

    uint8_t *fixture = malloc(screen_size);
    if (!fixture) {
        fprintf(stderr, "Failed to allocate memory for fixture\n");
        fclose(f);
        return false;
    }
    size_t read = fread(fixture, 1, screen_size, f);
    fclose(f);
    if (read != screen_size) {
        fprintf(stderr, "Failed to read complete fixture file\n");
        free(fixture);
        return false;
    }

    // Compare with current framebuffer
    volatile uint8_t *vram = (volatile uint8_t *)dev->bar[0];
    bool match = true;
    int first_mismatch = -1;
    int mismatch_count = 0;
    for (size_t i = 0; i < screen_size; i++) {
        if (vram[i] != fixture[i]) {
            if (first_mismatch == -1) {
                first_mismatch = i;
            }
            mismatch_count++;
        }
    }
    if (!match || mismatch_count > 0) {
        match = false;
        printf("MISMATCH: %d bytes differ\n", mismatch_count);
        if (first_mismatch >= 0) {
            printf("First mismatch at byte offset 0x%x:\n", first_mismatch);
            printf("  Expected: 0x%02x\n", fixture[first_mismatch]);
            printf("  Got:      0x%02x\n", vram[first_mismatch]);

            int pixel_offset = first_mismatch / 4;
            int y = pixel_offset / 640;
            int x = pixel_offset % 640;
            printf("  Pixel at (%d, %d)\n", x, y);
        }
    }
    free(fixture);
    return match;
}

void ati_screen_clear(ati_device_t *dev) {
  size_t screen_size = 640 * 480 * 4;
  memset(dev->bar[0], NULL, screen_size);
}

void ati_vram_clear(ati_device_t *dev) {
  size_t vram_size = dev->pci_dev->size[0];
  memset(dev->bar[0], NULL, vram_size);
}

void ati_screen_dump(ati_device_t *dev, const char *filename) {
  volatile uint32_t *vram = (volatile uint32_t *)dev->bar[0];
  size_t screen_size = 640 * 480 * 4;

  FILE *f = fopen(filename, "wb");
  if (!f) {
    fprintf(stderr, "Failed to open %s for writing: %s\n",
            filename, strerror(errno));
  }

  size_t written = fwrite((void *)vram, 1, screen_size, f);

  if (written != screen_size) {
    fprintf(stderr, "Warning: only wrote %zu of %zu bytes\n",
            written, screen_size);
  }
  fclose(f);
  printf("Dumped %zu bytes of screen to %s\n", written, filename);
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

void wr_sc_top_left(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, SC_TOP_LEFT, val);
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

void wr_dst_x_y(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_X_Y, val);
}

void wr_dst_y_x(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_Y_X, val);
}

uint32_t rd_dst_x(ati_device_t *dev) {
  ati_reg_read(dev, DST_X);
}

void wr_dst_x(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_X, val);
}

uint32_t rd_dst_y(ati_device_t *dev) {
  ati_reg_read(dev, DST_Y);
}

void wr_dst_y(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_Y, val);
}

void wr_dst_width_height(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_WIDTH_HEIGHT, val);
}

uint32_t rd_dst_width(ati_device_t *dev) {
  ati_reg_read(dev, DST_WIDTH);
}

void wr_dst_width(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_WIDTH, val);
}

uint32_t rd_dst_height(ati_device_t *dev) {
  ati_reg_read(dev, DST_HEIGHT);
}

void wr_dst_height(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DST_HEIGHT, val);
}

uint32_t rd_dp_datatype(ati_device_t *dev) {
  return ati_reg_read(dev, DP_DATATYPE);
}

void wr_dp_datatype(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_DATATYPE, val);
}

uint32_t rd_dp_mix(ati_device_t *dev) {
  return ati_reg_read(dev, DP_MIX);
}

void wr_dp_mix(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_MIX, val);
}

uint32_t rd_dp_write_msk(ati_device_t *dev) {
  return ati_reg_read(dev, DP_WRITE_MSK);
}

void wr_dp_write_msk(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_WRITE_MSK, val);
}

uint32_t rd_dp_src_frgd_clr(ati_device_t *dev) {
  return ati_reg_read(dev, DP_SRC_FRGD_CLR);
}

void wr_dp_src_frgd_clr(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_SRC_FRGD_CLR, val);
}

uint32_t rd_dp_src_bkgd_clr(ati_device_t *dev) {
  return ati_reg_read(dev, DP_SRC_BKGD_CLR);
}

void wr_dp_src_bkgd_clr(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_SRC_BKGD_CLR, val);
}

uint32_t rd_gui_stat(ati_device_t *dev) {
  return ati_reg_read(dev, GUI_STAT);
}

void wr_host_data0(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA0, val);
}

void wr_host_data1(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA1, val);
}

void wr_host_data2(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA2, val);
}

void wr_host_data3(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA3, val);
}

void wr_host_data4(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA4, val);
}

void wr_host_data5(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA5, val);
}

void wr_host_data6(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA6, val);
}

void wr_host_data7(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA7, val);
}

void wr_host_data_last(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, HOST_DATA_LAST, val);
}

uint32_t rd_dp_cntl(ati_device_t *dev) {
  return ati_reg_read(dev, DP_CNTL);
}

void wr_dp_cntl(ati_device_t *dev, uint32_t val) {
  ati_reg_write(dev, DP_CNTL, val);
}
