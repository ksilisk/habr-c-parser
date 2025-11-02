#ifndef HTML_SCAN_H
#define HTML_SCAN_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    TOKEN_START_TAG,
    TOKEN_END_TAG,
    TOKEN_TEXT
} token_type_t;

typedef struct {
    token_type_t type;
    char tag[64];
    char attrs[1024];
    char text[2048];
} token_t;

typedef void (*token_callback_t)(const token_t *token, void *user_data);

typedef struct {
    token_callback_t callback;
    void *user_data;
    bool capture_text;

    enum {
        STATE_TEXT,
        STATE_TAG_OPEN,
        STATE_TAG_NAME,
        STATE_TAG_REST,
        STATE_END_TAG_REST,
        STATE_COMMENT_START,
        STATE_COMMENT,
        STATE_SKIP_DECL
    } state;

    char tag_buf[64];
    size_t tag_len;

    char attrs_buf[1024];
    size_t attrs_len;

    char text_buf[2048];
    size_t text_len;

    bool closing_tag;
    bool self_closing;
    char quote_char;
    int comment_dash_count;
} html_scanner_t;

void html_scanner_init(html_scanner_t *scanner, token_callback_t callback, void *user_data);
void html_scanner_set_capture_text(html_scanner_t *scanner, bool capture);
void html_scanner_feed(html_scanner_t *scanner, const char *data, size_t len, bool final_chunk);
void html_scanner_finish(html_scanner_t *scanner);

#endif
