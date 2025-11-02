#include "html_scan.h"

#include <ctype.h>
#include <string.h>

static void emit_text(html_scanner_t *scanner) {
    if (!scanner->capture_text || scanner->text_len == 0) {
        scanner->text_len = 0;
        return;
    }
    token_t token;
    memset(&token, 0, sizeof(token));
    token.type = TOKEN_TEXT;
    size_t len = scanner->text_len < sizeof(token.text) - 1 ? scanner->text_len : sizeof(token.text) - 1;
    memcpy(token.text, scanner->text_buf, len);
    token.text[len] = '\0';
    scanner->callback(&token, scanner->user_data);
    scanner->text_len = 0;
}

static void emit_start_tag(html_scanner_t *scanner) {
    token_t token;
    memset(&token, 0, sizeof(token));
    token.type = TOKEN_START_TAG;
    size_t len = scanner->tag_len < sizeof(token.tag) - 1 ? scanner->tag_len : sizeof(token.tag) - 1;
    memcpy(token.tag, scanner->tag_buf, len);
    token.tag[len] = '\0';
    size_t attr_len = scanner->attrs_len < sizeof(token.attrs) - 1 ? scanner->attrs_len : sizeof(token.attrs) - 1;
    memcpy(token.attrs, scanner->attrs_buf, attr_len);
    token.attrs[attr_len] = '\0';
    scanner->callback(&token, scanner->user_data);
    if (scanner->self_closing) {
        token.type = TOKEN_END_TAG;
        token.attrs[0] = '\0';
        scanner->callback(&token, scanner->user_data);
    }
}

static void emit_end_tag(html_scanner_t *scanner) {
    token_t token;
    memset(&token, 0, sizeof(token));
    token.type = TOKEN_END_TAG;
    size_t len = scanner->tag_len < sizeof(token.tag) - 1 ? scanner->tag_len : sizeof(token.tag) - 1;
    memcpy(token.tag, scanner->tag_buf, len);
    token.tag[len] = '\0';
    scanner->callback(&token, scanner->user_data);
}

static void reset_tag_buffers(html_scanner_t *scanner) {
    scanner->tag_len = 0;
    scanner->attrs_len = 0;
    scanner->attrs_buf[0] = '\0';
    scanner->tag_buf[0] = '\0';
    scanner->quote_char = '\0';
    scanner->self_closing = false;
}

void html_scanner_init(html_scanner_t *scanner, token_callback_t callback, void *user_data) {
    memset(scanner, 0, sizeof(*scanner));
    scanner->callback = callback;
    scanner->user_data = user_data;
    scanner->capture_text = true;
    scanner->state = STATE_TEXT;
}

void html_scanner_set_capture_text(html_scanner_t *scanner, bool capture) {
    if (scanner->capture_text && !capture) {
        emit_text(scanner);
    }
    scanner->capture_text = capture;
}

static void handle_comment_state(html_scanner_t *scanner, char c) {
    if (scanner->state == STATE_COMMENT_START) {
        if (c == '-' && scanner->comment_dash_count < 2) {
            scanner->comment_dash_count++;
            if (scanner->comment_dash_count == 2) {
                scanner->state = STATE_COMMENT;
            }
            return;
        }
        /* Not actually a comment, treat as declaration */
        scanner->state = STATE_SKIP_DECL;
    }
    if (scanner->state == STATE_COMMENT) {
        if (c == '-') {
            if (scanner->comment_dash_count < 2) {
                scanner->comment_dash_count++;
            }
        } else if (c == '>' && scanner->comment_dash_count >= 2) {
            scanner->state = STATE_TEXT;
            scanner->comment_dash_count = 0;
        } else {
            scanner->comment_dash_count = 0;
        }
        return;
    }
    if (scanner->state == STATE_SKIP_DECL) {
        if (c == '>') {
            scanner->state = STATE_TEXT;
        }
    }
}

