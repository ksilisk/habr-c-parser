#include "csv_writer.h"

#include "extractor.h"

static void csv_escape_and_print(FILE *out, const char *text) {
    fputc('"', out);
    for (const char *p = text; *p; ++p) {
        if (*p == '"') {
            fputc('"', out);
            fputc('"', out);
        } else {
            fputc(*p, out);
        }
    }
    fputc('"', out);
}

void csv_writer_init(csv_writer_t *writer, FILE *out) {
    writer->out = out;
}

void csv_writer_write_header(csv_writer_t *writer) {
    if (!writer || !writer->out) {
        return;
    }
    fprintf(writer->out, "title,url,date,author,tags\n");
}

void csv_writer_write(csv_writer_t *writer, const struct article *article) {
    if (!writer || !writer->out || !article) {
        return;
    }
    csv_escape_and_print(writer->out, article->title);
    fputc(',', writer->out);
    csv_escape_and_print(writer->out, article->url);
    fputc(',', writer->out);
    csv_escape_and_print(writer->out, article->date);
    fputc(',', writer->out);
    csv_escape_and_print(writer->out, article->author);
    fputc(',', writer->out);
    csv_escape_and_print(writer->out, article->tags);
    fputc('\n', writer->out);
}

