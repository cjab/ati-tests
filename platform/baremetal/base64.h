/* SPDX-License-Identifier: Unlicense
 *
 * Minimal Base64 encoder for baremetal use.
 * Based on public domain code from https://github.com/zhicheng/base64
 */
#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>
#include <stdint.h>

/* Calculate output size for Base64 encoding (without null terminator) */
#define BASE64_ENCODE_OUT_SIZE(s) (((s) + 2) / 3 * 4)

/*
 * Encode data to Base64.
 *
 * in:    input data
 * inlen: input length in bytes
 * out:   output buffer (must be at least BASE64_ENCODE_OUT_SIZE(inlen) bytes)
 *
 * Returns: number of characters written to out (not null-terminated)
 */
static inline size_t
base64_encode(const uint8_t *in, size_t inlen, char *out)
{
    static const char b64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t i, j;

    for (i = j = 0; i < inlen; i += 3) {
        uint32_t a = in[i];
        uint32_t b = (i + 1 < inlen) ? in[i + 1] : 0;
        uint32_t c = (i + 2 < inlen) ? in[i + 2] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;

        out[j++] = b64_table[(triple >> 18) & 0x3F];
        out[j++] = b64_table[(triple >> 12) & 0x3F];
        out[j++] = (i + 1 < inlen) ? b64_table[(triple >> 6) & 0x3F] : '=';
        out[j++] = (i + 2 < inlen) ? b64_table[triple & 0x3F] : '=';
    }

    return j;
}

#endif /* BASE64_H */
