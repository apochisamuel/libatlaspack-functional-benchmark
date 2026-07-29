#include "internal.h"

size_t atlas_document_out_degree(const atlas_document *document, uint32_t node_id) {
    size_t index = atlas_node_index_by_id(document, node_id);
    return index == ATLAS_NO_INDEX ? 0 : document->nodes[index].outgoing_count;
}

size_t atlas_document_in_degree(const atlas_document *document, uint32_t node_id) {
    size_t index = atlas_node_index_by_id(document, node_id);
    return index == ATLAS_NO_INDEX ? 0 : document->nodes[index].incoming_count;
}

atlas_status atlas_document_outgoing_edge_at(const atlas_document *document,
                                             uint32_t node_id,
                                             size_t ordinal,
                                             atlas_edge_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    const atlas_node *node = &document->nodes[node_index];
    if (ordinal >= node->outgoing_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
    return atlas_document_edge_at(document, edge_index, out_view);
}

atlas_status atlas_document_incoming_edge_at(const atlas_document *document,
                                             uint32_t node_id,
                                             size_t ordinal,
                                             atlas_edge_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    const atlas_node *node = &document->nodes[node_index];
    if (ordinal >= node->incoming_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t edge_index = document->incoming_order[node->incoming_start + ordinal];
    return atlas_document_edge_at(document, edge_index, out_view);
}

atlas_status atlas_document_node_attribute_at(const atlas_document *document,
                                              uint32_t node_id,
                                              size_t ordinal,
                                              atlas_attribute_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    const atlas_node *node = &document->nodes[node_index];
    if (ordinal >= node->attribute_count) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_attribute_at(document,
                                       node->first_attribute + ordinal,
                                       out_view);
}
