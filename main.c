/* SPDX-License-Identifier: GPL-2.0-or-later */

// IWYU pragma: begin_exports
#include "platform/platform.h"
// IWYU pragma: end_exports

#include "ati.h"
#include "common.h"
#include "repl.h"

#define MAX_TESTS 100

typedef struct {
    const char *id;
    const char *display_name;
    bool (*func)(ati_device_t *);
} test_case_t;

static test_case_t tests[MAX_TESTS];
static int test_count = 0;

void
register_test(const char *id, const char *display_name,
              bool (*func)(ati_device_t *))
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
    ati_reset_for_test(dev);
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
            return;
        }
    }
    printf("Unknown test: %s\n", name);
}

void
list_tests(void)
{
    printf("Available tests:\n");
    for (int i = 0; i < test_count; i++) {
        printf("  %-35s - %s\n", tests[i].id, tests[i].display_name);
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
    platform_t *platform = platform_init(argc, argv);
    ati_device_t *dev = ati_device_init(platform->pci_dev);

    ati_set_display_mode(dev);
    ati_init_gui_engine(dev);

    register_all_tests();

    if (platform->argc > 0) {
        for (int i = 0; i < platform->argc; i++) {
            run_test_by_name(dev, platform->argv[i]);
        }
    } else {
        run_all_tests(dev);
    }

    repl(dev);

    ati_device_destroy(dev);
    platform_destroy(platform);
    return 0;
}
