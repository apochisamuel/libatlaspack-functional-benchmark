#include "internal.h"

static atlas_status atlas_path_allocate(atlas_context *context,
                                        const atlas_document *document,
                                        size_t node_count,
                                        atlas_path **out_path) {
    atlas_path *path = atlas_calloc(context, 1, sizeof(*path));
    if (path == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    path->node_indices = atlas_calloc(context,
                                      node_count,
                                      sizeof(*path->node_indices));
    if (path->node_indices == NULL && node_count != 0) {
        free(path);
        return ATLAS_ERR_NOMEM;
    }
    if (node_count > 1) {
        path->edge_indices = atlas_calloc(context,
                                          node_count - 1,
                                          sizeof(*path->edge_indices));
        if (path->edge_indices == NULL) {
            free(path->node_indices);
            free(path);
            return ATLAS_ERR_NOMEM;
        }
    }
    path->document = document;
    path->node_count = node_count;
    *out_path = path;
    return ATLAS_OK;
}

atlas_status atlas_path_find(atlas_context *context,
                             const atlas_document *document,
                             uint32_t source_node_id,
                             uint32_t target_node_id,
                             atlas_path **out_path) {
    if (context == NULL || document == NULL || out_path == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_path = NULL;
    size_t source_index = atlas_node_index_by_id(document, source_node_id);
    size_t target_index = atlas_node_index_by_id(document, target_node_id);
    if (source_index == ATLAS_NO_INDEX || target_index == ATLAS_NO_INDEX) {
        atlas_set_error(context,
                        "cannot find path between missing nodes %u and %u",
                        source_node_id,
                        target_node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    size_t count = document->node_count;
    size_t *queue = atlas_calloc(context, count, sizeof(*queue));
    size_t *previous_node = atlas_calloc(context, count, sizeof(*previous_node));
    size_t *previous_edge = atlas_calloc(context, count, sizeof(*previous_edge));
    unsigned char *visited = atlas_calloc(context, count, sizeof(*visited));
    if ((queue == NULL || previous_node == NULL || previous_edge == NULL || visited == NULL) &&
        count != 0) {
        free(queue);
        free(previous_node);
        free(previous_edge);
        free(visited);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < count; ++index) {
        previous_node[index] = ATLAS_NO_INDEX;
        previous_edge[index] = ATLAS_NO_INDEX;
    }
    size_t head = 0;
    size_t tail = 0;
    queue[tail++] = source_index;
    visited[source_index] = 1;
    while (head < tail && !visited[target_index]) {
        size_t current = queue[head++];
        const atlas_node *node = &document->nodes[current];
        for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
            size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
            size_t next = document->edges[edge_index].target_index;
            if (!visited[next]) {
                visited[next] = 1;
                previous_node[next] = current;
                previous_edge[next] = edge_index;
                queue[tail++] = next;
            }
        }
    }
    if (!visited[target_index]) {
        free(queue);
        free(previous_node);
        free(previous_edge);
        free(visited);
        atlas_set_error(context,
                        "node %u is not reachable from node %u",
                        target_node_id,
                        source_node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    size_t path_length = 1;
    for (size_t cursor = target_index; cursor != source_index; cursor = previous_node[cursor]) {
        ++path_length;
    }
    atlas_path *path = NULL;
    atlas_status status = atlas_path_allocate(context, document, path_length, &path);
    if (status != ATLAS_OK) {
        free(queue);
        free(previous_node);
        free(previous_edge);
        free(visited);
        return status;
    }
    size_t cursor = target_index;
    for (size_t position = path_length; position > 0; --position) {
        path->node_indices[position - 1] = cursor;
        if (position > 1) {
            path->edge_indices[position - 2] = previous_edge[cursor];
            cursor = previous_node[cursor];
        }
    }
    free(queue);
    free(previous_node);
    free(previous_edge);
    free(visited);
    *out_path = path;
    return ATLAS_OK;
}

void atlas_path_destroy(atlas_path *path) {
    if (path == NULL) {
        return;
    }
    free(path->node_indices);
    free(path->edge_indices);
    free(path);
}

size_t atlas_path_count(const atlas_path *path) {
    return path == NULL ? 0 : path->node_count;
}

atlas_status atlas_path_node_at(const atlas_path *path,
                                size_t index,
                                atlas_node_view *out_view) {
    if (path == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= path->node_count) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_node_at(path->document,
                                  path->node_indices[index],
                                  out_view);
}

size_t atlas_path_edge_count(const atlas_path *path) {
    return path == NULL || path->node_count == 0 ? 0 : path->node_count - 1;
}

atlas_status atlas_path_edge_at(const atlas_path *path,
                                size_t index,
                                atlas_edge_view *out_view) {
    if (path == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= atlas_path_edge_count(path)) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_edge_at(path->document,
                                  path->edge_indices[index],
                                  out_view);
}
