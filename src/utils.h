#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>

int utils_parse_attr(const char *attrs, const char *name, char *out, size_t out_cap);
bool utils_class_contains(const char *attrs, const char *needle);
void utils_trim(char *str);
void utils_normalize_whitespace(char *str);
void utils_replace_newlines_with_space(char *str);
bool utils_safe_append(char *dest, size_t cap, const char *src);
bool utils_safe_append_char(char *dest, size_t cap, char ch);
bool utils_copy_string(char *dest, size_t cap, const char *src);
int utils_strcasecmp_local(const char *a, const char *b);
int utils_strncasecmp_local(const char *a, const char *b, size_t n);
bool utils_strcasestr_bool(const char *haystack, const char *needle);
void utils_sleep_ms(long ms);
bool utils_urlencode(const char *input, char *output, size_t cap);
void utils_make_absolute_url(const char *href, char *out, size_t cap);
void utils_replace_char(char *str, char from, char to);

#endif
