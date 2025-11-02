#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <stdio.h>

struct article;

typedef struct {
    FILE *out;
} csv_writer_t;

void csv_writer_init(csv_writer_t *writer, FILE *out);
void csv_writer_write_header(csv_writer_t *writer);
void csv_writer_write(csv_writer_t *writer, const struct article *article);

#endif
