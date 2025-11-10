#include <stdlib.h>

#include "common.h"
#include "ati.h"

#define MAX_TESTS 100

typedef struct {
  const char *name;
  bool (*func)(ati_device_t *);
} test_case_t;

static test_case_t tests[MAX_TESTS];
static int test_count = 0;

void register_test(const char *name, bool (*func)(ati_device_t *)) {
  if (test_count >= MAX_TESTS) {
    fprintf(stderr, "Too many tests registered");
    exit(1);
  }
  tests[test_count].name = name;
  tests[test_count].func = func;
  test_count += 1;
}

void run_test(ati_device_t *dev, const test_case_t *test) {
  printf("  %s ... ", test->name);
  if (test->func(dev)) {
    printf(GREEN "ok" RESET "\n");
  } else {
    printf(RED "FAILED" RESET "\n");
    ati_screen_dump(dev, "failed-vram.bin");
  };
}

void run_all_tests(ati_device_t *dev) {
  printf("\nRunning tests...\n");
  for (int i = 0; i < test_count; i++) {
    run_test(dev, &tests[i]);
  }
}

void run_test_by_name(ati_device_t *dev, char *name) {
  for (int i = 0; i < test_count; i++) {
    if (!strcmp(name, tests[i].name)) {
      run_test(dev, &tests[i]);
    }
  }
}

extern void register_clipping_tests(void);
extern void register_pitch_offset_cntl_tests(void);
extern void register_host_data_tests(void);

void register_all_tests(void) {
  register_clipping_tests();
  register_pitch_offset_cntl_tests();
  register_host_data_tests();
}

int main(int argc, char **argv) {
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

