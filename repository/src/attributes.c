#include "internal.h"

atlas_status atlas_parse_attributes(atlas_context *context,
                                    const uint8_t *data,
                                    size_t size,
                                    const atlas_section *section,
                                    atlas_document *document) {
    (void)size;
    if (section == NULL) {
        document->attributes = NULL;
        document->attribute_count = 0;
        return ATLAS_OK;
    }
    atlas_reader reader;
    atlas_reader_init(&reader, data + (size_t)section->offset, (size_t)section->size);
    uint32_t count;
    if (!atlas_reader_u32(&reader, &count)) return ATLAS_ERR_TRUNCATED;
    if (count > ATLAS_MAX_ATTRIBUTES) return ATLAS_ERR_RANGE;
    atlas_attribute *attributes = atlas_calloc(context, count, sizeof(*attributes));
    if (attributes == NULL && count != 0) return ATLAS_ERR_NOMEM;
    for (uint32_t i = 0; i < count; ++i) {
        if (!atlas_reader_u32(&reader, &attributes[i].node_id) ||
            !atlas_reader_u32(&reader, &attributes[i].key_index) ||
            !atlas_reader_u32(&reader, &attributes[i].value_index)) {
            free(attributes);
            atlas_set_error(context, "attribute section truncated at entry %u", i);
            return ATLAS_ERR_TRUNCATED;
        }
    }
    if (atlas_reader_remaining(&reader) != 0) {
        free(attributes);
        atlas_set_error(context,
                        "attribute section has %zu trailing bytes",
                        atlas_reader_remaining(&reader));
        return ATLAS_ERR_FORMAT;
    }
    document->attributes = attributes;
    document->attribute_count = count;
    return ATLAS_OK;
}
