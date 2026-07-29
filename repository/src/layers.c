#include "internal.h"

atlas_status atlas_layers_build(atlas_context *context,
                                const atlas_document *document,
                                atlas_layers **out_layers) {
    if (context == NULL || document == NULL || out_layers == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_layers = NULL;
    size_t count = document->node_count;
    size_t *indegree = atlas_calloc(context, count, sizeof(*indegree));
    size_t *current = atlas_calloc(context, count, sizeof(*current));
    size_t *next = atlas_calloc(context, count, sizeof(*next));
    size_t *ordered = atlas_calloc(context, count, sizeof(*ordered));
    size_t *offsets = atlas_calloc(context, count + 1, sizeof(*offsets));
    if ((indegree == NULL || current == NULL || next == NULL || ordered == NULL ||
         offsets == NULL) && count != 0) {
        free(indegree);
        free(current);
        free(next);
        free(ordered);
        free(offsets);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        ++indegree[document->edges[edge_index].target_index];
    }
    size_t current_count = 0;
    for (size_t node_index = 0; node_index < count; ++node_index) {
        if (indegree[node_index] == 0) {
            current[current_count++] = node_index;
        }
    }
    size_t processed = 0;
    size_t layer_count = 0;
    while (current_count != 0) {
        offsets[layer_count++] = processed;
        size_t next_count = 0;
        for (size_t ordinal = 0; ordinal < current_count; ++ordinal) {
            size_t node_index = current[ordinal];
            ordered[processed++] = node_index;
            const atlas_node *node = &document->nodes[node_index];
            for (size_t edge_ordinal = 0;
                 edge_ordinal < node->outgoing_count;
                 ++edge_ordinal) {
                size_t edge_index = document->edge_order[node->outgoing_start + edge_ordinal];
                size_t target_index = document->edges[edge_index].target_index;
                if (--indegree[target_index] == 0) {
                    next[next_count++] = target_index;
                }
            }
        }
        size_t *swap = current;
        current = next;
        next = swap;
        current_count = next_count;
    }
    free(indegree);
    free(current);
    free(next);
    if (processed != count) {
        free(ordered);
        free(offsets);
        atlas_set_error(context, "cannot layer a graph containing a cycle");
        return ATLAS_ERR_CYCLE;
    }
    offsets[layer_count] = processed;
    atlas_layers *layers = atlas_calloc(context, 1, sizeof(*layers));
    if (layers == NULL) {
        free(ordered);
        free(offsets);
        return ATLAS_ERR_NOMEM;
    }
    layers->document = document;
    layers->node_indices = ordered;
    layers->offsets = offsets;
    layers->count = layer_count;
    *out_layers = layers;
    return ATLAS_OK;
}

void atlas_layers_destroy(atlas_layers *layers) {
    if (layers == NULL) {
        return;
    }
    free(layers->node_indices);
    free(layers->offsets);
    free(layers);
}

size_t atlas_layers_count(const atlas_layers *layers) {
    return layers == NULL ? 0 : layers->count;
}

size_t atlas_layers_size(const atlas_layers *layers, size_t layer_index) {
    if (layers == NULL || layer_index >= layers->count) {
        return 0;
    }
    return layers->offsets[layer_index + 1] - layers->offsets[layer_index];
}

atlas_status atlas_layers_node_at(const atlas_layers *layers,
                                  size_t layer_index,
                                  size_t ordinal,
                                  atlas_node_view *out_view) {
    if (layers == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (layer_index >= layers->count) {
        return ATLAS_ERR_RANGE;
    }
    size_t start = layers->offsets[layer_index];
    size_t end = layers->offsets[layer_index + 1];
    if (ordinal >= end - start) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_node_at(layers->document,
                                  layers->node_indices[start + ordinal],
                                  out_view);
}
