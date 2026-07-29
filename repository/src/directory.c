#include "internal.h"

static int known_type(uint32_t type) {
    return type == ATLAS_SECTION_STRINGS || type == ATLAS_SECTION_NODES ||
           type == ATLAS_SECTION_EDGES || type == ATLAS_SECTION_ATTRIBUTES ||
           type == ATLAS_SECTION_METADATA;
}

static int compare_sections(const void *a, const void *b) {
    const atlas_section *left = (const atlas_section *)a;
    const atlas_section *right = (const atlas_section *)b;
    if (left->offset < right->offset) return -1;
    if (left->offset > right->offset) return 1;
    if (left->size < right->size) return -1;
    if (left->size > right->size) return 1;
    return 0;
}

atlas_status atlas_parse_directory(atlas_context *context,
                                   const uint8_t *data,
                                   size_t size,
                                   const atlas_header *header,
                                   atlas_section **out_sections,
                                   size_t *out_count,
                                   int allow_unknown) {
    if (header->directory_offset > size) {
        atlas_set_error(context, "directory offset outside file");
        return ATLAS_ERR_RANGE;
    }
    uint64_t directory_bytes = (uint64_t)header->section_count * ATLAS_DIRECTORY_ENTRY_SIZE;
    if (directory_bytes > size - (size_t)header->directory_offset) {
        atlas_set_error(context, "directory exceeds file bounds");
        return ATLAS_ERR_TRUNCATED;
    }
    atlas_section *sections = atlas_calloc(context, header->section_count, sizeof(*sections));
    if (sections == NULL) return ATLAS_ERR_NOMEM;
    atlas_reader reader;
    atlas_reader_init(&reader, data, size);
    atlas_reader_seek(&reader, (size_t)header->directory_offset);
    for (uint32_t i = 0; i < header->section_count; ++i) {
        atlas_section *section = &sections[i];
        if (!atlas_reader_u32(&reader, &section->type) ||
            !atlas_reader_u32(&reader, &section->flags) ||
            !atlas_reader_u64(&reader, &section->offset) ||
            !atlas_reader_u64(&reader, &section->size) ||
            !atlas_reader_u64(&reader, &section->aux)) {
            free(sections);
            atlas_set_error(context, "directory entry %u is truncated", i);
            return ATLAS_ERR_TRUNCATED;
        }
        if (!allow_unknown && !known_type(section->type)) {
            uint32_t unknown_type = section->type;
            free(sections);
            atlas_set_error(context, "unknown section type %u", unknown_type);
            return ATLAS_ERR_FORMAT;
        }
        if (section->offset > size || section->size > size - (size_t)section->offset) {
            free(sections);
            atlas_set_error(context, "section %u exceeds file bounds", i);
            return ATLAS_ERR_RANGE;
        }
    }
    qsort(sections, header->section_count, sizeof(*sections), compare_sections);
    for (uint32_t i = 1; i < header->section_count; ++i) {
        uint64_t previous_end = sections[i - 1].offset + sections[i - 1].size;
        if (previous_end > sections[i].offset) {
            free(sections);
            atlas_set_error(context, "sections overlap");
            return ATLAS_ERR_FORMAT;
        }
    }
    *out_sections = sections;
    *out_count = header->section_count;
    return ATLAS_OK;
}

const atlas_section *atlas_find_section(const atlas_document *document, uint32_t type) {
    for (size_t i = 0; i < document->section_count; ++i) {
        if (document->sections[i].type == type) return &document->sections[i];
    }
    return NULL;
}
