#include "internal.h"

static atlas_status atlas_selection_allocate(atlas_context *context,
                                             const atlas_document *document,
                                             size_t capacity,
                                             atlas_selection **out_selection) {
    atlas_selection *selection = atlas_calloc(context, 1, sizeof(*selection));
    if (selection == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    selection->node_indices = atlas_calloc(context,
                                           capacity,
                                           sizeof(*selection->node_indices));
    if (selection->node_indices == NULL && capacity != 0) {
        free(selection);
        return ATLAS_ERR_NOMEM;
    }
    selection->document = document;
    *out_selection = selection;
    return ATLAS_OK;
}

atlas_status atlas_document_select_ids(atlas_context *context,
                                       const atlas_document *document,
                                       const uint32_t *node_ids,
                                       size_t node_id_count,
                                       atlas_selection **out_selection) {
    if (context == NULL || document == NULL || out_selection == NULL ||
        (node_ids == NULL && node_id_count != 0)) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    atlas_selection *selection = NULL;
    atlas_status status = atlas_selection_allocate(context,
                                                   document,
                                                   node_id_count,
                                                   &selection);
    if (status != ATLAS_OK) {
        return status;
    }
    for (size_t ordinal = 0; ordinal < node_id_count; ++ordinal) {
        size_t node_index = atlas_node_index_by_id(document, node_ids[ordinal]);
        if (node_index == ATLAS_NO_INDEX) {
            atlas_selection_destroy(selection);
            atlas_set_error(context, "selected node %u does not exist", node_ids[ordinal]);
            return ATLAS_ERR_NOT_FOUND;
        }
        int duplicate = 0;
        for (size_t existing = 0; existing < selection->count; ++existing) {
            if (selection->node_indices[existing] == node_index) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            selection->node_indices[selection->count++] = node_index;
        }
    }
    *out_selection = selection;
    return ATLAS_OK;
}

static atlas_status atlas_document_select_degree_class(atlas_context *context,
                                                       const atlas_document *document,
                                                       int sources,
                                                       atlas_selection **out_selection) {
    atlas_selection *selection = NULL;
    atlas_status status = atlas_selection_allocate(context,
                                                   document,
                                                   document->node_count,
                                                   &selection);
    if (status != ATLAS_OK) {
        return status;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        const atlas_node *node = &document->nodes[index];
        int include = sources ? node->incoming_count == 0 : node->outgoing_count == 0;
        if (include) {
            selection->node_indices[selection->count++] = index;
        }
    }
    *out_selection = selection;
    return ATLAS_OK;
}

atlas_status atlas_document_select_sources(atlas_context *context,
                                           const atlas_document *document,
                                           atlas_selection **out_selection) {
    if (context == NULL || document == NULL || out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    return atlas_document_select_degree_class(context,
                                              document,
                                              1,
                                              out_selection);
}

atlas_status atlas_document_select_sinks(atlas_context *context,
                                         const atlas_document *document,
                                         atlas_selection **out_selection) {
    if (context == NULL || document == NULL || out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    return atlas_document_select_degree_class(context,
                                              document,
                                              0,
                                              out_selection);
}

atlas_status atlas_document_select_reachable(atlas_context *context,
                                             const atlas_document *document,
                                             uint32_t start_node_id,
                                             int include_start,
                                             atlas_selection **out_selection) {
    if (context == NULL || document == NULL || out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    size_t start_index = atlas_node_index_by_id(document, start_node_id);
    if (start_index == ATLAS_NO_INDEX) {
        atlas_set_error(context, "reachable selection start node %u does not exist", start_node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    atlas_selection *selection = NULL;
    atlas_status status = atlas_selection_allocate(context,
                                                   document,
                                                   document->node_count,
                                                   &selection);
    if (status != ATLAS_OK) {
        return status;
    }
    unsigned char *visited = atlas_calloc(context,
                                          document->node_count,
                                          sizeof(*visited));
    size_t *queue = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*queue));
    if ((visited == NULL || queue == NULL) && document->node_count != 0) {
        free(visited);
        free(queue);
        atlas_selection_destroy(selection);
        return ATLAS_ERR_NOMEM;
    }
    size_t head = 0;
    size_t tail = 0;
    visited[start_index] = 1;
    queue[tail++] = start_index;
    while (head < tail) {
        size_t node_index = queue[head++];
        if (include_start || node_index != start_index) {
            selection->node_indices[selection->count++] = node_index;
        }
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
    *out_selection = selection;
    return ATLAS_OK;
}
