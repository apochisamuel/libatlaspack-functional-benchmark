#include "internal.h"

atlas_status atlas_parse_header(atlas_context *context, atlas_reader *reader, atlas_header *header) {
    uint32_t magic;
    if (!atlas_reader_u32(reader, &magic) ||
        !atlas_reader_u16(reader, &header->version) ||
        !atlas_reader_u16(reader, &header->flags) ||
        !atlas_reader_u32(reader, &header->section_count) ||
        !atlas_reader_u64(reader, &header->directory_offset) ||
        !atlas_reader_u64(reader, &header->declared_size) ||
        !atlas_reader_u32(reader, &header->checksum)) {
        atlas_set_error(context, "header is truncated");
        return ATLAS_ERR_TRUNCATED;
    }
    if (magic != ATLAS_MAGIC) {
        atlas_set_error(context, "invalid ATPK magic");
        return ATLAS_ERR_MAGIC;
    }
    if (header->version != ATLAS_VERSION) {
        atlas_set_error(context, "unsupported ATPK version %u", (unsigned)header->version);
        return ATLAS_ERR_VERSION;
    }
    if (header->section_count == 0 || header->section_count > ATLAS_MAX_SECTIONS) {
        atlas_set_error(context, "invalid section count %u", header->section_count);
        return ATLAS_ERR_RANGE;
    }
    return ATLAS_OK;
}
