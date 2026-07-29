#include "internal.h"

atlas_status atlas_document_prune_reachable(atlas_context *context,
                                            const atlas_document *document,
                                            uint32_t start_node_id,
                                            atlas_document **out_document) {
    if (context == NULL || document == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    atlas_selection *selection = NULL;
    atlas_status status = atlas_document_select_reachable(context,
                                                          document,
                                                          start_node_id,
                                                          1,
                                                          &selection);
    if (status == ATLAS_OK) {
        status = atlas_document_subgraph(context,
                                         document,
                                         selection,
                                         out_document);
    }
    atlas_selection_destroy(selection);
    return status;
}

atlas_status atlas_document_compact_ids(atlas_context *context,
                                        const atlas_document *document,
                                        uint32_t first_id,
                                        atlas_document **out_document) {
    if (context == NULL || document == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    if (document->node_count != 0 &&
        document->node_count - 1 > UINT32_MAX - first_id) {
        atlas_set_error(context, "compacted node IDs would overflow uint32_t");
        return ATLAS_ERR_RANGE;
    }
    uint32_t *new_ids = atlas_calloc(context,
                                     document->node_count,
                                     sizeof(*new_ids));
    if (new_ids == NULL && document->node_count != 0) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_builder *builder = atlas_builder_create();
    if (builder == NULL) {
        free(new_ids);
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = ATLAS_OK;
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        new_ids[index] = first_id + (uint32_t)index;
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_builder_add_node(builder, new_ids[index], name->bytes);
    }
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_builder_add_edge(builder,
                                        new_ids[edge->source_index],
                                        new_ids[edge->target_index],
                                        label->bytes);
    }
    for (size_t index = 0; index < document->attribute_count && status == ATLAS_OK; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        status = atlas_builder_add_attribute(builder,
                                             new_ids[attribute->node_index],
                                             key->bytes,
                                             value->bytes);
    }
    for (size_t index = 0; index < document->metadata_count && status == ATLAS_OK; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        status = atlas_builder_add_metadata(builder, key->bytes, value->bytes);
    }
    if (status == ATLAS_OK) {
        atlas_parse_options options = document->options;
        status = atlas_builder_parse(context, builder, &options, out_document);
    }
    atlas_builder_destroy(builder);
    free(new_ids);
    return status;
}

atlas_status atlas_document_filter_edge_label(atlas_context *context,
                                              const atlas_document *document,
                                              const char *label,
                                              int keep_matches,
                                              atlas_document **out_document) {
    if (context == NULL || document == NULL || label == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    atlas_builder *builder = atlas_builder_create();
    if (builder == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = ATLAS_OK;
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_builder_add_node(builder, node->id, name->bytes);
    }
    size_t label_length = strlen(label);
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *candidate = &document->strings[edge->label_index];
        int matches = candidate->length == label_length &&
                      memcmp(candidate->bytes, label, label_length) == 0;
        if ((keep_matches && !matches) || (!keep_matches && matches)) {
            continue;
        }
        status = atlas_builder_add_edge(builder,
                                        edge->source_id,
                                        edge->target_id,
                                        candidate->bytes);
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
    if (status == ATLAS_OK) {
        atlas_parse_options options = document->options;
        status = atlas_builder_parse(context, builder, &options, out_document);
    }
    atlas_builder_destroy(builder);
    return status;
}
