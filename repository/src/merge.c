#include "internal.h"

static size_t atlas_builder_node_index_for_merge(const atlas_builder *builder,
                                                 uint32_t node_id) {
    for (size_t index = 0; index < builder->node_count; ++index) {
        if (builder->nodes[index].id == node_id) {
            return index;
        }
    }
    return ATLAS_NO_INDEX;
}

static int atlas_builder_has_edge_for_merge(const atlas_builder *builder,
                                            uint32_t source_id,
                                            uint32_t target_id,
                                            const char *label) {
    for (size_t index = 0; index < builder->edge_count; ++index) {
        const atlas_builder_edge *edge = &builder->edges[index];
        if (edge->source_id == source_id && edge->target_id == target_id &&
            strcmp(edge->label, label) == 0) {
            return 1;
        }
    }
    return 0;
}

static size_t atlas_builder_attribute_index_for_merge(const atlas_builder *builder,
                                                      uint32_t node_id,
                                                      const char *key) {
    for (size_t index = 0; index < builder->attribute_count; ++index) {
        const atlas_builder_attribute *attribute = &builder->attributes[index];
        if (attribute->node_id == node_id && strcmp(attribute->key, key) == 0) {
            return index;
        }
    }
    return ATLAS_NO_INDEX;
}

static size_t atlas_builder_metadata_index_for_merge(const atlas_builder *builder,
                                                     const char *key) {
    for (size_t index = 0; index < builder->metadata_count; ++index) {
        if (strcmp(builder->metadata[index].key, key) == 0) {
            return index;
        }
    }
    return ATLAS_NO_INDEX;
}

static atlas_status atlas_merge_nodes(atlas_builder *builder,
                                      const atlas_document *document,
                                      atlas_merge_policy policy) {
    for (size_t index = 0; index < document->node_count; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        size_t builder_index = atlas_builder_node_index_for_merge(builder, node->id);
        if (builder_index == ATLAS_NO_INDEX) {
            atlas_status status = atlas_builder_add_node(builder,
                                                         node->id,
                                                         name->bytes);
            if (status != ATLAS_OK) {
                return status;
            }
            continue;
        }
        if (strcmp(builder->nodes[builder_index].name, name->bytes) == 0) {
            continue;
        }
        if (policy == ATLAS_MERGE_REJECT_CONFLICTS) {
            return ATLAS_ERR_CONFLICT;
        }
        if (policy == ATLAS_MERGE_PREFER_RIGHT) {
            atlas_status status = atlas_builder_rename_node(builder,
                                                            node->id,
                                                            name->bytes);
            if (status != ATLAS_OK) {
                return status;
            }
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_merge_edges(atlas_builder *builder,
                                      const atlas_document *document) {
    for (size_t index = 0; index < document->edge_count; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        if (atlas_builder_has_edge_for_merge(builder,
                                             edge->source_id,
                                             edge->target_id,
                                             label->bytes)) {
            continue;
        }
        atlas_status status = atlas_builder_add_edge(builder,
                                                     edge->source_id,
                                                     edge->target_id,
                                                     label->bytes);
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_merge_attributes(atlas_builder *builder,
                                           const atlas_document *document,
                                           atlas_merge_policy policy) {
    for (size_t index = 0; index < document->attribute_count; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        size_t builder_index = atlas_builder_attribute_index_for_merge(builder,
                                                                       attribute->node_id,
                                                                       key->bytes);
        if (builder_index == ATLAS_NO_INDEX) {
            atlas_status status = atlas_builder_add_attribute(builder,
                                                              attribute->node_id,
                                                              key->bytes,
                                                              value->bytes);
            if (status != ATLAS_OK) {
                return status;
            }
            continue;
        }
        atlas_builder_attribute *existing = &builder->attributes[builder_index];
        if (strcmp(existing->value, value->bytes) == 0) {
            continue;
        }
        if (policy == ATLAS_MERGE_REJECT_CONFLICTS) {
            return ATLAS_ERR_CONFLICT;
        }
        if (policy == ATLAS_MERGE_PREFER_RIGHT) {
            atlas_status status = atlas_builder_set_attribute(builder,
                                                              attribute->node_id,
                                                              key->bytes,
                                                              value->bytes);
            if (status != ATLAS_OK) {
                return status;
            }
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_merge_metadata(atlas_builder *builder,
                                         const atlas_document *document,
                                         atlas_merge_policy policy) {
    for (size_t index = 0; index < document->metadata_count; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        size_t builder_index = atlas_builder_metadata_index_for_merge(builder, key->bytes);
        if (builder_index == ATLAS_NO_INDEX) {
            atlas_status status = atlas_builder_add_metadata(builder,
                                                             key->bytes,
                                                             value->bytes);
            if (status != ATLAS_OK) {
                return status;
            }
            continue;
        }
        atlas_builder_metadata *existing = &builder->metadata[builder_index];
        if (strcmp(existing->value, value->bytes) == 0) {
            continue;
        }
        if (policy == ATLAS_MERGE_REJECT_CONFLICTS) {
            return ATLAS_ERR_CONFLICT;
        }
        if (policy == ATLAS_MERGE_PREFER_RIGHT) {
            atlas_status status = atlas_builder_set_metadata(builder,
                                                             key->bytes,
                                                             value->bytes);
            if (status != ATLAS_OK) {
                return status;
            }
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_document_merge(atlas_context *context,
                                  const atlas_document *left,
                                  const atlas_document *right,
                                  atlas_merge_policy policy,
                                  atlas_document **out_document) {
    if (context == NULL || left == NULL || right == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_document = NULL;
    if (policy != ATLAS_MERGE_REJECT_CONFLICTS &&
        policy != ATLAS_MERGE_PREFER_LEFT &&
        policy != ATLAS_MERGE_PREFER_RIGHT) {
        atlas_set_error(context, "unknown merge policy %d", (int)policy);
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_builder *builder = NULL;
    atlas_status status = atlas_builder_from_document(left, &builder);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_merge_nodes(builder, right, policy);
    if (status == ATLAS_OK) status = atlas_merge_edges(builder, right);
    if (status == ATLAS_OK) status = atlas_merge_attributes(builder, right, policy);
    if (status == ATLAS_OK) status = atlas_merge_metadata(builder, right, policy);
    if (status == ATLAS_ERR_CONFLICT) {
        atlas_set_error(context, "documents contain a merge conflict");
    }
    if (status == ATLAS_OK) {
        atlas_parse_options options = left->options;
        status = atlas_builder_parse(context, builder, &options, out_document);
    }
    atlas_builder_destroy(builder);
    return status;
}
