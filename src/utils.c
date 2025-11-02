#include "utils.h"

#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static bool utils_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int utils_strncasecmp_local(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca == '\0' || cb == '\0') {
            return ca - cb;
        }
        int da = tolower(ca);
        int db = tolower(cb);
        if (da != db) {
            return da - db;
        }
    }
    return 0;
}

int utils_strcasecmp_local(const char *a, const char *b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    size_t min = la < lb ? la : lb;
    int cmp = utils_strncasecmp_local(a, b, min);
    if (cmp != 0) {
        return cmp;
    }
    return (int)((unsigned char)a[min]) - (int)((unsigned char)b[min]);
}

bool utils_strcasestr_bool(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return true;
    }
    for (const char *p = haystack; *p; ++p) {
        if (utils_strncasecmp_local(p, needle, needle_len) == 0) {
            return true;
        }
    }
    return false;
}

int utils_parse_attr(const char *attrs, const char *name, char *out, size_t out_cap) {
    if (!attrs || !name || !out || out_cap == 0) {
        return 0;
    }
    const char *p = attrs;
    size_t name_len = strlen(name);
    while (*p) {
        while (*p && utils_is_space(*p)) {
            ++p;
        }
        if (!*p || *p == '>') {
            break;
        }
        const char *key_start = p;
        while (*p && !utils_is_space(*p) && *p != '=' && *p != '>') {
            ++p;
        }
        size_t key_len = (size_t)(p - key_start);
        const char *after_key = p;
        while (*p && utils_is_space(*p)) {
            ++p;
        }
        if (*p != '=') {
            p = after_key;
            continue;
        }
        ++p;
        while (*p && utils_is_space(*p)) {
            ++p;
        }
        char quote = '\0';
        if (*p == '"' || *p == '\'') {
            quote = *p;
            ++p;
        }
        const char *value_start = p;
        while (*p) {
            if (quote) {
                if (*p == quote) {
                    break;
                }
            } else if (utils_is_space(*p) || *p == '>') {
                break;
            }
            ++p;
        }
        const char *value_end = p;
        if (quote && *p == quote) {
            ++p;
        }
        if (key_len == name_len && utils_strncasecmp_local(key_start, name, name_len) == 0) {
            size_t len = (size_t)(value_end - value_start);
            if (len >= out_cap) {
                len = out_cap - 1;
            }
            memcpy(out, value_start, len);
            out[len] = '\0';
            return 1;
        }
    }
    return 0;
}

bool utils_class_contains(const char *attrs, const char *needle) {
    if (!attrs || !needle) {
        return false;
    }
    char class_buf[512];
    if (!utils_parse_attr(attrs, "class", class_buf, sizeof(class_buf))) {
        return false;
    }
    return strstr(class_buf, needle) != NULL;
}

void utils_trim(char *str) {
    if (!str || *str == '\0') {
        return;
    }
    char *start = str;
    while (*start && utils_is_space(*start)) {
        ++start;
    }
    char *end = start + strlen(start);
    while (end > start && utils_is_space(*(end - 1))) {
        --end;
    }
    size_t len = (size_t)(end - start);
    if (start != str) {
        memmove(str, start, len);
    }
    str[len] = '\0';
}

void utils_normalize_whitespace(char *str) {
    if (!str) {
        return;
    }
    char *dst = str;
    bool in_space = false;
    for (char *src = str; *src; ++src) {
        if (utils_is_space(*src)) {
            if (!in_space) {
                *dst++ = ' ';
                in_space = true;
            }
        } else {
            *dst++ = *src;
            in_space = false;
        }
    }
    *dst = '\0';
    utils_trim(str);
}

void utils_replace_newlines_with_space(char *str) {
    if (!str) {
        return;
    }
    for (char *p = str; *p; ++p) {
        if (*p == '\n' || *p == '\r') {
            *p = ' ';
        }
    }
}

bool utils_safe_append_char(char *dest, size_t cap, char ch) {
    size_t len = strlen(dest);
    if (len + 1 >= cap) {
        return false;
    }
    dest[len] = ch;
    dest[len + 1] = '\0';
    return true;
}

bool utils_safe_append(char *dest, size_t cap, const char *src) {
    if (!dest || !src) {
        return false;
    }
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    if (dest_len + src_len >= cap) {
        src_len = cap - dest_len - 1;
    }
    memcpy(dest + dest_len, src, src_len);
    dest[dest_len + src_len] = '\0';
    return dest_len + src_len < cap - 1;
}

bool utils_copy_string(char *dest, size_t cap, const char *src) {
    if (!dest || cap == 0) {
        return false;
    }
    size_t len = strlen(src);
    if (len >= cap) {
        len = cap - 1;
    }
    memcpy(dest, src, len);
    dest[len] = '\0';
    return len < cap - 1;
}

void utils_sleep_ms(long ms) {
    if (ms <= 0) {
        return;
    }
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

bool utils_urlencode(const char *input, char *output, size_t cap) {
    if (!input || !output || cap == 0) {
        return false;
    }
    size_t out_index = 0;
    for (const unsigned char *p = (const unsigned char *)input; *p; ++p) {
        unsigned char ch = *p;
        bool safe = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                    (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' ||
                    ch == '.' || ch == '~';
        if (safe) {
            if (out_index + 1 >= cap) {
                return false;
            }
            output[out_index++] = (char)ch;
        } else {
            if (out_index + 3 >= cap) {
                return false;
            }
            snprintf(output + out_index, cap - out_index, "%%%02X", ch);
            out_index += 3;
        }
    }
    if (out_index >= cap) {
        return false;
    }
    output[out_index] = '\0';
    return true;
}

void utils_make_absolute_url(const char *href, char *out, size_t cap) {
    if (!href || !out || cap == 0) {
        return;
    }
    if (strncmp(href, "http://", 7) == 0 || strncmp(href, "https://", 8) == 0) {
        utils_copy_string(out, cap, href);
        return;
    }
    if (href[0] == '/') {
        const char *prefix = "https://habr.com";
        size_t needed = strlen(prefix) + strlen(href) + 1;
        if (needed > cap) {
            utils_copy_string(out, cap, "https://habr.com");
            size_t remain = cap - strlen(out) - 1;
            strncat(out, href, remain);
        } else {
            snprintf(out, cap, "%s%s", prefix, href);
        }
    } else {
        utils_copy_string(out, cap, href);
    }
}

void utils_replace_char(char *str, char from, char to) {
    if (!str) {
        return;
    }
    for (char *p = str; *p; ++p) {
        if (*p == from) {
            *p = to;
        }
    }
}

