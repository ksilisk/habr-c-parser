#include "entities.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int decode_numeric(const char *entity) {
    if (entity[1] == 'x' || entity[1] == 'X') {
        return (int)strtol(entity + 2, NULL, 16);
    }
    return (int)strtol(entity + 1, NULL, 10);
}

void entities_decode_inplace(char *str) {
    if (!str) {
        return;
    }
    char *dst = str;
    for (char *src = str; *src; ) {
        if (*src == '&') {
            char *semi = strchr(src, ';');
            if (!semi || (semi - src) > 10) {
                *dst++ = *src++;
                continue;
            }
            size_t len = (size_t)(semi - src - 1);
            if (len == 0) {
                *dst++ = *src++;
                continue;
            }
            if (strncmp(src, "&amp;", 5) == 0) {
                *dst++ = '&';
                src = semi + 1;
                continue;
            }
            if (strncmp(src, "&lt;", 4) == 0) {
                *dst++ = '<';
                src = semi + 1;
                continue;
            }
            if (strncmp(src, "&gt;", 4) == 0) {
                *dst++ = '>';
                src = semi + 1;
                continue;
            }
            if (strncmp(src, "&nbsp;", 6) == 0) {
                *dst++ = ' ';
                src = semi + 1;
                continue;
            }
            if (src[1] == '#') {
                int value = decode_numeric(src + 1);
                if (value <= 0) {
                    src = semi + 1;
                    continue;
                }
                if (value < 0x80) {
                    *dst++ = (char)value;
                } else if (value < 0x800) {
                    *dst++ = (char)(0xC0 | (value >> 6));
                    *dst++ = (char)(0x80 | (value & 0x3F));
                } else if (value < 0x10000) {
                    *dst++ = (char)(0xE0 | (value >> 12));
                    *dst++ = (char)(0x80 | ((value >> 6) & 0x3F));
                    *dst++ = (char)(0x80 | (value & 0x3F));
                } else {
                    *dst++ = '?';
                }
                src = semi + 1;
                continue;
            }
            /* Unknown entity: keep as-is */
            while (src <= semi) {
                *dst++ = *src++;
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

