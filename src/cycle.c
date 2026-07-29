#include "internal.h"

typedef struct atlas_dfs_frame {
    size_t node_index;
    size_t next_ordinal;
} atlas_dfs_frame;

static atlas_status atlas_cycle_from_back_edge(atlas_context *context,
                                               const atlas_document *document,
                                               const size_t *parent_node,
                                               const size_t *parent_edge,
                                               size_t current,
                                               size_t ancestor,
                                               size_t back_edge,
                                               atlas_path **out_cycle) {
    size_t chain_length = 1;
    for (size_t cursor = current; cursor != ancestor; cursor = parent_node[cursor]) {
        if (cursor == ATLAS_NO_INDEX || parent_node[cursor] == ATLAS_NO_INDEX) {
            atlas_set_error(context, "cycle reconstruction encountered an invalid parent chain");
            return ATLAS_ERR_FORMAT;
        }
        ++chain_length;
    }
    if (chain_length == SIZE_MAX) {
        return ATLAS_ERR_RANGE;
    }
    size_t node_count = chain_length + 1;
    atlas_path *cycle = atlas_calloc(context, 1, sizeof(*cycle));
    if (cycle == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    cycle->node_indices = atlas_calloc(context,
                                       node_count,
                                       sizeof(*cycle->node_indices));
    cycle->edge_indices = atlas_calloc(context,
                                       node_count - 1,
                                       sizeof(*cycle->edge_indices));
    if (cycle->node_indices == NULL || cycle->edge_indices == NULL) {
        atlas_path_destroy(cycle);
        return ATLAS_ERR_NOMEM;
    }
    cycle->document = document;
    cycle->node_count = node_count;
    cycle->node_indices[0] = ancestor;
    cycle->node_indices[node_count - 1] = ancestor;
    cycle->edge_indices[node_count - 2] = back_edge;
    size_t cursor = current;
    for (size_t position = node_count - 1; position > 1; --position) {
        cycle->node_indices[position - 1] = cursor;
        cycle->edge_indices[position - 2] = parent_edge[cursor];
        cursor = parent_node[cursor];
    }
    *out_cycle = cycle;
    return ATLAS_OK;
}

atlas_status atlas_document_find_cycle(atlas_context *context,
                                       const atlas_document *document,
                                       atlas_path **out_cycle) {
    if (context == NULL || document == NULL || out_cycle == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_cycle = NULL;
    size_t count = document->node_count;
    unsigned char *color = atlas_calloc(context, count, sizeof(*color));
    size_t *parent_node = atlas_calloc(context, count, sizeof(*parent_node));
    size_t *parent_edge = atlas_calloc(context, count, sizeof(*parent_edge));
    atlas_dfs_frame *stack = atlas_calloc(context, count, sizeof(*stack));
    if ((color == NULL || parent_node == NULL || parent_edge == NULL || stack == NULL) &&
        count != 0) {
        free(color);
        free(parent_node);
        free(parent_edge);
        free(stack);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < count; ++index) {
        parent_node[index] = ATLAS_NO_INDEX;
        parent_edge[index] = ATLAS_NO_INDEX;
    }
    atlas_status status = ATLAS_ERR_NOT_FOUND;
    for (size_t start = 0; start < count && status == ATLAS_ERR_NOT_FOUND; ++start) {
        if (color[start] != 0) {
            continue;
        }
        size_t stack_count = 0;
        color[start] = 1;
        stack[stack_count++] = (atlas_dfs_frame){start, 0};
        while (stack_count != 0 && status == ATLAS_ERR_NOT_FOUND) {
            atlas_dfs_frame *frame = &stack[stack_count - 1];
            const atlas_node *node = &document->nodes[frame->node_index];
            if (frame->next_ordinal >= node->outgoing_count) {
                color[frame->node_index] = 2;
                --stack_count;
                continue;
            }
            size_t edge_index =
                document->edge_order[node->outgoing_start + frame->next_ordinal++];
            size_t target = document->edges[edge_index].target_index;
            if (color[target] == 0) {
                parent_node[target] = frame->node_index;
                parent_edge[target] = edge_index;
                color[target] = 1;
                stack[stack_count++] = (atlas_dfs_frame){target, 0};
            } else if (color[target] == 1) {
                status = atlas_cycle_from_back_edge(context,
                                                    document,
                                                    parent_node,
                                                    parent_edge,
                                                    frame->node_index,
                                                    target,
                                                    edge_index,
                                                    out_cycle);
            }
        }
    }
    free(color);
    free(parent_node);
    free(parent_edge);
    free(stack);
    if (status == ATLAS_ERR_NOT_FOUND) {
        atlas_set_error(context, "document graph is acyclic");
    }
    return status;
}
