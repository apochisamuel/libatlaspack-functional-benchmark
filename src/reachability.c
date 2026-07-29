#include "internal.h"

static atlas_status atlas_mark_reachable(const atlas_document *document,
                                         size_t start_index,
                                         int reverse,
                                         unsigned char *visited,
                                         size_t *out_count) {
    size_t *queue = (size_t *)calloc(document->node_count, sizeof(*queue));
    if (queue == NULL && document->node_count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    size_t head = 0;
    size_t tail = 0;
    visited[start_index] = 1;
    queue[tail++] = start_index;
    size_t count = 0;
    while (head < tail) {
        size_t node_index = queue[head++];
        const atlas_node *node = &document->nodes[node_index];
        size_t edge_count = reverse ? node->incoming_count : node->outgoing_count;
        size_t edge_start = reverse ? node->incoming_start : node->outgoing_start;
        const size_t *order = reverse ? document->incoming_order : document->edge_order;
        for (size_t ordinal = 0; ordinal < edge_count; ++ordinal) {
            size_t edge_index = order[edge_start + ordinal];
            const atlas_edge *edge = &document->edges[edge_index];
            size_t neighbor = reverse ? edge->source_index : edge->target_index;
            if (!visited[neighbor]) {
                visited[neighbor] = 1;
                queue[tail++] = neighbor;
                ++count;
            }
        }
    }
    free(queue);
    if (out_count != NULL) {
        *out_count = count;
    }
    return ATLAS_OK;
}

atlas_status atlas_document_reachable(const atlas_document *document,
                                      uint32_t source_node_id,
                                      uint32_t target_node_id,
                                      int *out_reachable) {
    if (document == NULL || out_reachable == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t source_index = atlas_node_index_by_id(document, source_node_id);
    size_t target_index = atlas_node_index_by_id(document, target_node_id);
    if (source_index == ATLAS_NO_INDEX || target_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    unsigned char *visited = (unsigned char *)calloc(document->node_count,
                                                     sizeof(*visited));
    if (visited == NULL && document->node_count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = atlas_mark_reachable(document,
                                               source_index,
                                               0,
                                               visited,
                                               NULL);
    if (status == ATLAS_OK) {
        *out_reachable = visited[target_index] != 0;
    }
    free(visited);
    return status;
}

atlas_status atlas_document_descendant_count(const atlas_document *document,
                                             uint32_t source_node_id,
                                             size_t *out_count) {
    if (document == NULL || out_count == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t source_index = atlas_node_index_by_id(document, source_node_id);
    if (source_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    unsigned char *visited = (unsigned char *)calloc(document->node_count,
                                                     sizeof(*visited));
    if (visited == NULL && document->node_count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = atlas_mark_reachable(document,
                                               source_index,
                                               0,
                                               visited,
                                               out_count);
    free(visited);
    return status;
}

atlas_status atlas_document_ancestor_count(const atlas_document *document,
                                           uint32_t target_node_id,
                                           size_t *out_count) {
    if (document == NULL || out_count == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t target_index = atlas_node_index_by_id(document, target_node_id);
    if (target_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    unsigned char *visited = (unsigned char *)calloc(document->node_count,
                                                     sizeof(*visited));
    if (visited == NULL && document->node_count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = atlas_mark_reachable(document,
                                               target_index,
                                               1,
                                               visited,
                                               out_count);
    free(visited);
    return status;
}

atlas_status atlas_document_longest_path_length(atlas_context *context,
                                                const atlas_document *document,
                                                size_t *out_length) {
    if (context == NULL || document == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_plan *plan = NULL;
    atlas_status status = atlas_plan_build(context, document, &plan);
    if (status != ATLAS_OK) {
        return status;
    }
    size_t *distance = atlas_calloc(context,
                                    document->node_count,
                                    sizeof(*distance));
    if (distance == NULL && document->node_count != 0) {
        atlas_plan_destroy(plan);
        return ATLAS_ERR_NOMEM;
    }
    size_t maximum = 0;
    for (size_t order_index = 0; order_index < plan->count; ++order_index) {
        size_t node_index = plan->node_order[order_index];
        const atlas_node *node = &document->nodes[node_index];
        for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
            size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
            size_t target_index = document->edges[edge_index].target_index;
            size_t candidate = distance[node_index] + 1;
            if (candidate > distance[target_index]) {
                distance[target_index] = candidate;
                if (candidate > maximum) {
                    maximum = candidate;
                }
            }
        }
    }
    free(distance);
    atlas_plan_destroy(plan);
    *out_length = maximum;
    return ATLAS_OK;
}
