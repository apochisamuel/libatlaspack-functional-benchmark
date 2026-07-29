#include "internal.h"

atlas_status atlas_parse_edges(atlas_context *context,
                               const uint8_t *data,
                               size_t size,
                               const atlas_section *section,
                               atlas_document *document) {
    (void)size;
    if (section == NULL) {
        document->edges = NULL;
        document->edge_count = 0;
        return ATLAS_OK;
    }
    atlas_reader reader;
    atlas_reader_init(&reader, data + (size_t)section->offset, (size_t)section->size);
    uint32_t count;
    if (!atlas_reader_u32(&reader, &count)) return ATLAS_ERR_TRUNCATED;
    if (count > ATLAS_MAX_EDGES) return ATLAS_ERR_RANGE;
    atlas_edge *edges = atlas_calloc(context, count, sizeof(*edges));
    if (edges == NULL && count != 0) return ATLAS_ERR_NOMEM;
    for (uint32_t i = 0; i < count; ++i) {
        if (!atlas_reader_u32(&reader, &edges[i].source_id) ||
            !atlas_reader_u32(&reader, &edges[i].target_id) ||
            !atlas_reader_u32(&reader, &edges[i].label_index)) {
            free(edges);
            atlas_set_error(context, "edge section truncated at entry %u", i);
            return ATLAS_ERR_TRUNCATED;
        }
    }
    document->edges = edges;
    document->edge_count = count;
    return ATLAS_OK;
}
