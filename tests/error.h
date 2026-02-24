/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef ERROR_H
#define ERROR_H

#include "../ati/ati.h"

/* Error buffer for deferred test error reporting.
 *
 * During test execution, errors are buffered rather than printed
 * immediately. After the test verdict (PASS/FAILED) is printed,
 * the error buffer is flushed wrapped in \x1D (Group Separator)
 * control characters for structured parsing by the console client.
 */

/* Append formatted text to the error buffer. */
void error_printf(const char *fmt, ...);

/* Flush the error buffer: emits \x1D, buffer contents, \x1D,
 * then clears the buffer. No-op if the buffer is empty. */
void error_flush(void);

/* Clear the error buffer without printing. */
void error_clear(void);

/* Schedule a screen dump to be sent after the error block.
 * The dump is deferred so it appears after the error markers. */
void error_set_pending_dump(const char *path);

/* Send the pending screen dump if one was scheduled, then clear it. */
void error_flush_dump(ati_device_t *dev);

#endif
