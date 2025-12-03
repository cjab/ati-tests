/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "ati.h"
#include "platform/platform.h"

// Generate register name lookup table from X-macro
typedef struct {
    const char *name;
    uint32_t offset;
} reg_entry_t;

#define X(func_name, const_name, offset, mode) {#const_name, offset},
static const reg_entry_t reg_table[] = {ATI_REGISTERS{NULL, 0}};
#undef X

static int
strcasecmp_simple(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        char c1 = *s1++;
        char c2 = *s2++;
        if (c1 >= 'a' && c1 <= 'z')
            c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z')
            c2 -= 32;
        if (c1 != c2)
            return c1 - c2;
    }
    return *s1 - *s2;
}

static int
lookup_reg(const char *name, uint32_t *out)
{
    for (int i = 0; reg_table[i].name != NULL; i++) {
        if (strcasecmp_simple(name, reg_table[i].name) == 0) {
            *out = reg_table[i].offset;
            return 0;
        }
    }
    return -1;
}

static int
parse_hex(const char *s, uint32_t *out)
{
    uint32_t val = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    if (*s == '\0')
        return -1;

    while (*s) {
        char c = *s++;
        uint32_t digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else
            return -1;
        val = (val << 4) | digit;
    }

    *out = val;
    return 0;
}

// Parse integer: decimal by default, hex with 0x prefix
static int
parse_int(const char *s, uint32_t *out)
{
    // If it starts with 0x, parse as hex
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return parse_hex(s, out);
    }

    // Otherwise parse as decimal
    uint32_t val = 0;

    if (*s == '\0')
        return -1;

    while (*s) {
        char c = *s++;
        if (c >= '0' && c <= '9')
            val = val * 10 + (c - '0');
        else
            return -1;
    }

    *out = val;
    return 0;
}

static int
parse_addr(const char *s, uint32_t *out)
{
    // Try hex first
    if (parse_hex(s, out) == 0) {
        return 0;
    }
    // Fall back to register name lookup
    return lookup_reg(s, out);
}

// Print a single color swatch using ANSI 24-bit color
static void
print_swatch(uint8_t r, uint8_t g, uint8_t b)
{
    printf("\x1b[48;2;%d;%d;%dm  \x1b[0m", r, g, b);
}

// Get bytes per pixel for current CRTC mode
static uint32_t
get_bytes_per_pixel(ati_device_t *dev)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width =
        (crtc_gen_cntl & CRTC_PIX_WIDTH_MASK) >> CRTC_PIX_WIDTH_SHIFT;

    switch (pix_width) {
    case CRTC_PIX_WIDTH_8BPP:
        return 1;
    case CRTC_PIX_WIDTH_15BPP:
    case CRTC_PIX_WIDTH_16BPP:
        return 2;
    case CRTC_PIX_WIDTH_24BPP:
        return 3;
    case CRTC_PIX_WIDTH_32BPP:
        return 4;
    default:
        return 1;
    }
}

// Print pixel value and color swatch based on current pixel format
static void
print_pixel(ati_device_t *dev, uint32_t pixel_idx)
{
    uint32_t crtc_gen_cntl = rd_crtc_gen_cntl(dev);
    uint32_t pix_width =
        (crtc_gen_cntl & CRTC_PIX_WIDTH_MASK) >> CRTC_PIX_WIDTH_SHIFT;
    uint32_t bpp = get_bytes_per_pixel(dev);
    uint32_t byte_offset = pixel_idx * bpp;
    uint32_t val = ati_vram_read(dev, byte_offset & ~3); // Align to dword
    uint32_t shift = (byte_offset & 3) * 8;

    switch (pix_width) {
    case CRTC_PIX_WIDTH_32BPP: {
        // aRGB 8888: 0xAARRGGBB
        uint8_t r = (val >> 16) & 0xff;
        uint8_t g = (val >> 8) & 0xff;
        uint8_t b = val & 0xff;
        printf("0x%08x ", val);
        print_swatch(r, g, b);
        break;
    }
    case CRTC_PIX_WIDTH_16BPP: {
        // RGB 565
        uint16_t pixel = (val >> shift) & 0xffff;
        uint8_t r = ((pixel >> 11) & 0x1f) << 3;
        uint8_t g = ((pixel >> 5) & 0x3f) << 2;
        uint8_t b = (pixel & 0x1f) << 3;
        printf("0x%04x ", pixel);
        print_swatch(r, g, b);
        break;
    }
    case CRTC_PIX_WIDTH_15BPP: {
        // aRGB 1555
        uint16_t pixel = (val >> shift) & 0xffff;
        uint8_t r = ((pixel >> 10) & 0x1f) << 3;
        uint8_t g = ((pixel >> 5) & 0x1f) << 3;
        uint8_t b = (pixel & 0x1f) << 3;
        printf("0x%04x ", pixel);
        print_swatch(r, g, b);
        break;
    }
    default:
        // Unsupported format - just print raw value
        printf("0x%08x", val);
        break;
    }
}

