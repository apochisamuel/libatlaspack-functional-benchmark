#include "internal.h"

typedef struct atlas_tarjan_state {
    const atlas_document *document;
    size_t *index_by_node;
    size_t *lowlink;
    unsigned char *on_stack;
    size_t *stack;
    size_t stack_count;
    size_t next_index;
    atlas_strong_components *components;
} atlas_tarjan_state;

static atlas_status atlas_tarjan_visit(atlas_tarjan_state *state, size_t node_index) {
    state->index_by_node[node_index] = state->next_index;
    state->lowlink[node_index] = state->next_index;
    ++state->next_index;
    state->stack[state->stack_count++] = node_index;
    state->on_stack[node_index] = 1;
    const atlas_node *node = &state->document->nodes[node_index];
    for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
        size_t edge_index =
            state->document->edge_order[node->outgoing_start + ordinal];
        size_t target_index = state->document->edges[edge_index].target_index;
        if (state->index_by_node[target_index] == ATLAS_NO_INDEX) {
            atlas_status status = atlas_tarjan_visit(state, target_index);
            if (status != ATLAS_OK) {
                return status;
            }
            if (state->lowlink[target_index] < state->lowlink[node_index]) {
                state->lowlink[node_index] = state->lowlink[target_index];
            }
        } else if (state->on_stack[target_index] &&
                   state->index_by_node[target_index] < state->lowlink[node_index]) {
            state->lowlink[node_index] = state->index_by_node[target_index];
        }
    }
    if (state->lowlink[node_index] == state->index_by_node[node_index]) {
        size_t component_index = state->components->count++;
        size_t component_size = 0;
        size_t member = ATLAS_NO_INDEX;
        do {
            member = state->stack[--state->stack_count];
            state->on_stack[member] = 0;
            state->components->component_by_node[member] = component_index;
            ++component_size;
        } while (member != node_index);
        state->components->component_sizes[component_index] = component_size;
    }
    return ATLAS_OK;
}

static atlas_status atlas_strong_components_allocate(
    atlas_context *context,
    const atlas_document *document,
    atlas_strong_components **out_components) {
    atlas_strong_components *components =
        atlas_calloc(context, 1, sizeof(*components));
    if (components == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    components->component_by_node = atlas_calloc(
        context,
        document->node_count,
        sizeof(*components->component_by_node));
    components->component_sizes = atlas_calloc(
        context,
        document->node_count,
        sizeof(*components->component_sizes));
    components->component_cyclic = atlas_calloc(
        context,
        document->node_count,
        sizeof(*components->component_cyclic));
    if ((components->component_by_node == NULL ||
         components->component_sizes == NULL ||
         components->component_cyclic == NULL) &&
        document->node_count != 0) {
        atlas_strong_components_destroy(components);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        components->component_by_node[index] = ATLAS_NO_INDEX;
    }
    components->document = document;
    *out_components = components;
    return ATLAS_OK;
}

static void atlas_strong_components_mark_cycles(atlas_strong_components *components) {
    const atlas_document *document = components->document;
    for (size_t component = 0; component < components->count; ++component) {
        if (components->component_sizes[component] > 1) {
            components->component_cyclic[component] = 1;
            ++components->cyclic_count;
        }
    }
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        if (edge->source_index != edge->target_index) {
            continue;
        }
        size_t component = components->component_by_node[edge->source_index];
        if (!components->component_cyclic[component]) {
            components->component_cyclic[component] = 1;
            ++components->cyclic_count;
        }
    }
}

atlas_status atlas_strong_components_build(atlas_context *context,
                                           const atlas_document *document,
                                           atlas_strong_components **out_components) {
    if (context == NULL || document == NULL || out_components == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_components = NULL;
    atlas_strong_components *components = NULL;
    atlas_status status = atlas_strong_components_allocate(context,
                                                           document,
                                                           &components);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_tarjan_state state;
    memset(&state, 0, sizeof(state));
    state.document = document;
    state.components = components;
    state.index_by_node = atlas_calloc(context,
                                       document->node_count,
                                       sizeof(*state.index_by_node));
    state.lowlink = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*state.lowlink));
    state.on_stack = atlas_calloc(context,
                                  document->node_count,
                                  sizeof(*state.on_stack));
    state.stack = atlas_calloc(context,
                               document->node_count,
                               sizeof(*state.stack));
    if ((state.index_by_node == NULL || state.lowlink == NULL ||
         state.on_stack == NULL || state.stack == NULL) &&
        document->node_count != 0) {
        free(state.index_by_node);
        free(state.lowlink);
        free(state.on_stack);
        free(state.stack);
        atlas_strong_components_destroy(components);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        state.index_by_node[index] = ATLAS_NO_INDEX;
    }
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        if (state.index_by_node[index] == ATLAS_NO_INDEX) {
            status = atlas_tarjan_visit(&state, index);
        }
    }
    free(state.index_by_node);
    free(state.lowlink);
    free(state.on_stack);
    free(state.stack);
    if (status != ATLAS_OK) {
        atlas_strong_components_destroy(components);
        return status;
    }
    atlas_strong_components_mark_cycles(components);
    *out_components = components;
    return ATLAS_OK;
}

void atlas_strong_components_destroy(atlas_strong_components *components) {
    if (components == NULL) {
        return;
    }
    free(components->component_by_node);
    free(components->component_sizes);
    free(components->component_cyclic);
    free(components);
}

size_t atlas_strong_components_count(const atlas_strong_components *components) {
    return components == NULL ? 0 : components->count;
}

size_t atlas_strong_components_size(const atlas_strong_components *components,
                                    size_t component_index) {
    if (components == NULL || component_index >= components->count) {
        return 0;
    }
    return components->component_sizes[component_index];
}

atlas_status atlas_strong_components_node_component(
    const atlas_strong_components *components,
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

size_t atlas_strong_components_cyclic_count(
    const atlas_strong_components *components) {
    return components == NULL ? 0 : components->cyclic_count;
}
