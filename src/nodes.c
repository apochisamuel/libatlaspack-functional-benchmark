#include "internal.h"

atlas_status atlas_parse_nodes(atlas_context *context,
                               const uint8_t *data,
                               size_t size,
                               const atlas_section *section,
                               atlas_document *document) {
    (void)size;
    if (section == NULL) {
        atlas_set_error(context, "missing node section");
        return ATLAS_ERR_FORMAT;
    }
    atlas_reader reader;
    atlas_reader_init(&reader, data + (size_t)section->offset, (size_t)section->size);
    uint32_t count;
    if (!atlas_reader_u32(&reader, &count)) return ATLAS_ERR_TRUNCATED;
    if (count > ATLAS_MAX_NODES) return ATLAS_ERR_RANGE;
    atlas_node *nodes = atlas_calloc(context, count, sizeof(*nodes));
    if (nodes == NULL && count != 0) return ATLAS_ERR_NOMEM;
    for (uint32_t i = 0; i < count; ++i) {
        if (!atlas_reader_u32(&reader, &nodes[i].id) ||
            !atlas_reader_u32(&reader, &nodes[i].name_index) ||
            !atlas_reader_u32(&reader, &nodes[i].first_attribute) ||
            !atlas_reader_u32(&reader, &nodes[i].attribute_count)) {
            free(nodes);
            atlas_set_error(context, "node section truncated at entry %u", i);
            return ATLAS_ERR_TRUNCATED;
        }
    }
    document->nodes = nodes;
    document->node_count = count;
    return ATLAS_OK;
}
