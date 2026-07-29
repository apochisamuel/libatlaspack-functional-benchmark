#include "internal.h"

static atlas_status atlas_copy_nodes_attributes_metadata(const atlas_document *document,
                                                         atlas_builder *builder) {
    atlas_status status = ATLAS_OK;
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_builder_add_node(builder, node->id, name->bytes);
    }
    for (size_t index = 0; index < document->attribute_count && status == ATLAS_OK; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        status = atlas_builder_add_attribute(builder,
                                             attribute->node_id,
                                             key->bytes,
                                             value->bytes);
    }
    for (size_t index = 0; index < document->metadata_count && status == ATLAS_OK; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        status = atlas_builder_add_metadata(builder, key->bytes, value->bytes);
    }
    return status;
}

atlas_status atlas_document_reverse(atlas_context *context,
                                    const atlas_document *document,
                                    atlas_document **out_document) {
    if (context == NULL || document == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    atlas_builder *builder = atlas_builder_create();
    if (builder == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = atlas_copy_nodes_attributes_metadata(document, builder);
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_builder_add_edge(builder,
                                        edge->target_id,
                                        edge->source_id,
                                        label->bytes);
    }
    if (status == ATLAS_OK) {
        atlas_parse_options options = document->options;
        status = atlas_builder_parse(context, builder, &options, out_document);
    }
    atlas_builder_destroy(builder);
    return status;
}

static int atlas_alternate_path_exists(const atlas_document *document,
                                       size_t source_index,
                                       size_t target_index,
                                       size_t excluded_edge) {
    unsigned char *visited = (unsigned char *)calloc(document->node_count,
                                                     sizeof(*visited));
    size_t *queue = (size_t *)calloc(document->node_count, sizeof(*queue));
    if ((visited == NULL || queue == NULL) && document->node_count != 0) {
        free(visited);
        free(queue);
        return -1;
    }
    size_t head = 0;
    size_t tail = 0;
    visited[source_index] = 1;
    queue[tail++] = source_index;
    while (head < tail) {
        size_t node_index = queue[head++];
        const atlas_node *node = &document->nodes[node_index];
        for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
            size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
            if (edge_index == excluded_edge) {
                continue;
            }
            size_t neighbor = document->edges[edge_index].target_index;
            if (neighbor == target_index) {
                free(visited);
                free(queue);
                return 1;
            }
            if (!visited[neighbor]) {
                visited[neighbor] = 1;
                queue[tail++] = neighbor;
            }
        }
    }
    free(visited);
    free(queue);
    return 0;
}

atlas_status atlas_document_transitive_reduction(atlas_context *context,
                                                 const atlas_document *document,
                                                 atlas_document **out_document) {
    if (context == NULL || document == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    atlas_plan *plan = NULL;
    atlas_status status = atlas_plan_build(context, document, &plan);
    atlas_plan_destroy(plan);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_builder *builder = atlas_builder_create();
    if (builder == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    status = atlas_copy_nodes_attributes_metadata(document, builder);
    for (size_t edge_index = 0;
         edge_index < document->edge_count && status == ATLAS_OK;
         ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        int alternate = atlas_alternate_path_exists(document,
                                                    edge->source_index,
                                                    edge->target_index,
                                                    edge_index);
        if (alternate < 0) {
            status = ATLAS_ERR_NOMEM;
            break;
        }
        if (alternate) {
            continue;
        }
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_builder_add_edge(builder,
                                        edge->source_id,
                                        edge->target_id,
                                        label->bytes);
    }
    if (status == ATLAS_OK) {
        atlas_parse_options options = document->options;
        status = atlas_builder_parse(context, builder, &options, out_document);
    }
    atlas_builder_destroy(builder);
    return status;
}
