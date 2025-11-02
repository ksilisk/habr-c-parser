#include "extractor.h"

#include <string.h>

#include "entities.h"
#include "utils.h"

static void reset_current(extractor_t *ext) {
    memset(&ext->current, 0, sizeof(ext->current));
    ext->current_tag_text[0] = '\0';
    ext->in_title_link = false;
    ext->title_depth = 0;
    ext->in_author_link = false;
    ext->author_depth = 0;
    ext->in_tag_link = false;
    ext->tag_link_depth = 0;
    ext->in_hubs = false;
    ext->hubs_depth = 0;
}

void extractor_init(extractor_t *ext, csv_writer_t *writer, size_t limit) {
    memset(ext, 0, sizeof(*ext));
    ext->writer = writer;
    ext->limit = limit;
}

size_t extractor_get_count(const extractor_t *ext) {
    return ext->count;
}

bool extractor_is_done(const extractor_t *ext) {
    return ext->done;
}

static void finalize_field(char *buffer) {
    utils_replace_newlines_with_space(buffer);
    utils_normalize_whitespace(buffer);
    entities_decode_inplace(buffer);
}

static void finalize_tag_text(extractor_t *ext) {
    char temp[ARTICLE_TAG_TEXT_CAP];
    utils_copy_string(temp, sizeof(temp), ext->current_tag_text);
    utils_replace_newlines_with_space(temp);
    utils_normalize_whitespace(temp);
    entities_decode_inplace(temp);
    if (temp[0] == '\0') {
        return;
    }
    if (ext->current.tags[0] != '\0') {
        utils_safe_append_char(ext->current.tags, sizeof(ext->current.tags), ';');
    }
    utils_safe_append(ext->current.tags, sizeof(ext->current.tags), temp);
    ext->current_tag_text[0] = '\0';
}

static void finish_article(extractor_t *ext) {
    if (ext->current.title[0] == '\0' || ext->current.url[0] == '\0') {
        reset_current(ext);
        ext->in_article = false;
        ext->article_depth = 0;
        return;
    }
    finalize_field(ext->current.title);
    finalize_field(ext->current.author);
    if (ext->current.tags[0] != '\0') {
        utils_replace_newlines_with_space(ext->current.tags);
    }
    if (ext->current.date[0] == '\0') {
        ext->current.date[0] = '\0';
    }
    if (ext->writer) {
        csv_writer_write(ext->writer, &ext->current);
    }
    ext->count++;
    if (ext->limit > 0 && ext->count >= ext->limit) {
        ext->done = true;
    }
    reset_current(ext);
    ext->in_article = false;
    ext->article_depth = 0;
}

static void handle_text(extractor_t *ext, const token_t *token) {
    if (!ext->in_article) {
        return;
    }
    if (ext->in_title_link) {
        utils_safe_append(ext->current.title, sizeof(ext->current.title), token->text);
    }
    if (ext->in_author_link) {
        utils_safe_append(ext->current.author, sizeof(ext->current.author), token->text);
    }
    if (ext->in_tag_link) {
        utils_safe_append(ext->current_tag_text, sizeof(ext->current_tag_text), token->text);
    }
}

