#include "internal.h"

static int atlas_selection_contains_index(const atlas_selection *selection,
                                          size_t node_index) {
    for (size_t index = 0; index < selection->count; ++index) {
        if (selection->node_indices[index] == node_index) {
            return 1;
        }
    }
    return 0;
}

atlas_status atlas_document_subgraph(atlas_context *context,
                                     const atlas_document *document,
                                     const atlas_selection *selection,
                                     atlas_document **out_document) {
    if (context == NULL || document == NULL || selection == NULL ||
        out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    if (selection->document != document) {
        atlas_set_error(context, "selection belongs to a different document");
        return ATLAS_ERR_ARGUMENT;
    }
    if (selection->count == 0) {
        atlas_set_error(context, "cannot create an ATPK document with no nodes");
        return ATLAS_ERR_FORMAT;
    }
    atlas_builder *builder = atlas_builder_create();
    if (builder == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = ATLAS_OK;
    for (size_t ordinal = 0; ordinal < selection->count && status == ATLAS_OK; ++ordinal) {
        size_t node_index = selection->node_indices[ordinal];
        const atlas_node *node = &document->nodes[node_index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_builder_add_node(builder, node->id, name->bytes);
    }
    for (size_t edge_index = 0;
         edge_index < document->edge_count && status == ATLAS_OK;
         ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        if (!atlas_selection_contains_index(selection, edge->source_index) ||
            !atlas_selection_contains_index(selection, edge->target_index)) {
            continue;
        }
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_builder_add_edge(builder,
                                        edge->source_id,
                                        edge->target_id,
                                        label->bytes);
    }
    for (size_t attribute_index = 0;
         attribute_index < document->attribute_count && status == ATLAS_OK;
         ++attribute_index) {
        const atlas_attribute *attribute = &document->attributes[attribute_index];
        if (!atlas_selection_contains_index(selection, attribute->node_index)) {
            continue;
        }
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        status = atlas_builder_add_attribute(builder,
                                             attribute->node_id,
                                             key->bytes,
                                             value->bytes);
    }
    for (size_t metadata_index = 0;
         metadata_index < document->metadata_count && status == ATLAS_OK;
         ++metadata_index) {
        const atlas_metadata *metadata = &document->metadata[metadata_index];
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
