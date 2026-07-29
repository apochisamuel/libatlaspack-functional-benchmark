#include "atlas.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned char *read_file(const char *path, size_t *size_out) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) { fclose(file); return NULL; }
    unsigned char *data = malloc((size_t)length);
    if (!data && length != 0) { fclose(file); return NULL; }
    if (fread(data, 1, (size_t)length, file) != (size_t)length) { free(data); fclose(file); return NULL; }
    fclose(file);
    *size_out = (size_t)length;
    return data;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE.atpk\n", argv[0]);
        return 2;
    }
    size_t size = 0;
    unsigned char *data = read_file(argv[1], &size);
    if (!data) { perror("read"); return 1; }
    atlas_context *context = atlas_context_create();
    atlas_document *document = NULL;
    atlas_status status = atlas_parse(context, data, size, NULL, &document);
    free(data);
    if (status != ATLAS_OK) {
        fprintf(stderr, "%s: %s\n", atlas_status_name(status), atlas_context_error(context));
        atlas_context_destroy(context);
        return 1;
    }
    char *report = NULL;
    size_t length = 0;
    status = atlas_document_report(context, document, &report, &length);
    if (status == ATLAS_OK) fwrite(report, 1, length, stdout), fputc('\n', stdout);
    else fprintf(stderr, "%s\n", atlas_context_error(context));
    atlas_free(report);
    atlas_document_destroy(document);
    atlas_context_destroy(context);
    return status == ATLAS_OK ? 0 : 1;
}
