#include "internal.h"

static atlas_status atlas_text_builder_grow(atlas_context *context,
                                            atlas_text_builder *builder,
                                            size_t required) {
    size_t capacity = builder->capacity == 0 ? 256 : builder->capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            atlas_set_error(context, "text capacity overflow");
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    char *replacement = (char *)realloc(builder->data, capacity);
    if (replacement == NULL) {
        atlas_set_error(context, "unable to grow text buffer to %zu bytes", capacity);
        return ATLAS_ERR_NOMEM;
    }
    builder->data = replacement;
    builder->capacity = capacity;
    return ATLAS_OK;
}

void atlas_text_builder_init(atlas_text_builder *builder) {
    if (builder == NULL) {
        return;
    }
    builder->data = NULL;
    builder->size = 0;
    builder->capacity = 0;
}

void atlas_text_builder_dispose(atlas_text_builder *builder) {
    if (builder == NULL) {
        return;
    }
    free(builder->data);
    builder->data = NULL;
    builder->size = 0;
    builder->capacity = 0;
}

atlas_status atlas_text_builder_reserve(atlas_context *context,
                                        atlas_text_builder *builder,
                                        size_t additional) {
    if (builder == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (additional > SIZE_MAX - builder->size - 1) {
        atlas_set_error(context, "text size overflow");
        return ATLAS_ERR_RANGE;
    }
    size_t required = builder->size + additional + 1;
    if (required <= builder->capacity) {
        return ATLAS_OK;
    }
    return atlas_text_builder_grow(context, builder, required);
}

atlas_status atlas_text_builder_append_n(atlas_context *context,
                                         atlas_text_builder *builder,
                                         const char *text,
                                         size_t length) {
    if (text == NULL && length != 0) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_status status = atlas_text_builder_reserve(context, builder, length);
    if (status != ATLAS_OK) {
        return status;
    }
    if (length != 0) {
        memcpy(builder->data + builder->size, text, length);
        builder->size += length;
    }
    builder->data[builder->size] = '\0';
    return ATLAS_OK;
}

atlas_status atlas_text_builder_append(atlas_context *context,
                                       atlas_text_builder *builder,
                                       const char *text) {
    if (text == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    return atlas_text_builder_append_n(context, builder, text, strlen(text));
}

atlas_status atlas_text_builder_append_char(atlas_context *context,
                                            atlas_text_builder *builder,
                                            char value) {
    return atlas_text_builder_append_n(context, builder, &value, 1);
}

atlas_status atlas_text_builder_printf(atlas_context *context,
                                       atlas_text_builder *builder,
                                       const char *format,
                                       ...) {
    if (builder == NULL || format == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    va_list first;
    va_start(first, format);
    va_list second;
    va_copy(second, first);
    int needed = vsnprintf(NULL, 0, format, first);
    va_end(first);
    if (needed < 0) {
        va_end(second);
        atlas_set_error(context, "text formatting failed");
        return ATLAS_ERR_FORMAT;
    }
    atlas_status status = atlas_text_builder_reserve(context, builder, (size_t)needed);
    if (status != ATLAS_OK) {
        va_end(second);
        return status;
    }
    int written = vsnprintf(builder->data + builder->size,
                            builder->capacity - builder->size,
                            format,
                            second);
    va_end(second);
    if (written != needed) {
        atlas_set_error(context, "text formatting produced an unexpected length");
        return ATLAS_ERR_FORMAT;
    }
    builder->size += (size_t)written;
    return ATLAS_OK;
}

atlas_status atlas_text_builder_json_string(atlas_context *context,
                                            atlas_text_builder *builder,
                                            const char *text,
                                            size_t length) {
    atlas_status status = atlas_text_builder_append_char(context, builder, '"');
    if (status != ATLAS_OK) {
        return status;
    }
    for (size_t index = 0; index < length; ++index) {
        unsigned char value = (unsigned char)text[index];
        switch (value) {
            case '"': status = atlas_text_builder_append(context, builder, "\\\""); break;
            case '\\': status = atlas_text_builder_append(context, builder, "\\\\"); break;
            case '\b': status = atlas_text_builder_append(context, builder, "\\b"); break;
            case '\f': status = atlas_text_builder_append(context, builder, "\\f"); break;
            case '\n': status = atlas_text_builder_append(context, builder, "\\n"); break;
            case '\r': status = atlas_text_builder_append(context, builder, "\\r"); break;
            case '\t': status = atlas_text_builder_append(context, builder, "\\t"); break;
            default:
                if (value < 0x20) {
                    status = atlas_text_builder_printf(context, builder, "\\u%04x", value);
                } else {
                    status = atlas_text_builder_append_char(context, builder, (char)value);
                }
                break;
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return atlas_text_builder_append_char(context, builder, '"');
}

atlas_status atlas_text_builder_csv_field(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const char *text,
                                          size_t length) {
    int quoted = 0;
    for (size_t index = 0; index < length; ++index) {
        if (text[index] == ',' || text[index] == '"' ||
            text[index] == '\r' || text[index] == '\n') {
            quoted = 1;
            break;
        }
    }
    if (!quoted) {
        return atlas_text_builder_append_n(context, builder, text, length);
    }
    atlas_status status = atlas_text_builder_append_char(context, builder, '"');
    if (status != ATLAS_OK) {
        return status;
    }
    for (size_t index = 0; index < length; ++index) {
        if (text[index] == '"') {
            status = atlas_text_builder_append(context, builder, "\"\"");
        } else {
            status = atlas_text_builder_append_char(context, builder, text[index]);
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return atlas_text_builder_append_char(context, builder, '"');
}

char *atlas_text_builder_take(atlas_text_builder *builder, size_t *out_size) {
    if (builder == NULL || out_size == NULL) {
        return NULL;
    }
    if (builder->data == NULL) {
        builder->data = (char *)malloc(1);
        if (builder->data == NULL) {
            return NULL;
        }
        builder->data[0] = '\0';
        builder->capacity = 1;
    }
    char *data = builder->data;
    *out_size = builder->size;
    builder->data = NULL;
    builder->size = 0;
    builder->capacity = 0;
    return data;
}