void
repl(ati_device_t *dev)
{
    char buf[64];
    char *cmd, *arg1, *arg2, *arg3, *p;

    // clang-format off
    printf("\n\nTests complete.\n");
    printf("Commands: reboot                     - reboot system (baremetal)\n");
    printf("          r <addr|reg_name>          - register read\n");
    printf("          w <addr|reg_name> <val>    - register write\n");
    printf("          vr <offset> [count]        - vram read\n");
    printf("          vw <offset> <val> [count]  - vram write\n");
    printf("          pr <pixel> [count]         - pixel read\n");
    printf("          pw <pixel> <val> [count]   - pixel write\n");
    printf("          mr <addr> [count]          - system memory read\n");
    printf("> ");
    fflush(stdout);
    // clang-format on

    while (fgets(buf, sizeof(buf), stdin)) {
        // Strip trailing newline
        for (p = buf; *p; p++) {
            if (*p == '\n' || *p == '\r') {
                *p = '\0';
                break;
            }
        }

        // Parse command and arguments
        p = buf;
        while (*p == ' ' || *p == '\t')
            p++;
        cmd = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        while (*p == ' ' || *p == '\t')
            p++;
        arg1 = *p ? p : NULL;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        while (*p == ' ' || *p == '\t')
            p++;
        arg2 = *p ? p : NULL;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        while (*p == ' ' || *p == '\t')
            p++;
        arg3 = *p ? p : NULL;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        if (*p)
            *p++ = '\0';

        if (strcmp(cmd, "reboot") == 0) {
            printf("Rebooting...\n");
            platform_reboot();
        } else if (strcmp(cmd, "r") == 0) {
            uint32_t addr;
            if (arg1 && parse_addr(arg1, &addr) == 0) {
                uint32_t val = ati_reg_read(dev, addr);
                printf("0x%04x = 0x%08x\n", addr, val);
            } else {
                printf("Usage: r <addr|reg_name>\n");
            }
        } else if (strcmp(cmd, "w") == 0) {
            uint32_t addr, val;
            if (arg1 && arg2 && parse_addr(arg1, &addr) == 0 &&
                parse_hex(arg2, &val) == 0) {
                ati_reg_write(dev, addr, val);
                printf("0x%04x <- 0x%08x\n", addr, val);
            } else {
                printf("Usage: w <addr|reg_name> <val>\n");
            }
        } else if (strcmp(cmd, "vr") == 0) {
            uint32_t offset, count = 1;
            if (arg1 && parse_hex(arg1, &offset) == 0) {
                if (arg2)
                    parse_hex(arg2, &count);
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t addr = offset + i * 4;
                    uint32_t val = ati_vram_read(dev, addr);
                    printf("0x%08x: 0x%08x\n", addr, val);
                }
            } else {
                printf("Usage: vr <offset> [count]\n");
            }
        } else if (strcmp(cmd, "vw") == 0) {
            uint32_t offset, val, count = 1;
            if (arg1 && arg2 && parse_hex(arg1, &offset) == 0 &&
                parse_hex(arg2, &val) == 0) {
                if (arg3)
                    parse_hex(arg3, &count);
                for (uint32_t i = 0; i < count; i++) {
                    ati_vram_write(dev, offset + i * 4, val);
                }
                printf("0x%08x <- 0x%08x (x%d)\n", offset, val, count);
            } else {
                printf("Usage: vw <offset> <val> [count]\n");
            }
        } else if (strcmp(cmd, "pr") == 0) {
            uint32_t pixel, count = 1;
            if (arg1 && parse_int(arg1, &pixel) == 0) {
                if (arg2)
                    parse_int(arg2, &count);
                for (uint32_t i = 0; i < count; i++) {
                    printf("pixel %d: ", pixel + i);
                    print_pixel(dev, pixel + i);
                    printf("\n");
                }
            } else {
                printf("Usage: pr <pixel> [count]\n");
            }
        } else if (strcmp(cmd, "pw") == 0) {
            uint32_t pixel, val, count = 1;
            if (arg1 && arg2 && parse_int(arg1, &pixel) == 0 &&
                parse_hex(arg2, &val) == 0) {
                if (arg3)
                    parse_int(arg3, &count);
                uint32_t bpp = get_bytes_per_pixel(dev);
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t byte_offset = (pixel + i) * bpp;
                    // Write pixel value at appropriate size
                    if (bpp == 4) {
                        ati_vram_write(dev, byte_offset, val);
                    } else if (bpp == 2) {
                        uint32_t aligned = byte_offset & ~3;
                        uint32_t shift = (byte_offset & 2) * 8;
                        uint32_t mask = 0xffff << shift;
                        uint32_t cur = ati_vram_read(dev, aligned);
                        cur = (cur & ~mask) | ((val & 0xffff) << shift);
                        ati_vram_write(dev, aligned, cur);
                    } else if (bpp == 1) {
                        uint32_t aligned = byte_offset & ~3;
                        uint32_t shift = (byte_offset & 3) * 8;
                        uint32_t mask = 0xff << shift;
                        uint32_t cur = ati_vram_read(dev, aligned);
                        cur = (cur & ~mask) | ((val & 0xff) << shift);
                        ati_vram_write(dev, aligned, cur);
                    }
                }
                printf("pixel %d <- 0x%x (x%d)\n", pixel, val, count);
            } else {
                printf("Usage: pw <pixel> <val> [count]\n");
            }
        } else if (strcmp(cmd, "mr") == 0) {
            uint32_t addr, count = 1;
            if (arg1 && parse_hex(arg1, &addr) == 0) {
                if (arg2)
                    parse_hex(arg2, &count);
                for (uint32_t i = 0; i < count; i++) {
                    volatile uint32_t *ptr =
                        (volatile uint32_t *) (uintptr_t) (addr + i * 4);
                    printf("0x%08x: 0x%08x\n", addr + i * 4, *ptr);
                }
            } else {
                printf("Usage: mr <addr> [count]\n");
            }
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
        }

        printf("> ");
        fflush(stdout);
    }
}
