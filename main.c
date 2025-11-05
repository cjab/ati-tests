#include <stdlib.h>

#include "common.h"
#include "ati.h"

#define MAX_TESTS 100

typedef struct {
  const char *name;
  void (*func)(ati_device_t *);
} test_case_t;

static test_case_t tests[MAX_TESTS];
static int test_count = 0;

void register_test(const char *name, void (*func)(ati_device_t *)) {
  if (test_count >= MAX_TESTS) {
    fprintf(stderr, "Too many tests registered");
    exit(1);
  }
  tests[test_count].name = name;
  tests[test_count].func = func;
  test_count += 1;
}

void run_all_tests(ati_device_t *dev) {
  for (int i = 0; i < test_count; i++) {
    printf("\n=== Running: %s ===\n", tests[i].name);
    tests[i].func(dev);
  }
}

extern void register_clipping_tests(void);
extern void register_pitch_offset_cntl_tests(void);

void register_all_tests(void) {
  register_clipping_tests();
  register_pitch_offset_cntl_tests();
}

int main(void) {
  ati_device_t *ati = ati_device_init();

  register_all_tests();
  run_all_tests(ati);

  ati_device_destroy(ati);
  return 0;
}

