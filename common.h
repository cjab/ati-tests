#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "ati.h"

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

void register_test(const char *name, void (*func)(ati_device_t *));
void register_all_tests(void);
void run_all_tests(ati_device_t *dev);

#endif
