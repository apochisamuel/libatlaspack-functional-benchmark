#include "internal.h"

atlas_status atlas_parse_metadata(atlas_context *context,
                                  const uint8_t *data,
                                  size_t size,
                                  const atlas_section *section,
                                  atlas_document *document) {
    (void)size;
    if (section == NULL) {
        document->metadata = NULL;
        document->metadata_count = 0;
        return ATLAS_OK;
    }
    atlas_reader reader;
    atlas_reader_init(&reader, data + (size_t)section->offset, (size_t)section->size);
    uint32_t count = 0;
    if (!atlas_reader_u32(&reader, &count)) {
        atlas_set_error(context, "metadata section is missing its entry count");
        return ATLAS_ERR_TRUNCATED;
    }
    if (count > ATLAS_MAX_METADATA) {
        atlas_set_error(context, "metadata count %u exceeds the supported limit", count);
        return ATLAS_ERR_RANGE;
    }
    atlas_metadata *metadata = atlas_calloc(context, count, sizeof(*metadata));
    if (metadata == NULL && count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    for (uint32_t index = 0; index < count; ++index) {
        if (!atlas_reader_u32(&reader, &metadata[index].key_index) ||
            !atlas_reader_u32(&reader, &metadata[index].value_index)) {
            free(metadata);
            atlas_set_error(context, "metadata section truncated at entry %u", index);
            return ATLAS_ERR_TRUNCATED;
        }
    }
    if (atlas_reader_remaining(&reader) != 0) {
        free(metadata);
        atlas_set_error(context, "metadata section has %zu trailing bytes",
                        atlas_reader_remaining(&reader));
        return ATLAS_ERR_FORMAT;
    }
    document->metadata = metadata;
    document->metadata_count = count;
    return ATLAS_OK;
}

size_t atlas_document_metadata_count(const atlas_document *document) {
    return document == NULL ? 0 : document->metadata_count;
}

atlas_status atlas_document_metadata_at(const atlas_document *document,
                                        size_t index,
                                        atlas_metadata_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= document->metadata_count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_metadata *entry = &document->metadata[index];
    const atlas_string *key = &document->strings[entry->key_index];
    const atlas_string *value = &document->strings[entry->value_index];
    out_view->key = key->bytes;
    out_view->key_length = key->length;
    out_view->value = value->bytes;
    out_view->value_length = value->length;
    return ATLAS_OK;
}

atlas_status atlas_document_find_metadata(const atlas_document *document,
                                          const char *key,
                                          atlas_metadata_view *out_view) {
    if (document == NULL || key == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t key_length = strlen(key);
    for (size_t index = 0; index < document->metadata_count; ++index) {
        const atlas_metadata *entry = &document->metadata[index];
        const atlas_string *candidate = &document->strings[entry->key_index];
        if (candidate->length == key_length &&
            memcmp(candidate->bytes, key, key_length) == 0) {
            return atlas_document_metadata_at(document, index, out_view);
        }
    }
    return ATLAS_ERR_NOT_FOUND;
}
