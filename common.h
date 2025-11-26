/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef COMMON_H
#define COMMON_H

// IWYU pragma: begin_exports
#include "platform/platform.h"
// IWYU pragma: end_exports

#include "ati.h"

#define YELLOW "\033[33m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

#define ASSERT_EQ(actual, expected)                                            \
    do {                                                                       \
        uint32_t _a = (actual);                                                \
        uint32_t _e = (expected);                                              \
        if (_a != _e) {                                                        \
            fprintf(stderr,                                                    \
                    "%s:%d: ASSERT_EQ failed: got 0x%08x, expected 0x%08x\n",  \
                    __FILE__, __LINE__, _a, _e);                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "%s:%d: ASSERT_TRUE failed: %s\n", __FILE__,       \
                    __LINE__, #cond);                                          \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define FATAL                                                                  \
    do {                                                                       \
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,     \
                __FILE__, errno, strerror(errno));                             \
        exit(1);                                                               \
    } while (0)

void register_test(const char *id, const char *display_name, bool (*func)(ati_device_t *));

#define REGISTER_TEST(func, display_name) \
    register_test(#func, display_name, func)

void register_all_tests(void);
void run_all_tests(ati_device_t *dev);

#endif
