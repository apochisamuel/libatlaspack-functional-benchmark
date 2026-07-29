#include "internal.h"

typedef enum atlas_edge_sort_key {
    ATLAS_EDGE_SORT_OUTGOING,
    ATLAS_EDGE_SORT_INCOMING
} atlas_edge_sort_key;

static int atlas_edge_index_less(const atlas_document *document,
                                 size_t left_index,
                                 size_t right_index,
                                 atlas_edge_sort_key key) {
    const atlas_edge *left = &document->edges[left_index];
    const atlas_edge *right = &document->edges[right_index];
    size_t left_primary = key == ATLAS_EDGE_SORT_OUTGOING
                              ? left->source_index
                              : left->target_index;
    size_t right_primary = key == ATLAS_EDGE_SORT_OUTGOING
                               ? right->source_index
                               : right->target_index;
    if (left_primary != right_primary) {
        return left_primary < right_primary;
    }
    size_t left_secondary = key == ATLAS_EDGE_SORT_OUTGOING
                                ? left->target_index
                                : left->source_index;
    size_t right_secondary = key == ATLAS_EDGE_SORT_OUTGOING
                                 ? right->target_index
                                 : right->source_index;
    if (left_secondary != right_secondary) {
        return left_secondary < right_secondary;
    }
    if (left->label_index != right->label_index) {
        return left->label_index < right->label_index;
    }
    return left_index < right_index;
}

static void atlas_stable_sort_edge_indices(size_t *indices,
                                           size_t count,
                                           const atlas_document *document,
                                           atlas_edge_sort_key key) {
    for (size_t index = 1; index < count; ++index) {
        size_t value = indices[index];
        size_t position = index;
        while (position > 0 &&
               atlas_edge_index_less(document, value, indices[position - 1], key)) {
            indices[position] = indices[position - 1];
            --position;
        }
        indices[position] = value;
    }
}

atlas_status atlas_build_graph(atlas_context *context, atlas_document *document) {
    size_t *outgoing = atlas_calloc(context, document->edge_count, sizeof(*outgoing));
    size_t *incoming = atlas_calloc(context, document->edge_count, sizeof(*incoming));
    if ((outgoing == NULL || incoming == NULL) && document->edge_count != 0) {
        free(outgoing);
        free(incoming);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->edge_count; ++index) {
        outgoing[index] = index;
        incoming[index] = index;
    }
    atlas_stable_sort_edge_indices(outgoing,
                                   document->edge_count,
                                   document,
                                   ATLAS_EDGE_SORT_OUTGOING);
    atlas_stable_sort_edge_indices(incoming,
                                   document->edge_count,
                                   document,
                                   ATLAS_EDGE_SORT_INCOMING);
    for (size_t index = 0; index < document->node_count; ++index) {
        atlas_node *node = &document->nodes[index];
        node->outgoing_start = 0;
        node->outgoing_count = 0;
        node->incoming_start = 0;
        node->incoming_count = 0;
    }
    for (size_t ordinal = 0; ordinal < document->edge_count; ++ordinal) {
        atlas_edge *edge = &document->edges[outgoing[ordinal]];
        atlas_node *node = &document->nodes[edge->source_index];
        if (node->outgoing_count == 0) {
            node->outgoing_start = ordinal;
        }
        ++node->outgoing_count;
    }
    for (size_t ordinal = 0; ordinal < document->edge_count; ++ordinal) {
        atlas_edge *edge = &document->edges[incoming[ordinal]];
        atlas_node *node = &document->nodes[edge->target_index];
        if (node->incoming_count == 0) {
            node->incoming_start = ordinal;
        }
        ++node->incoming_count;
    }
    document->edge_order = outgoing;
    document->incoming_order = incoming;
    return ATLAS_OK;
}

size_t atlas_edge_index_from_view(const atlas_document *document,
                                  const atlas_edge *edge) {
    if (document == NULL || edge == NULL || edge < document->edges ||
        edge >= document->edges + document->edge_count) {
        return ATLAS_NO_INDEX;
    }
    return (size_t)(edge - document->edges);
}
