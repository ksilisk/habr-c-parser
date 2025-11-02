#include "http.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

int http_init(void) {
    return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK ? 0 : -1;
}

void http_cleanup(void) {
    curl_global_cleanup();
}

void http_buffer_init(http_buffer_t *buffer) {
    buffer->data = NULL;
    buffer->size = 0;
}

void http_buffer_free(http_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    http_buffer_t *buffer = (http_buffer_t *)userdata;
    char *new_data = (char *)realloc(buffer->data, buffer->size + total + 1);
    if (!new_data) {
        return 0;
    }
    buffer->data = new_data;
    memcpy(buffer->data + buffer->size, ptr, total);
    buffer->size += total;
    buffer->data[buffer->size] = '\0';
    return total;
}

int http_get(const char *url, long timeout_seconds, int retries, http_buffer_t *buffer, long *status_code) {
    if (!url || !buffer) {
        return -1;
    }
    CURL *curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    const long backoff_delays[] = {200, 600, 1200};
    int attempts = retries + 1;
    int attempt = 0;
    int result = -1;

    for (attempt = 0; attempt < attempts; ++attempt) {
        http_buffer_free(buffer);
        http_buffer_init(buffer);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "habr-c-parser/0.1");
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            if (attempt < retries) {
                utils_sleep_ms(backoff_delays[attempt < 3 ? attempt : 2]);
                continue;
            }
            break;
        }

        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (status_code) {
            *status_code = code;
        }
        if (code == 429 || code >= 500) {
            if (attempt < retries) {
                utils_sleep_ms(backoff_delays[attempt < 3 ? attempt : 2]);
                continue;
            }
            break;
        }
        if (code >= 400) {
            break;
        }
        if (!buffer->data) {
            buffer->data = (char *)malloc(1);
            if (!buffer->data) {
                break;
            }
            buffer->data[0] = '\0';
        }
        result = 0;
        break;
    }

    curl_easy_cleanup(curl);
    return result;
}

