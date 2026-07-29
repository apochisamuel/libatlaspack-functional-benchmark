#include "internal.h"

static atlas_status atlas_read_entire_file(atlas_context *context,
                                           const char *path,
                                           uint8_t **out_data,
                                           size_t *out_size) {
    if (context == NULL || path == NULL || out_data == NULL || out_size == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_data = NULL;
    *out_size = 0;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        atlas_set_error(context, "unable to open '%s' for reading", path);
        return ATLAS_ERR_IO;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        atlas_set_error(context, "unable to seek to the end of '%s'", path);
        return ATLAS_ERR_IO;
    }
    long end = ftell(file);
    if (end < 0) {
        fclose(file);
        atlas_set_error(context, "unable to determine the size of '%s'", path);
        return ATLAS_ERR_IO;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        atlas_set_error(context, "unable to rewind '%s'", path);
        return ATLAS_ERR_IO;
    }
    size_t size = (size_t)end;
    uint8_t *data = (uint8_t *)malloc(size == 0 ? 1 : size);
    if (data == NULL) {
        fclose(file);
        atlas_set_error(context, "unable to allocate %zu bytes for '%s'", size, path);
        return ATLAS_ERR_NOMEM;
    }
    if (size != 0 && fread(data, 1, size, file) != size) {
        free(data);
        fclose(file);
        atlas_set_error(context, "unable to read all bytes from '%s'", path);
        return ATLAS_ERR_IO;
    }
    if (fclose(file) != 0) {
        free(data);
        atlas_set_error(context, "unable to close '%s' after reading", path);
        return ATLAS_ERR_IO;
    }
    *out_data = data;
    *out_size = size;
    return ATLAS_OK;
}

static atlas_status atlas_write_entire_file(atlas_context *context,
                                            const char *path,
                                            const uint8_t *data,
                                            size_t size) {
    if (context == NULL || path == NULL || (data == NULL && size != 0)) {
        return ATLAS_ERR_ARGUMENT;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        atlas_set_error(context, "unable to open '%s' for writing", path);
        return ATLAS_ERR_IO;
    }
    if (size != 0 && fwrite(data, 1, size, file) != size) {
        fclose(file);
        atlas_set_error(context, "unable to write all bytes to '%s'", path);
        return ATLAS_ERR_IO;
    }
    if (fflush(file) != 0) {
        fclose(file);
        atlas_set_error(context, "unable to flush '%s'", path);
        return ATLAS_ERR_IO;
    }
    if (fclose(file) != 0) {
        atlas_set_error(context, "unable to close '%s' after writing", path);
        return ATLAS_ERR_IO;
    }
    return ATLAS_OK;
}

atlas_status atlas_parse_file(atlas_context *context,
                              const char *path,
                              const atlas_parse_options *options,
                              atlas_document **out_document) {
    if (context == NULL || path == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    uint8_t *data = NULL;
    size_t size = 0;
    atlas_status status = atlas_read_entire_file(context, path, &data, &size);
    if (status == ATLAS_OK) {
        status = atlas_parse(context, data, size, options, out_document);
    }
    free(data);
    return status;
}

atlas_status atlas_builder_write_file(atlas_context *context,
                                      const atlas_builder *builder,
                                      const char *path) {
    if (context == NULL || builder == NULL || path == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    uint8_t *data = NULL;
    size_t size = 0;
    atlas_status status = atlas_builder_serialize(context, builder, &data, &size);
    if (status == ATLAS_OK) {
        status = atlas_write_entire_file(context, path, data, size);
    }
    free(data);
    return status;
}

atlas_status atlas_document_write_file(atlas_context *context,
                                       const atlas_document *document,
                                       const char *path) {
    if (context == NULL || document == NULL || path == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    uint8_t *data = NULL;
    size_t size = 0;
    atlas_status status = atlas_document_serialize(context, document, &data, &size);
    if (status == ATLAS_OK) {
        status = atlas_write_entire_file(context, path, data, size);
    }
    free(data);
    return status;
}
