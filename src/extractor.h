#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <stddef.h>
#include <stdbool.h>

#include "html_scan.h"
#include "csv_writer.h"

#define ARTICLE_TITLE_CAP 1024
#define ARTICLE_URL_CAP 1024
#define ARTICLE_DATE_CAP 32
#define ARTICLE_AUTHOR_CAP 256
#define ARTICLE_TAGS_CAP 1024
#define ARTICLE_TAG_TEXT_CAP 512

typedef struct article {
    char title[ARTICLE_TITLE_CAP];
    char url[ARTICLE_URL_CAP];
    char date[ARTICLE_DATE_CAP];
    char author[ARTICLE_AUTHOR_CAP];
    char tags[ARTICLE_TAGS_CAP];
} article_t;

typedef struct {
    csv_writer_t *writer;
    size_t limit;
    size_t count;
    bool done;

    bool in_article;
    int article_depth;

    bool in_title_link;
    int title_depth;

    bool in_author_link;
    int author_depth;

    bool in_hubs;
    int hubs_depth;

    bool in_tag_link;
    int tag_link_depth;

    article_t current;
    char current_tag_text[ARTICLE_TAG_TEXT_CAP];
} extractor_t;

void extractor_init(extractor_t *ext, csv_writer_t *writer, size_t limit);
void extractor_consume_html(extractor_t *ext, const char *html, size_t len);
void extractor_process_token(extractor_t *ext, const token_t *token);
size_t extractor_get_count(const extractor_t *ext);
bool extractor_is_done(const extractor_t *ext);

#endif
