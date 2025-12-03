/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef ACPI_H
#define ACPI_H

// Initialize ACPI - parse tables, extract shutdown info
// Returns 0 on success, -1 if ACPI not available
int acpi_init(void);

// Power off the system via ACPI S5 state
// Only works if acpi_init() succeeded
void acpi_poweroff(void);

#endif