static void finish_tag(html_scanner_t *scanner) {
    while (scanner->attrs_len > 0) {
        char ch = scanner->attrs_buf[scanner->attrs_len - 1];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            scanner->attrs_len--;
        } else if (ch == '/') {
            scanner->attrs_len--;
            scanner->self_closing = true;
        } else {
            break;
        }
    }
    scanner->attrs_buf[scanner->attrs_len] = '\0';
    if (scanner->closing_tag) {
        emit_end_tag(scanner);
    } else {
        emit_start_tag(scanner);
    }
    reset_tag_buffers(scanner);
    scanner->closing_tag = false;
    scanner->state = STATE_TEXT;
}

void html_scanner_feed(html_scanner_t *scanner, const char *data, size_t len, bool final_chunk) {
    const char *ptr = data;
    const char *end = data + len;
    while (ptr < end) {
        char c = *ptr++;
        switch (scanner->state) {
            case STATE_TEXT:
                if (c == '<') {
                    emit_text(scanner);
                    reset_tag_buffers(scanner);
                    scanner->closing_tag = false;
                    scanner->state = STATE_TAG_OPEN;
                } else if (scanner->capture_text) {
                    if (scanner->text_len + 1 < sizeof(scanner->text_buf)) {
                        scanner->text_buf[scanner->text_len++] = c;
                    }
                }
                break;
            case STATE_TAG_OPEN:
                if (c == '!') {
                    scanner->state = STATE_COMMENT_START;
                    scanner->comment_dash_count = 0;
                } else if (c == '/') {
                    scanner->closing_tag = true;
                    scanner->state = STATE_TAG_NAME;
                } else if (c == '?' ) {
                    scanner->state = STATE_SKIP_DECL;
                } else if (isspace((unsigned char)c)) {
                    /* ignore */
                } else {
                    scanner->closing_tag = false;
                    scanner->state = STATE_TAG_NAME;
                    ptr--; /* reprocess */
                }
                break;
            case STATE_TAG_NAME:
                if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == ':' ) {
                    if (scanner->tag_len + 1 < sizeof(scanner->tag_buf)) {
                        scanner->tag_buf[scanner->tag_len++] = (char)tolower((unsigned char)c);
                    }
                } else {
                    scanner->tag_buf[scanner->tag_len] = '\0';
                    if (scanner->closing_tag) {
                        scanner->state = STATE_END_TAG_REST;
                    } else {
                        scanner->state = STATE_TAG_REST;
                    }
                    ptr--;
                }
                break;
            case STATE_TAG_REST:
                if (scanner->quote_char) {
                    if (c == scanner->quote_char) {
                        if (scanner->attrs_len + 1 < sizeof(scanner->attrs_buf)) {
                            scanner->attrs_buf[scanner->attrs_len++] = c;
                        }
                        scanner->quote_char = '\0';
                    } else {
                        if (scanner->attrs_len + 1 < sizeof(scanner->attrs_buf)) {
                            scanner->attrs_buf[scanner->attrs_len++] = c;
                        }
                    }
                } else {
                    if (c == '"' || c == '\'') {
                        if (scanner->attrs_len + 1 < sizeof(scanner->attrs_buf)) {
                            scanner->attrs_buf[scanner->attrs_len++] = c;
                        }
                        scanner->quote_char = c;
                    } else if (c == '>') {
                        finish_tag(scanner);
                    } else if (c == '/') {
                        scanner->self_closing = true;
                    } else {
                        if (scanner->attrs_len + 1 < sizeof(scanner->attrs_buf)) {
                            scanner->attrs_buf[scanner->attrs_len++] = c;
                        }
                    }
                }
                break;
            case STATE_END_TAG_REST:
                if (c == '>') {
                    finish_tag(scanner);
                }
                break;
            case STATE_COMMENT_START:
            case STATE_COMMENT:
            case STATE_SKIP_DECL:
                handle_comment_state(scanner, c);
                break;
        }
    }
    if (final_chunk) {
        emit_text(scanner);
    }
}

void html_scanner_finish(html_scanner_t *scanner) {
    emit_text(scanner);
}

