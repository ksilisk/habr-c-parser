#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

typedef struct {
    char *data;
    size_t size;
} http_buffer_t;

int http_init(void);
void http_cleanup(void);
void http_buffer_init(http_buffer_t *buffer);
void http_buffer_free(http_buffer_t *buffer);
int http_get(const char *url, long timeout_seconds, int retries, http_buffer_t *buffer, long *status_code);

#endif
