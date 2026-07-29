#include "internal.h"

static atlas_status atlas_document_degree_histogram(
    atlas_context *context,
    const atlas_document *document,
    int incoming,
    atlas_degree_bucket **out_buckets,
    size_t *out_count) {
    if (context == NULL || document == NULL || out_buckets == NULL || out_count == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_buckets = NULL;
    *out_count = 0;
    size_t maximum = 0;
    for (size_t index = 0; index < document->node_count; ++index) {
        size_t degree = incoming ? document->nodes[index].incoming_count
                                 : document->nodes[index].outgoing_count;
        if (degree > maximum) {
            maximum = degree;
        }
    }
    if (maximum == SIZE_MAX) {
        return ATLAS_ERR_RANGE;
    }
    size_t slot_count = maximum + 1;
    size_t *counts = atlas_calloc(context, slot_count, sizeof(*counts));
    if (counts == NULL && slot_count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        size_t degree = incoming ? document->nodes[index].incoming_count
                                 : document->nodes[index].outgoing_count;
        ++counts[degree];
    }
    size_t nonempty = 0;
    for (size_t degree = 0; degree < slot_count; ++degree) {
        if (counts[degree] != 0) {
            ++nonempty;
        }
    }
    atlas_degree_bucket *buckets = atlas_calloc(context,
                                                nonempty,
                                                sizeof(*buckets));
    if (buckets == NULL && nonempty != 0) {
        free(counts);
        return ATLAS_ERR_NOMEM;
    }
    size_t cursor = 0;
    for (size_t degree = 0; degree < slot_count; ++degree) {
        if (counts[degree] == 0) {
            continue;
        }
        buckets[cursor].degree = degree;
        buckets[cursor].node_count = counts[degree];
        ++cursor;
    }
    free(counts);
    *out_buckets = buckets;
    *out_count = nonempty;
    return ATLAS_OK;
}

atlas_status atlas_document_out_degree_histogram(
    atlas_context *context,
    const atlas_document *document,
    atlas_degree_bucket **out_buckets,
    size_t *out_count) {
    return atlas_document_degree_histogram(context,
                                           document,
                                           0,
                                           out_buckets,
                                           out_count);
}

atlas_status atlas_document_in_degree_histogram(
    atlas_context *context,
    const atlas_document *document,
    atlas_degree_bucket **out_buckets,
    size_t *out_count) {
    return atlas_document_degree_histogram(context,
                                           document,
                                           1,
                                           out_buckets,
                                           out_count);
}
