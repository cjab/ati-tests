/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TEST_H
#define TEST_H

// IWYU pragma: begin_exports
#include "../platform/platform.h"
// IWYU pragma: end_exports

#include "../ati/ati.h"
#include "error.h"

#define YELLOW "\033[33m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

#define ASSERT_EQ(actual, expected)                                            \
    do {                                                                       \
        uint32_t _a = (actual);                                                \
        uint32_t _e = (expected);                                              \
        if (_a != _e) {                                                        \
            error_printf(                                                      \
                "%s:%d: ASSERT_EQ failed: got 0x%08x, expected 0x%08x\n",      \
                __FILE__, __LINE__, _a, _e);                                   \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            error_printf("%s:%d: ASSERT_TRUE failed: %s\n", __FILE__,          \
                         __LINE__, #cond);                                     \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define FATAL                                                                  \
    do {                                                                       \
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,     \
                __FILE__, errno, strerror(errno));                             \
        exit(1);                                                               \
    } while (0)

// ============================================================================
// Test Registration
// ============================================================================

// Register a test with chip compatibility info
void register_test_internal(const char *id, const char *display_name,
                            bool (*func)(ati_device_t *),
                            ati_chip_family_t chips);

// Register a test that runs on all chips (default)
#define REGISTER_TEST(func, display_name)                                      \
    register_test_internal(#func, display_name, func, CHIP_ALL)

// Register a test for specific chip(s)
#define REGISTER_TEST_FOR(func, display_name, chips)                           \
    register_test_internal(#func, display_name, func, chips)

// Legacy compatibility - maps to internal function
static inline void
register_test(const char *id, const char *display_name,
              bool (*func)(ati_device_t *))
{
    register_test_internal(id, display_name, func, (ati_chip_family_t)CHIP_ALL);
}

// ============================================================================
// Draw direction enums for clipping tests
// ============================================================================

typedef enum { LEFT_TO_RIGHT = 1, RIGHT_TO_LEFT = 0 } horiz_dir_t;
typedef enum { TOP_TO_BOTTOM = 1, BOTTOM_TO_TOP = 0 } vert_dir_t;

void register_all_tests(void);
void run_all_tests(ati_device_t *dev);
void run_test_by_name(ati_device_t *dev, char *name);
void list_tests(void);

#endif
