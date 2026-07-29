#include "internal.h"

static atlas_status atlas_resolve_nodes(atlas_context *context,
                                        atlas_document *document) {
    for (size_t index = 0; index < document->node_count; ++index) {
        atlas_node *node = &document->nodes[index];
        if (node->name_index >= document->string_count) {
            atlas_set_error(context,
                            "node %u has invalid name index %u",
                            node->id,
                            node->name_index);
            return ATLAS_ERR_REFERENCE;
        }
        if (node->first_attribute > document->attribute_count ||
            node->attribute_count >
                document->attribute_count - node->first_attribute) {
            atlas_set_error(context,
                            "node %u has invalid attribute slice [%u, %u)",
                            node->id,
                            node->first_attribute,
                            node->first_attribute + node->attribute_count);
            return ATLAS_ERR_REFERENCE;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_resolve_edges(atlas_context *context,
                                        atlas_document *document) {
    for (size_t index = 0; index < document->edge_count; ++index) {
        atlas_edge *edge = &document->edges[index];
        int source_found = 0;
        int target_found = 0;
        edge->source_index = atlas_id_lookup(document,
                                             edge->source_id,
                                             &source_found);
        edge->target_index = atlas_id_lookup(document,
                                             edge->target_id,
                                             &target_found);
        if (!source_found) {
            atlas_set_error(context,
                            "edge %zu refers to missing source node %u",
                            index,
                            edge->source_id);
            return ATLAS_ERR_REFERENCE;
        }
        if (!target_found) {
            atlas_set_error(context,
                            "edge %zu refers to missing target node %u",
                            index,
                            edge->target_id);
            return ATLAS_ERR_REFERENCE;
        }
        if (edge->label_index >= document->string_count) {
            atlas_set_error(context,
                            "edge %zu has invalid label index %u",
                            index,
                            edge->label_index);
            return ATLAS_ERR_REFERENCE;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_resolve_attributes(atlas_context *context,
                                             atlas_document *document) {
    for (size_t index = 0; index < document->attribute_count; ++index) {
        atlas_attribute *attribute = &document->attributes[index];
        int found = 0;
        attribute->node_index = atlas_id_lookup(document,
                                                attribute->node_id,
                                                &found);
        if (!found) {
            atlas_set_error(context,
                            "attribute %zu refers to missing node %u",
                            index,
                            attribute->node_id);
            return ATLAS_ERR_REFERENCE;
        }
        if (attribute->key_index >= document->string_count) {
            atlas_set_error(context,
                            "attribute %zu has invalid key index %u",
                            index,
                            attribute->key_index);
            return ATLAS_ERR_REFERENCE;
        }
        if (attribute->value_index >= document->string_count) {
            atlas_set_error(context,
                            "attribute %zu has invalid value index %u",
                            index,
                            attribute->value_index);
            return ATLAS_ERR_REFERENCE;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_resolve_metadata(atlas_context *context,
                                           const atlas_document *document) {
    for (size_t index = 0; index < document->metadata_count; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        if (metadata->key_index >= document->string_count) {
            atlas_set_error(context,
                            "metadata %zu has invalid key index %u",
                            index,
                            metadata->key_index);
            return ATLAS_ERR_REFERENCE;
        }
        if (metadata->value_index >= document->string_count) {
            atlas_set_error(context,
                            "metadata %zu has invalid value index %u",
                            index,
                            metadata->value_index);
            return ATLAS_ERR_REFERENCE;
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_resolve_references(atlas_context *context,
                                      atlas_document *document) {
    if (context == NULL || document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_status status = atlas_resolve_nodes(context, document);
    if (status == ATLAS_OK) {
        status = atlas_resolve_edges(context, document);
    }
    if (status == ATLAS_OK) {
        status = atlas_resolve_attributes(context, document);
    }
    if (status == ATLAS_OK) {
        status = atlas_resolve_metadata(context, document);
    }
    return status;
}
