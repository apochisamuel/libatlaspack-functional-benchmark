#include "internal.h"

static atlas_status atlas_writer_grow(atlas_context *context,
                                      atlas_writer *writer,
                                      size_t required) {
    size_t capacity = writer->capacity == 0 ? 128 : writer->capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            atlas_set_error(context, "writer capacity overflow");
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    uint8_t *replacement = (uint8_t *)realloc(writer->data, capacity);
    if (replacement == NULL) {
        atlas_set_error(context, "unable to grow binary writer to %zu bytes", capacity);
        return ATLAS_ERR_NOMEM;
    }
    writer->data = replacement;
    writer->capacity = capacity;
    return ATLAS_OK;
}

void atlas_writer_init(atlas_writer *writer) {
    if (writer == NULL) {
        return;
    }
    writer->data = NULL;
    writer->size = 0;
    writer->capacity = 0;
}

void atlas_writer_dispose(atlas_writer *writer) {
    if (writer == NULL) {
        return;
    }
    free(writer->data);
    writer->data = NULL;
    writer->size = 0;
    writer->capacity = 0;
}

atlas_status atlas_writer_reserve(atlas_context *context,
                                  atlas_writer *writer,
                                  size_t additional) {
    if (writer == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (additional > SIZE_MAX - writer->size) {
        atlas_set_error(context, "writer size overflow");
        return ATLAS_ERR_RANGE;
    }
    size_t required = writer->size + additional;
    if (required <= writer->capacity) {
        return ATLAS_OK;
    }
    return atlas_writer_grow(context, writer, required);
}

atlas_status atlas_writer_u8(atlas_context *context,
                             atlas_writer *writer,
                             uint8_t value) {
    atlas_status status = atlas_writer_reserve(context, writer, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    writer->data[writer->size++] = value;
    return ATLAS_OK;
}

atlas_status atlas_writer_u16(atlas_context *context,
                              atlas_writer *writer,
                              uint16_t value) {
    atlas_status status = atlas_writer_reserve(context, writer, 2);
    if (status != ATLAS_OK) {
        return status;
    }
    writer->data[writer->size++] = (uint8_t)value;
    writer->data[writer->size++] = (uint8_t)(value >> 8);
    return ATLAS_OK;
}

atlas_status atlas_writer_u32(atlas_context *context,
                              atlas_writer *writer,
                              uint32_t value) {
    atlas_status status = atlas_writer_reserve(context, writer, 4);
    if (status != ATLAS_OK) {
        return status;
    }
    for (unsigned shift = 0; shift < 32; shift += 8) {
        writer->data[writer->size++] = (uint8_t)(value >> shift);
    }
    return ATLAS_OK;
}

atlas_status atlas_writer_u64(atlas_context *context,
                              atlas_writer *writer,
                              uint64_t value) {
    atlas_status status = atlas_writer_reserve(context, writer, 8);
    if (status != ATLAS_OK) {
        return status;
    }
    for (unsigned shift = 0; shift < 64; shift += 8) {
        writer->data[writer->size++] = (uint8_t)(value >> shift);
    }
    return ATLAS_OK;
}

atlas_status atlas_writer_bytes(atlas_context *context,
                                atlas_writer *writer,
                                const void *data,
                                size_t size) {
    if (data == NULL && size != 0) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_status status = atlas_writer_reserve(context, writer, size);
    if (status != ATLAS_OK) {
        return status;
    }
    if (size != 0) {
        memcpy(writer->data + writer->size, data, size);
        writer->size += size;
    }
    return ATLAS_OK;
}

atlas_status atlas_writer_patch_u32(atlas_writer *writer,
                                    size_t offset,
                                    uint32_t value) {
    if (writer == NULL || offset > writer->size || writer->size - offset < 4) {
        return ATLAS_ERR_RANGE;
    }
    for (unsigned shift = 0; shift < 32; shift += 8) {
        writer->data[offset + shift / 8] = (uint8_t)(value >> shift);
    }
    return ATLAS_OK;
}

atlas_status atlas_writer_patch_u64(atlas_writer *writer,
                                    size_t offset,
                                    uint64_t value) {
    if (writer == NULL || offset > writer->size || writer->size - offset < 8) {
        return ATLAS_ERR_RANGE;
    }
    for (unsigned shift = 0; shift < 64; shift += 8) {
        writer->data[offset + shift / 8] = (uint8_t)(value >> shift);
    }
    return ATLAS_OK;
}

uint8_t *atlas_writer_take(atlas_writer *writer, size_t *out_size) {
    if (writer == NULL || out_size == NULL) {
        return NULL;
    }
    uint8_t *data = writer->data;
    *out_size = writer->size;
    writer->data = NULL;
    writer->size = 0;
    writer->capacity = 0;
    return data;
}
