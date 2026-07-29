#include "internal.h"

static atlas_status atlas_traversal_allocate(atlas_context *context,
                                             const atlas_document *document,
                                             atlas_traversal **out_traversal) {
    atlas_traversal *traversal = atlas_calloc(context, 1, sizeof(*traversal));
    if (traversal == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    traversal->node_indices = atlas_calloc(context,
                                           document->node_count,
                                           sizeof(*traversal->node_indices));
    if (traversal->node_indices == NULL && document->node_count != 0) {
        free(traversal);
        return ATLAS_ERR_NOMEM;
    }
    traversal->document = document;
    *out_traversal = traversal;
    return ATLAS_OK;
}

static atlas_status atlas_traversal_breadth_first(atlas_context *context,
                                                  const atlas_document *document,
                                                  size_t start_index,
                                                  atlas_traversal *traversal) {
    unsigned char *visited = atlas_calloc(context,
                                          document->node_count,
                                          sizeof(*visited));
    size_t *queue = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*queue));
    if ((visited == NULL || queue == NULL) && document->node_count != 0) {
        free(visited);
        free(queue);
        return ATLAS_ERR_NOMEM;
    }
    size_t head = 0;
    size_t tail = 0;
    visited[start_index] = 1;
    queue[tail++] = start_index;
    while (head < tail) {
        size_t node_index = queue[head++];
        traversal->node_indices[traversal->count++] = node_index;
        const atlas_node *node = &document->nodes[node_index];
        for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
            size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
            size_t target_index = document->edges[edge_index].target_index;
            if (!visited[target_index]) {
                visited[target_index] = 1;
                queue[tail++] = target_index;
            }
        }
    }
    free(visited);
    free(queue);
    return ATLAS_OK;
}

static atlas_status atlas_traversal_depth_first(atlas_context *context,
                                                const atlas_document *document,
                                                size_t start_index,
                                                atlas_traversal *traversal) {
    unsigned char *visited = atlas_calloc(context,
                                          document->node_count,
                                          sizeof(*visited));
    size_t *stack = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*stack));
    if ((visited == NULL || stack == NULL) && document->node_count != 0) {
        free(visited);
        free(stack);
        return ATLAS_ERR_NOMEM;
    }
    size_t stack_size = 0;
    stack[stack_size++] = start_index;
    while (stack_size != 0) {
        size_t node_index = stack[--stack_size];
        if (visited[node_index]) {
            continue;
        }
        visited[node_index] = 1;
        traversal->node_indices[traversal->count++] = node_index;
        const atlas_node *node = &document->nodes[node_index];
        for (size_t ordinal = node->outgoing_count; ordinal > 0; --ordinal) {
            size_t edge_index = document->edge_order[node->outgoing_start + ordinal - 1];
            size_t target_index = document->edges[edge_index].target_index;
            if (!visited[target_index]) {
                stack[stack_size++] = target_index;
            }
        }
    }
    free(visited);
    free(stack);
    return ATLAS_OK;
}

atlas_status atlas_traversal_build(atlas_context *context,
                                   const atlas_document *document,
                                   uint32_t start_node_id,
                                   atlas_visit_order order,
                                   atlas_traversal **out_traversal) {
    if (context == NULL || document == NULL || out_traversal == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_traversal = NULL;
    size_t start_index = atlas_node_index_by_id(document, start_node_id);
    if (start_index == ATLAS_NO_INDEX) {
        atlas_set_error(context, "start node %u does not exist", start_node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    if (order != ATLAS_VISIT_BREADTH_FIRST && order != ATLAS_VISIT_DEPTH_FIRST) {
        atlas_set_error(context, "unknown traversal order %d", (int)order);
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_traversal *traversal = NULL;
    atlas_status status = atlas_traversal_allocate(context, document, &traversal);
    if (status != ATLAS_OK) {
        return status;
    }
    if (order == ATLAS_VISIT_BREADTH_FIRST) {
        status = atlas_traversal_breadth_first(context,
                                               document,
                                               start_index,
                                               traversal);
    } else {
        status = atlas_traversal_depth_first(context,
                                             document,
                                             start_index,
                                             traversal);
    }
    if (status != ATLAS_OK) {
        atlas_traversal_destroy(traversal);
        return status;
    }
    *out_traversal = traversal;
    return ATLAS_OK;
}

void atlas_traversal_destroy(atlas_traversal *traversal) {
    if (traversal == NULL) {
        return;
    }
    free(traversal->node_indices);
    free(traversal);
}

size_t atlas_traversal_count(const atlas_traversal *traversal) {
    return traversal == NULL ? 0 : traversal->count;
}

atlas_status atlas_traversal_node_at(const atlas_traversal *traversal,
                                     size_t index,
                                     atlas_node_view *out_view) {
    if (traversal == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= traversal->count) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_node_at(traversal->document,
                                  traversal->node_indices[index],
                                  out_view);
}
