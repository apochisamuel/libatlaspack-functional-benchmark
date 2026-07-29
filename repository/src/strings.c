#include "internal.h"

atlas_status atlas_parse_strings(atlas_context *context,
                                 const uint8_t *data,
                                 size_t size,
                                 const atlas_section *section,
                                 atlas_document *document) {
    (void)size;
    if (section == NULL) {
        atlas_set_error(context, "missing string section");
        return ATLAS_ERR_FORMAT;
    }
    atlas_reader reader;
    atlas_reader_init(&reader, data + (size_t)section->offset, (size_t)section->size);
    uint32_t count = 0;
    if (!atlas_reader_u32(&reader, &count)) {
        atlas_set_error(context, "string section is missing its entry count");
        return ATLAS_ERR_TRUNCATED;
    }
    if (count > ATLAS_MAX_STRINGS) {
        atlas_set_error(context, "string count %u exceeds the supported limit", count);
        return ATLAS_ERR_RANGE;
    }
    atlas_string *strings = atlas_calloc(context, count, sizeof(*strings));
    if (strings == NULL && count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    for (uint32_t index = 0; index < count; ++index) {
        uint32_t length = 0;
        const uint8_t *bytes = NULL;
        if (!atlas_reader_u32(&reader, &length) ||
            !atlas_reader_bytes(&reader, &bytes, length)) {
            for (uint32_t previous = 0; previous < index; ++previous) {
                free(strings[previous].bytes);
            }
            free(strings);
            atlas_set_error(context, "string table truncated at entry %u", index);
            return ATLAS_ERR_TRUNCATED;
        }
        if (!atlas_utf8_valid(bytes, length)) {
            for (uint32_t previous = 0; previous < index; ++previous) {
                free(strings[previous].bytes);
            }
            free(strings);
            atlas_set_error(context, "string %u is not valid UTF-8", index);
            return ATLAS_ERR_FORMAT;
        }
        strings[index].bytes = atlas_strdup_n(context, (const char *)bytes, length);
        if (strings[index].bytes == NULL) {
            for (uint32_t previous = 0; previous < index; ++previous) {
                free(strings[previous].bytes);
            }
            free(strings);
            return ATLAS_ERR_NOMEM;
        }
        strings[index].length = length;
        strings[index].hash = atlas_hash_bytes(bytes, length);
    }
    if (atlas_reader_remaining(&reader) != 0) {
        for (uint32_t index = 0; index < count; ++index) {
            free(strings[index].bytes);
        }
        free(strings);
        atlas_set_error(context, "string section has %zu trailing bytes",
                        atlas_reader_remaining(&reader));
        return ATLAS_ERR_FORMAT;
    }
    document->strings = strings;
    document->string_count = count;
    return ATLAS_OK;
}
