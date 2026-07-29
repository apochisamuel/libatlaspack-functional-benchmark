#include "internal.h"

static atlas_status atlas_components_allocate(atlas_context *context,
                                              const atlas_document *document,
                                              atlas_components **out_components) {
    atlas_components *components = atlas_calloc(context, 1, sizeof(*components));
    if (components == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    components->component_by_node = atlas_calloc(context,
                                                 document->node_count,
                                                 sizeof(*components->component_by_node));
    components->component_sizes = atlas_calloc(context,
                                               document->node_count,
                                               sizeof(*components->component_sizes));
    if ((components->component_by_node == NULL || components->component_sizes == NULL) &&
        document->node_count != 0) {
        free(components->component_by_node);
        free(components->component_sizes);
        free(components);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        components->component_by_node[index] = ATLAS_NO_INDEX;
    }
    components->document = document;
    *out_components = components;
    return ATLAS_OK;
}

atlas_status atlas_components_build(atlas_context *context,
                                    const atlas_document *document,
                                    atlas_components **out_components) {
    if (context == NULL || document == NULL || out_components == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_components = NULL;
    atlas_components *components = NULL;
    atlas_status status = atlas_components_allocate(context,
                                                    document,
                                                    &components);
    if (status != ATLAS_OK) {
        return status;
    }
    size_t *queue = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*queue));
    if (queue == NULL && document->node_count != 0) {
        atlas_components_destroy(components);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t start = 0; start < document->node_count; ++start) {
        if (components->component_by_node[start] != ATLAS_NO_INDEX) {
            continue;
        }
        size_t component_index = components->count++;
        size_t head = 0;
        size_t tail = 0;
        queue[tail++] = start;
        components->component_by_node[start] = component_index;
        while (head < tail) {
            size_t node_index = queue[head++];
            ++components->component_sizes[component_index];
            const atlas_node *node = &document->nodes[node_index];
            for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
                size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
                size_t neighbor = document->edges[edge_index].target_index;
                if (components->component_by_node[neighbor] == ATLAS_NO_INDEX) {
                    components->component_by_node[neighbor] = component_index;
                    queue[tail++] = neighbor;
                }
            }
            for (size_t ordinal = 0; ordinal < node->incoming_count; ++ordinal) {
                size_t edge_index = document->incoming_order[node->incoming_start + ordinal];
                size_t neighbor = document->edges[edge_index].source_index;
                if (components->component_by_node[neighbor] == ATLAS_NO_INDEX) {
                    components->component_by_node[neighbor] = component_index;
                    queue[tail++] = neighbor;
                }
            }
        }
    }
    free(queue);
    *out_components = components;
    return ATLAS_OK;
}

void atlas_components_destroy(atlas_components *components) {
    if (components == NULL) {
        return;
    }
    free(components->component_by_node);
    free(components->component_sizes);
    free(components);
}

size_t atlas_components_count(const atlas_components *components) {
    return components == NULL ? 0 : components->count;
}

size_t atlas_components_size(const atlas_components *components,
                             size_t component_index) {
    if (components == NULL || component_index >= components->count) {
        return 0;
    }
    return components->component_sizes[component_index];
}

atlas_status atlas_components_node_component(const atlas_components *components,
                                              uint32_t node_id,
                                              size_t *out_component_index) {
    if (components == NULL || out_component_index == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(components->document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    *out_component_index = components->component_by_node[node_index];
    return ATLAS_OK;
}
