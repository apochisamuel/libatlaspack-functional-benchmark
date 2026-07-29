#include "internal.h"

static size_t atlas_builder_find_node_index(const atlas_builder *builder, uint32_t node_id) {
    for (size_t index = 0; index < builder->node_count; ++index) {
        if (builder->nodes[index].id == node_id) {
            return index;
        }
    }
    return ATLAS_NO_INDEX;
}

atlas_status atlas_builder_rename_node(atlas_builder *builder,
                                       uint32_t node_id,
                                       const char *new_name) {
    if (builder == NULL || new_name == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_builder_find_node_index(builder, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    size_t length = strlen(new_name);
    if (length == 0 || !atlas_utf8_valid((const unsigned char *)new_name, length)) {
        return ATLAS_ERR_FORMAT;
    }
    char *copy = atlas_strdup_plain(new_name);
    if (copy == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    free(builder->nodes[node_index].name);
    builder->nodes[node_index].name = copy;
    return ATLAS_OK;
}

static void atlas_builder_remove_edge_at(atlas_builder *builder, size_t index) {
    free(builder->edges[index].label);
    if (index + 1 < builder->edge_count) {
        memmove(&builder->edges[index],
                &builder->edges[index + 1],
                (builder->edge_count - index - 1) * sizeof(*builder->edges));
    }
    --builder->edge_count;
}

static void atlas_builder_remove_attribute_at(atlas_builder *builder, size_t index) {
    free(builder->attributes[index].key);
    free(builder->attributes[index].value);
    if (index + 1 < builder->attribute_count) {
        memmove(&builder->attributes[index],
                &builder->attributes[index + 1],
                (builder->attribute_count - index - 1) * sizeof(*builder->attributes));
    }
    --builder->attribute_count;
}

atlas_status atlas_builder_remove_node(atlas_builder *builder,
                                       uint32_t node_id,
                                       int remove_incident_edges) {
    if (builder == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_builder_find_node_index(builder, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    if (!remove_incident_edges) {
        for (size_t index = 0; index < builder->edge_count; ++index) {
            if (builder->edges[index].source_id == node_id ||
                builder->edges[index].target_id == node_id) {
                return ATLAS_ERR_CONFLICT;
            }
        }
    }
    for (size_t index = builder->edge_count; index > 0; --index) {
        const atlas_builder_edge *edge = &builder->edges[index - 1];
        if (edge->source_id == node_id || edge->target_id == node_id) {
            atlas_builder_remove_edge_at(builder, index - 1);
        }
    }
    for (size_t index = builder->attribute_count; index > 0; --index) {
        if (builder->attributes[index - 1].node_id == node_id) {
            atlas_builder_remove_attribute_at(builder, index - 1);
        }
    }
    free(builder->nodes[node_index].name);
    if (node_index + 1 < builder->node_count) {
        memmove(&builder->nodes[node_index],
                &builder->nodes[node_index + 1],
                (builder->node_count - node_index - 1) * sizeof(*builder->nodes));
    }
    --builder->node_count;
    return ATLAS_OK;
}

atlas_status atlas_builder_remove_edge(atlas_builder *builder,
                                       uint32_t source_id,
                                       uint32_t target_id,
                                       const char *label) {
    if (builder == NULL || label == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    for (size_t index = 0; index < builder->edge_count; ++index) {
        atlas_builder_edge *edge = &builder->edges[index];
        if (edge->source_id == source_id && edge->target_id == target_id &&
            strcmp(edge->label, label) == 0) {
            atlas_builder_remove_edge_at(builder, index);
            return ATLAS_OK;
        }
    }
    return ATLAS_ERR_NOT_FOUND;
}

atlas_status atlas_builder_set_attribute(atlas_builder *builder,
                                         uint32_t node_id,
                                         const char *key,
                                         const char *value) {
    if (builder == NULL || key == NULL || value == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (atlas_builder_find_node_index(builder, node_id) == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    for (size_t index = 0; index < builder->attribute_count; ++index) {
        atlas_builder_attribute *attribute = &builder->attributes[index];
        if (attribute->node_id == node_id && strcmp(attribute->key, key) == 0) {
            char *copy = atlas_strdup_plain(value);
            if (copy == NULL) {
                return ATLAS_ERR_NOMEM;
            }
            free(attribute->value);
            attribute->value = copy;
            return ATLAS_OK;
        }
    }
    return atlas_builder_add_attribute(builder, node_id, key, value);
}

atlas_status atlas_builder_remove_attribute(atlas_builder *builder,
                                            uint32_t node_id,
                                            const char *key) {
    if (builder == NULL || key == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    for (size_t index = 0; index < builder->attribute_count; ++index) {
        if (builder->attributes[index].node_id == node_id &&
            strcmp(builder->attributes[index].key, key) == 0) {
            atlas_builder_remove_attribute_at(builder, index);
            return ATLAS_OK;
        }
    }
    return ATLAS_ERR_NOT_FOUND;
}

atlas_status atlas_builder_set_metadata(atlas_builder *builder,
                                        const char *key,
                                        const char *value) {
    if (builder == NULL || key == NULL || value == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    for (size_t index = 0; index < builder->metadata_count; ++index) {
        atlas_builder_metadata *metadata = &builder->metadata[index];
        if (strcmp(metadata->key, key) == 0) {
            char *copy = atlas_strdup_plain(value);
            if (copy == NULL) {
                return ATLAS_ERR_NOMEM;
            }
            free(metadata->value);
            metadata->value = copy;
            return ATLAS_OK;
        }
    }
    return atlas_builder_add_metadata(builder, key, value);
}

atlas_status atlas_builder_remove_metadata(atlas_builder *builder,
                                           const char *key) {
    if (builder == NULL || key == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    for (size_t index = 0; index < builder->metadata_count; ++index) {
        if (strcmp(builder->metadata[index].key, key) == 0) {
            free(builder->metadata[index].key);
            free(builder->metadata[index].value);
            if (index + 1 < builder->metadata_count) {
                memmove(&builder->metadata[index],
                        &builder->metadata[index + 1],
                        (builder->metadata_count - index - 1) *
                            sizeof(*builder->metadata));
            }
            --builder->metadata_count;
            return ATLAS_OK;
        }
    }
    return ATLAS_ERR_NOT_FOUND;
}
