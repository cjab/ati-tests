/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Simple ACPI poweroff implementation for baremetal x86.
 *
 * Based on the approach described at:
 * https://forum.osdev.org/viewtopic.php?t=16990
 *
 * This parses ACPI tables to find the PM1a control register and S5 sleep
 * type value needed for soft-off. It does NOT implement a full AML
 * interpreter - it manually scans the DSDT for the _S5_ object.
 */

#include <stddef.h>
#include <stdint.h>

#include "acpi.h"
#include "tinyprintf.h"

// External functions from baremetal.c
extern void outw(uint16_t port, uint16_t val);
extern uint16_t inw(uint16_t port);
extern int memcmp(const void *s1, const void *s2, size_t n);

// For SMI command
static inline void
outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Sleep enable bit in PM1_CNT register
#define SLP_EN (1 << 13)
// SCI enable bit - indicates ACPI mode is active
#define SCI_EN (1 << 0)

// ACPI state - populated by acpi_init()
static int acpi_available = 0;
static uint32_t pm1a_cnt_blk = 0;
static uint32_t pm1b_cnt_blk = 0;
static uint32_t smi_cmd = 0;
static uint8_t acpi_enable_val = 0;
static uint16_t slp_typa = 0;
static uint16_t slp_typb = 0;

// RSDP - Root System Description Pointer (ACPI 1.0, 20 bytes)
struct rsdp {
    char signature[8]; // "RSD PTR "
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed));

// Standard ACPI table header
struct acpi_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

// FADT - Fixed ACPI Description Table (partial, fields we care about)
struct fadt {
    struct acpi_header header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved1;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    // ... more fields we don't need
} __attribute__((packed));

// Validate checksum (sum of all bytes should be 0)
static int
acpi_checksum(const void *data, size_t len)
{
    const uint8_t *bytes = data;
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    return sum == 0;
}

// Search for RSDP in a memory region
static struct rsdp *
acpi_find_rsdp_in_region(uint32_t start, uint32_t end)
{
    // RSDP is always 16-byte aligned
    for (uint32_t addr = start; addr < end; addr += 16) {
        struct rsdp *rsdp = (struct rsdp *) (uintptr_t) addr;
        if (memcmp(rsdp->signature, "RSD PTR ", 8) == 0) {
            if (acpi_checksum(rsdp, sizeof(struct rsdp))) {
                return rsdp;
            }
        }
    }
    return NULL;
}

// Find the RSDP by searching standard locations
static struct rsdp *
acpi_find_rsdp(void)
{
    struct rsdp *rsdp;

    // Search EBDA (Extended BIOS Data Area)
    // The EBDA segment is at 0x40:0x0E (real mode address)
    uint16_t ebda_seg = *(uint16_t *) (uintptr_t) 0x40E;
    uint32_t ebda_addr = (uint32_t) ebda_seg << 4;
    if (ebda_addr) {
        rsdp = acpi_find_rsdp_in_region(ebda_addr, ebda_addr + 1024);
        if (rsdp)
            return rsdp;
    }

    // Search main BIOS area (0xE0000 - 0xFFFFF)
    rsdp = acpi_find_rsdp_in_region(0xE0000, 0x100000);
    if (rsdp)
        return rsdp;

    return NULL;
}

// Find a table by signature in the RSDT
static struct acpi_header *
acpi_find_table(struct acpi_header *rsdt, const char *sig)
{
    if (memcmp(rsdt->signature, "RSDT", 4) != 0) {
        return NULL;
    }

    // Number of entry pointers after the header
    int entries = (rsdt->length - sizeof(struct acpi_header)) / 4;
    uint32_t *entry_ptr = (uint32_t *) ((uint8_t *) rsdt + sizeof(struct acpi_header));

    for (int i = 0; i < entries; i++) {
        struct acpi_header *table = (struct acpi_header *) (uintptr_t) entry_ptr[i];
        if (memcmp(table->signature, sig, 4) == 0) {
            if (acpi_checksum(table, table->length)) {
                return table;
            }
        }
    }

    return NULL;
}

