/* SPDX-License-Identifier: GPL-2.0-or-later */

// IWYU pragma: begin_exports
#include "platform/platform.h"
// IWYU pragma: end_exports

#include "ati.h"
#include "common.h"

#define MAX_TESTS 100

typedef struct {
    const char *id;
    const char *display_name;
    bool (*func)(ati_device_t *);
} test_case_t;

static test_case_t tests[MAX_TESTS];
static int test_count = 0;

void
register_test(const char *id, const char *display_name, bool (*func)(ati_device_t *))
{
    if (test_count >= MAX_TESTS) {
        fprintf(stderr, "Too many tests registered");
        exit(1);
    }
    tests[test_count].id = id;
    tests[test_count].display_name = display_name;
    tests[test_count].func = func;
    test_count += 1;
}

void
run_test(ati_device_t *dev, const test_case_t *test)
{
    printf("  %s ... ", test->display_name);
    fflush(stdout);
    if (test->func(dev)) {
        printf(GREEN "ok" RESET "\n");
    } else {
        printf(RED "FAILED" RESET "\n");
    };
}

void
run_all_tests(ati_device_t *dev)
{
    printf("\nRunning tests...\n");
    for (int i = 0; i < test_count; i++) {
        run_test(dev, &tests[i]);
    }
}

void
run_test_by_name(ati_device_t *dev, char *name)
{
    for (int i = 0; i < test_count; i++) {
        if (!strcmp(name, tests[i].id)) {
            run_test(dev, &tests[i]);
        }
    }
}

extern void register_clipping_tests(void);
extern void register_pitch_offset_cntl_tests(void);
extern void register_host_data_tests(void);
extern void register_rop3_tests(void);

void
register_all_tests(void)
{
    register_clipping_tests();
    register_pitch_offset_cntl_tests();
    register_host_data_tests();
    register_rop3_tests();
}

int
main(int argc, char **argv)
{
    ati_device_t *dev = ati_device_init();

    register_all_tests();

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            run_test_by_name(dev, argv[i]);
        }
    } else {
        run_all_tests(dev);
    }

    printf("\n");

    ati_device_destroy(dev);
    return 0;
}

// Multiboot v1 information
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    // And more... See:
    // https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Boot-information-format
} multiboot_info_t;

void
kernel_main(uint32_t magic, multiboot_info_t *mbi)
{
    platform_init();

    if (magic != 0x2BADB002) {
        printf("Invalid multiboot magic\n");
        return;
    }

    // Bit 2 of flags determines if cmdline string is present.
    if (mbi->flags & (1 << 2)) {
        char *cmdline = (char *) (uintptr_t) mbi->cmdline;
        printf("Kernel cmdline: %s\n", cmdline);
    }

    ati_device_t *dev = ati_device_init();
    ati_set_display_mode(dev);
    ati_init_gui_engine(dev);
    register_all_tests();
    run_all_tests(dev);
    printf("\n\nTests complete. Halting.\n");
    ati_device_destroy(dev);
}
