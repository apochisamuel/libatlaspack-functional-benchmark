#include "internal.h"

atlas_status atlas_plan_build(atlas_context *context,
                              const atlas_document *document,
                              atlas_plan **out_plan) {
    if (context == NULL || document == NULL || out_plan == NULL) return ATLAS_ERR_ARGUMENT;
    *out_plan = NULL;
    size_t *indegree = atlas_calloc(context, document->node_count, sizeof(*indegree));
    size_t *queue = atlas_calloc(context, document->node_count, sizeof(*queue));
    size_t *order = atlas_calloc(context, document->node_count, sizeof(*order));
    if ((indegree == NULL || queue == NULL || order == NULL) && document->node_count != 0) {
        free(indegree); free(queue); free(order);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t i = 0; i < document->edge_count; ++i) indegree[document->edges[i].target_index]++;
    size_t head = 0, tail = 0;
    for (size_t i = 0; i < document->node_count; ++i) if (indegree[i] == 0) queue[tail++] = i;
    size_t count = 0;
    while (head < tail) {
        size_t node_index = queue[head++];
        order[count++] = node_index;
        const atlas_node *node = &document->nodes[node_index];
        for (size_t j = 0; j < node->outgoing_count; ++j) {
            const atlas_edge *edge = &document->edges[document->edge_order[node->outgoing_start + j]];
            if (--indegree[edge->target_index] == 0) queue[tail++] = edge->target_index;
        }
    }
    free(indegree);
    free(queue);
    if (count != document->node_count) {
        free(order);
        atlas_set_error(context, "graph contains a cycle");
        return ATLAS_ERR_CYCLE;
    }
    atlas_plan *plan = atlas_calloc(context, 1, sizeof(*plan));
    if (plan == NULL) { free(order); return ATLAS_ERR_NOMEM; }
    plan->document = document;
    plan->node_order = order;
    plan->count = count;
    *out_plan = plan;
    return ATLAS_OK;
}

void atlas_plan_destroy(atlas_plan *plan) {
    if (plan == NULL) return;
    free(plan->node_order);
    free(plan);
}

size_t atlas_plan_count(const atlas_plan *plan) { return plan == NULL ? 0 : plan->count; }

atlas_status atlas_plan_node_at(const atlas_plan *plan,
                                size_t index,
                                atlas_node_view *out_view) {
    if (plan == NULL || out_view == NULL) return ATLAS_ERR_ARGUMENT;
    if (index >= plan->count) return ATLAS_ERR_RANGE;
    return atlas_document_node_at(plan->document, plan->node_order[index], out_view);
}