static void handle_start(extractor_t *ext, const token_t *token) {
    if (ext->limit > 0 && ext->count >= ext->limit) {
        ext->done = true;
        return;
    }
    if (!ext->in_article) {
        if (strcmp(token->tag, "article") == 0 && utils_class_contains(token->attrs, "tm-articles-list__item")) {
            ext->in_article = true;
            ext->article_depth = 1;
            reset_current(ext);
        }
        return;
    }
    if (strcmp(token->tag, "article") == 0) {
        ext->article_depth++;
        return;
    }

    bool title_active_before = ext->in_title_link;
    bool author_active_before = ext->in_author_link;
    bool tag_active_before = ext->in_tag_link;

    if (strcmp(token->tag, "div") == 0) {
        if (!ext->in_hubs && utils_class_contains(token->attrs, "tm-publication-hubs")) {
            ext->in_hubs = true;
            ext->hubs_depth = 1;
        } else if (ext->in_hubs) {
            ext->hubs_depth++;
        }
    }

    if (strcmp(token->tag, "a") == 0) {
        if (utils_class_contains(token->attrs, "tm-title__link")) {
            ext->in_title_link = true;
            ext->title_depth = 1;
            char href[ARTICLE_URL_CAP];
            if (utils_parse_attr(token->attrs, "href", href, sizeof(href))) {
                utils_make_absolute_url(href, ext->current.url, sizeof(ext->current.url));
            }
        }
        if (utils_class_contains(token->attrs, "tm-user-info__username")) {
            ext->in_author_link = true;
            ext->author_depth = 1;
        }
        if (ext->in_hubs && utils_class_contains(token->attrs, "tm-publication-hub__link")) {
            ext->in_tag_link = true;
            ext->tag_link_depth = 1;
            ext->current_tag_text[0] = '\0';
        }
    }

    if (strcmp(token->tag, "time") == 0) {
        char datetime[128];
        if (utils_parse_attr(token->attrs, "datetime", datetime, sizeof(datetime))) {
            if (strlen(datetime) >= 10) {
                char temp[ARTICLE_DATE_CAP];
                size_t copy_len = 10;
                if (copy_len >= sizeof(temp)) {
                    copy_len = sizeof(temp) - 1;
                }
                memcpy(temp, datetime, copy_len);
                temp[copy_len] = '\0';
                utils_copy_string(ext->current.date, sizeof(ext->current.date), temp);
            }
        }
    }

    if (title_active_before && ext->in_title_link) {
        ext->title_depth++;
    }
    if (author_active_before && ext->in_author_link) {
        ext->author_depth++;
    }
    if (tag_active_before && ext->in_tag_link) {
        ext->tag_link_depth++;
    }
}

static void handle_end(extractor_t *ext, const token_t *token) {
    if (!ext->in_article) {
        return;
    }
    if (strcmp(token->tag, "article") == 0) {
        if (ext->article_depth > 0) {
            ext->article_depth--;
        }
        if (ext->article_depth == 0) {
            finish_article(ext);
        }
        return;
    }

    if (ext->in_title_link) {
        if (ext->title_depth > 0) {
            ext->title_depth--;
        }
        if (ext->title_depth == 0) {
            ext->in_title_link = false;
            finalize_field(ext->current.title);
        }
    }

    if (ext->in_author_link) {
        if (ext->author_depth > 0) {
            ext->author_depth--;
        }
        if (ext->author_depth == 0) {
            ext->in_author_link = false;
            finalize_field(ext->current.author);
        }
    }

    if (ext->in_tag_link) {
        if (ext->tag_link_depth > 0) {
            ext->tag_link_depth--;
        }
        if (ext->tag_link_depth == 0) {
            ext->in_tag_link = false;
            finalize_tag_text(ext);
        }
    }

    if (ext->in_hubs && strcmp(token->tag, "div") == 0) {
        if (ext->hubs_depth > 0) {
            ext->hubs_depth--;
        }
        if (ext->hubs_depth == 0) {
            ext->in_hubs = false;
        }
    }
}

void extractor_process_token(extractor_t *ext, const token_t *token) {
    if (ext->done) {
        return;
    }
    switch (token->type) {
        case TOKEN_START_TAG:
            handle_start(ext, token);
            break;
        case TOKEN_END_TAG:
            handle_end(ext, token);
            break;
        case TOKEN_TEXT:
            handle_text(ext, token);
            break;
    }
}

static void token_callback(const token_t *token, void *user_data) {
    extractor_t *ext = (extractor_t *)user_data;
    extractor_process_token(ext, token);
}

void extractor_consume_html(extractor_t *ext, const char *html, size_t len) {
    html_scanner_t scanner;
    html_scanner_init(&scanner, token_callback, ext);
    html_scanner_feed(&scanner, html, len, true);
    html_scanner_finish(&scanner);
}

