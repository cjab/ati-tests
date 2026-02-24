/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "error.h"

#include <stdarg.h>
#include <string.h>

#define ERROR_BUF_SIZE 4096
#define RECORD_GROUP_SEP "\x1d" // Group Separator — delimits error records

static char error_buf[ERROR_BUF_SIZE];
static size_t error_len;

static char pending_dump_path[256];

void
error_printf(const char *fmt, ...)
{
    if (error_len >= ERROR_BUF_SIZE - 1)
        return;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(error_buf + error_len,
                      ERROR_BUF_SIZE - error_len, fmt, ap);
    va_end(ap);

    if (n > 0)
        error_len += (size_t) n;
}

void
error_flush(void)
{
    if (error_len == 0)
        return;

    printf(RECORD_GROUP_SEP "%s" RECORD_GROUP_SEP, error_buf);
    fflush(stdout);

    error_clear();
}

void
error_clear(void)
{
    error_len = 0;
    error_buf[0] = '\0';
}

void
error_set_pending_dump(const char *path)
{
    snprintf(pending_dump_path, sizeof(pending_dump_path), "%s", path);
}

void
error_flush_dump(ati_device_t *dev)
{
    if (pending_dump_path[0] == '\0')
        return;

    ati_screen_dump(dev, pending_dump_path);
    pending_dump_path[0] = '\0';
}