// Parse the _S5_ object from the DSDT to get SLP_TYPa/b values
// Returns 0 on success, -1 on failure
static int
acpi_parse_s5(struct acpi_header *dsdt)
{
    if (memcmp(dsdt->signature, "DSDT", 4) != 0) {
        printf("ACPI: Invalid DSDT signature\n");
        return -1;
    }

    // Search for "_S5_" in the DSDT
    uint8_t *start = (uint8_t *) dsdt + sizeof(struct acpi_header);
    uint8_t *end = (uint8_t *) dsdt + dsdt->length;

    for (uint8_t *p = start; p < end - 4; p++) {
        if (memcmp(p, "_S5_", 4) == 0) {
            // Found _S5_, now parse the AML structure
            // Expected format:
            //   NameOp (0x08) [optional '\' 0x5C] '_S5_' PackageOp (0x12) ...
            //
            // Check that this looks like a valid NameOp definition
            if ((*(p - 1) == 0x08) ||
                (*(p - 2) == 0x08 && *(p - 1) == '\\')) {

                // Move past "_S5_"
                p += 4;

                // Should be PackageOp (0x12)
                if (*p != 0x12) {
                    printf("ACPI: _S5_ not followed by PackageOp\n");
                    continue;
                }
                p++;

                // Parse PkgLength - bits 6-7 indicate additional length bytes
                // After PkgLength comes NumElements
                int pkg_len_extra = (*p >> 6) & 0x03;
                p += pkg_len_extra + 1;  // Skip PkgLength byte(s)
                p++;  // Skip NumElements

                // Now we should have the SLP_TYPa value
                // It might have a BytePrefix (0x0A) or be a raw byte, or ZeroOp (0x00)
                if (*p == 0x0A)
                    p++;
                slp_typa = (uint16_t) (*p) << 10;
                p++;

                // SLP_TYPb
                if (*p == 0x0A)
                    p++;
                slp_typb = (uint16_t) (*p) << 10;

                printf("ACPI: Found _S5_ - SLP_TYPa=0x%x, SLP_TYPb=0x%x\n",
                       slp_typa, slp_typb);
                return 0;
            }
        }
    }

    printf("ACPI: _S5_ object not found in DSDT\n");
    return -1;
}

int
acpi_init(void)
{
    printf("ACPI: Initializing...\n");

    // Find RSDP
    struct rsdp *rsdp = acpi_find_rsdp();
    if (!rsdp) {
        printf("ACPI: RSDP not found\n");
        return -1;
    }
    printf("ACPI: Found RSDP at 0x%x, RSDT at 0x%x\n",
           (uint32_t) (uintptr_t) rsdp, rsdp->rsdt_address);

    // Get RSDT
    struct acpi_header *rsdt = (struct acpi_header *) (uintptr_t) rsdp->rsdt_address;
    if (!acpi_checksum(rsdt, rsdt->length)) {
        printf("ACPI: RSDT checksum failed\n");
        return -1;
    }

    // Find FADT (signature is "FACP")
    struct fadt *fadt = (struct fadt *) acpi_find_table(rsdt, "FACP");
    if (!fadt) {
        printf("ACPI: FADT not found\n");
        return -1;
    }
    printf("ACPI: Found FADT - PM1a_CNT=0x%x, PM1b_CNT=0x%x, DSDT=0x%x\n",
           fadt->pm1a_cnt_blk, fadt->pm1b_cnt_blk, fadt->dsdt);

    pm1a_cnt_blk = fadt->pm1a_cnt_blk;
    pm1b_cnt_blk = fadt->pm1b_cnt_blk;
    smi_cmd = fadt->smi_cmd;
    acpi_enable_val = fadt->acpi_enable;

    // Parse DSDT to find _S5_ sleep type values
    struct acpi_header *dsdt = (struct acpi_header *) (uintptr_t) fadt->dsdt;
    if (acpi_parse_s5(dsdt) != 0) {
        return -1;
    }

    acpi_available = 1;
    printf("ACPI: Initialization complete\n");
    return 0;
}

// Enable ACPI mode if not already enabled
static int
acpi_enable(void)
{
    // Check if ACPI is already enabled (SCI_EN bit set)
    uint16_t pm1a_val = inw((uint16_t) pm1a_cnt_blk);

    if (pm1a_val & SCI_EN) {
        return 0;
    }

    // Try to enable ACPI
    if (smi_cmd == 0 || acpi_enable_val == 0) {
        printf("ACPI: No SMI command available to enable ACPI\n");
        return -1;
    }

    printf("ACPI: Enabling via SMI_CMD...\n");
    outb((uint16_t) smi_cmd, acpi_enable_val);

    // Wait for ACPI to be enabled (with timeout)
    for (int i = 0; i < 300; i++) {
        pm1a_val = inw((uint16_t) pm1a_cnt_blk);
        if (pm1a_val & SCI_EN) {
            return 0;
        }
        // Simple delay - not accurate but good enough
        for (volatile int j = 0; j < 100000; j++)
            ;
    }

    printf("ACPI: Failed to enable (timeout)\n");
    return -1;
}

void
acpi_poweroff(void)
{
    if (!acpi_available) {
        printf("ACPI: Not available, cannot power off\n");
        return;
    }

    printf("ACPI: Powering off...\n");

    // Enable ACPI if needed
    acpi_enable();

    // Send the shutdown command
    outw((uint16_t) pm1a_cnt_blk, slp_typa | SLP_EN);

    // If PM1b is also set, write to it too
    if (pm1b_cnt_blk) {
        outw((uint16_t) pm1b_cnt_blk, slp_typb | SLP_EN);
    }

    // If we get here, shutdown failed
    printf("ACPI: Poweroff failed!\n");

    // Halt
    while (1) {
        __asm__ volatile("hlt");
    }
}
