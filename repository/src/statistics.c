#include "internal.h"

static int atlas_edges_equal(const atlas_document *document,
                             const atlas_edge *left,
                             const atlas_edge *right) {
    if (left->source_index != right->source_index ||
        left->target_index != right->target_index) {
        return 0;
    }
    const atlas_string *left_label = &document->strings[left->label_index];
    const atlas_string *right_label = &document->strings[right->label_index];
    return left_label->length == right_label->length &&
           memcmp(left_label->bytes, right_label->bytes, left_label->length) == 0;
}

atlas_status atlas_document_statistics(const atlas_document *document,
                                       atlas_statistics *out_statistics) {
    if (document == NULL || out_statistics == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    memset(out_statistics, 0, sizeof(*out_statistics));
    out_statistics->node_count = document->node_count;
    out_statistics->edge_count = document->edge_count;
    out_statistics->attribute_count = document->attribute_count;
    out_statistics->metadata_count = document->metadata_count;
    for (size_t node_index = 0; node_index < document->node_count; ++node_index) {
        const atlas_node *node = &document->nodes[node_index];
        if (node->incoming_count == 0) {
            ++out_statistics->source_count;
        }
        if (node->outgoing_count == 0) {
            ++out_statistics->sink_count;
        }
        if (node->incoming_count == 0 && node->outgoing_count == 0) {
            ++out_statistics->isolated_count;
        }
        if (node->outgoing_count > out_statistics->maximum_out_degree) {
            out_statistics->maximum_out_degree = node->outgoing_count;
        }
        if (node->incoming_count > out_statistics->maximum_in_degree) {
            out_statistics->maximum_in_degree = node->incoming_count;
        }
    }
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        if (edge->source_index == edge->target_index) {
            ++out_statistics->self_loop_count;
        }
        for (size_t previous = 0; previous < edge_index; ++previous) {
            if (atlas_edges_equal(document, edge, &document->edges[previous])) {
                ++out_statistics->duplicate_edge_count;
                break;
            }
        }
    }
    if (document->node_count != 0) {
        out_statistics->average_out_degree =
            (double)document->edge_count / (double)document->node_count;
    }
    if (document->node_count > 1) {
        double possible = (double)document->node_count *
                          (double)(document->node_count - 1);
        out_statistics->density = (double)document->edge_count / possible;
    }
    return ATLAS_OK;
}
