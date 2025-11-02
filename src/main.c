#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csv_writer.h"
#include "extractor.h"
#include "http.h"
#include "utils.h"

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s --input <file.html>\n"
            "  %s -q <query> [--max N] [--delay-ms D] [--timeout T] [--lang en|ru]\n",
            prog, prog);
}

static int read_file(const char *path, char **out_data, size_t *out_size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long length = ftell(fp);
    if (length < 0) {
        fclose(fp);
        return -1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    char *buffer = (char *)malloc((size_t)length + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }
    size_t read = fread(buffer, 1, (size_t)length, fp);
    fclose(fp);
    buffer[read] = '\0';
    if (out_data) {
        *out_data = buffer;
    }
    if (out_size) {
        *out_size = read;
    }
    return 0;
}

static int run_fixture_mode(extractor_t *extractor, const char *path) {
    char *data = NULL;
    size_t size = 0;
    if (read_file(path, &data, &size) != 0) {
        return 1;
    }
    extractor_consume_html(extractor, data, size);
    free(data);
    return 0;
}

static int run_search_mode(extractor_t *extractor, const char *query, int max_articles, int delay_ms,
                           long timeout_seconds, const char *lang) {
    if (http_init() != 0) {
        fprintf(stderr, "Failed to initialize HTTP layer\n");
        return 1;
    }

    int exit_code = 0;
    const char *base = NULL;
    if (strcmp(lang, "ru") == 0) {
        base = "https://habr.com/ru/search/";
    } else if (strcmp(lang, "en") == 0) {
        base = "https://habr.com/en/search/";
    } else {
        fprintf(stderr, "Unsupported language: %s\n", lang);
        http_cleanup();
        return 1;
    }

    size_t query_len = strlen(query);
    size_t encoded_cap = query_len * 3 + 1;
    char *encoded = (char *)malloc(encoded_cap);
    if (!encoded || !utils_urlencode(query, encoded, encoded_cap)) {
        fprintf(stderr, "Failed to encode query\n");
        free(encoded);
        http_cleanup();
        return 1;
    }

    http_buffer_t buffer;
    http_buffer_init(&buffer);

    for (int page = 1; !extractor_is_done(extractor) && extractor_get_count(extractor) < (size_t)max_articles;
         ++page) {
        char url[2048];
        snprintf(url, sizeof(url), "%s?q=%s&target_type=posts&order=relevance&page=%d", base, encoded, page);
        long status = 0;
        if (http_get(url, timeout_seconds, 3, &buffer, &status) != 0) {
            fprintf(stderr, "HTTP request failed for %s\n", url);
            exit_code = 1;
            break;
        }
        size_t before = extractor_get_count(extractor);
        extractor_consume_html(extractor, buffer.data ? buffer.data : "", buffer.size);
        size_t after = extractor_get_count(extractor);
        if (after == before) {
            break;
        }
        if (extractor_is_done(extractor)) {
            break;
        }
        utils_sleep_ms(delay_ms);
    }

    http_buffer_free(&buffer);
    free(encoded);
    http_cleanup();
    return exit_code;
}

int main(int argc, char **argv) {
    const char *input_path = "tests/fixtures/habr_example.html";
    const char *query = NULL;
    int max_articles = 100;
    int delay_ms = 300;
    long timeout_seconds = 15;
    const char *lang = "en";

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--input") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            input_path = argv[++i];
        } else if (strcmp(arg, "-q") == 0 || strcmp(arg, "--query") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            query = argv[++i];
        } else if (strcmp(arg, "--max") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            max_articles = (int)strtol(argv[++i], NULL, 10);
            if (max_articles <= 0) {
                fprintf(stderr, "--max must be positive\n");
                return 1;
            }
        } else if (strcmp(arg, "--delay-ms") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            delay_ms = (int)strtol(argv[++i], NULL, 10);
            if (delay_ms < 0) {
                fprintf(stderr, "--delay-ms must be non-negative\n");
                return 1;
            }
        } else if (strcmp(arg, "--timeout") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            timeout_seconds = strtol(argv[++i], NULL, 10);
            if (timeout_seconds <= 0) {
                fprintf(stderr, "--timeout must be positive\n");
                return 1;
            }
        } else if (strcmp(arg, "--lang") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            lang = argv[++i];
        } else {
            fprintf(stderr, "Unknown argument: %s\n", arg);
            print_usage(argv[0]);
            return 1;
        }
    }

    csv_writer_t writer;
    csv_writer_init(&writer, stdout);
    csv_writer_write_header(&writer);

    extractor_t extractor;
    extractor_init(&extractor, &writer, query ? (size_t)max_articles : 0);

    if (query) {
        return run_search_mode(&extractor, query, max_articles, delay_ms, timeout_seconds, lang);
    }
    return run_fixture_mode(&extractor, input_path);
}

