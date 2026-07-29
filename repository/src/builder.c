#include "internal.h"

static atlas_status atlas_builder_reserve_nodes(atlas_builder *builder, size_t additional) {
    if (additional > SIZE_MAX - builder->node_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = builder->node_count + additional;
    if (required <= builder->node_capacity) {
        return ATLAS_OK;
    }
    size_t capacity = builder->node_capacity == 0 ? 8 : builder->node_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_builder_node *replacement =
        (atlas_builder_node *)realloc(builder->nodes,
                                      capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    builder->nodes = replacement;
    builder->node_capacity = capacity;
    return ATLAS_OK;
}

static atlas_status atlas_builder_reserve_edges(atlas_builder *builder, size_t additional) {
    if (additional > SIZE_MAX - builder->edge_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = builder->edge_count + additional;
    if (required <= builder->edge_capacity) {
        return ATLAS_OK;
    }
    size_t capacity = builder->edge_capacity == 0 ? 8 : builder->edge_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_builder_edge *replacement =
        (atlas_builder_edge *)realloc(builder->edges,
                                      capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    builder->edges = replacement;
    builder->edge_capacity = capacity;
    return ATLAS_OK;
}

static atlas_status atlas_builder_reserve_attributes(atlas_builder *builder,
                                                     size_t additional) {
    if (additional > SIZE_MAX - builder->attribute_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = builder->attribute_count + additional;
    if (required <= builder->attribute_capacity) {
        return ATLAS_OK;
    }
    size_t capacity = builder->attribute_capacity == 0 ? 8 : builder->attribute_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_builder_attribute *replacement =
        (atlas_builder_attribute *)realloc(builder->attributes,
                                           capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    builder->attributes = replacement;
    builder->attribute_capacity = capacity;
    return ATLAS_OK;
}

static atlas_status atlas_builder_reserve_metadata(atlas_builder *builder,
                                                   size_t additional) {
    if (additional > SIZE_MAX - builder->metadata_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = builder->metadata_count + additional;
    if (required <= builder->metadata_capacity) {
        return ATLAS_OK;
    }
    size_t capacity = builder->metadata_capacity == 0 ? 8 : builder->metadata_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_builder_metadata *replacement =
        (atlas_builder_metadata *)realloc(builder->metadata,
                                          capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    builder->metadata = replacement;
    builder->metadata_capacity = capacity;
    return ATLAS_OK;
}

atlas_builder *atlas_builder_create(void) {
    return (atlas_builder *)calloc(1, sizeof(atlas_builder));
}

void atlas_builder_clear(atlas_builder *builder) {
    if (builder == NULL) {
        return;
    }
    for (size_t index = 0; index < builder->node_count; ++index) {
        free(builder->nodes[index].name);
    }
    for (size_t index = 0; index < builder->edge_count; ++index) {
        free(builder->edges[index].label);
    }
    for (size_t index = 0; index < builder->attribute_count; ++index) {
        free(builder->attributes[index].key);
        free(builder->attributes[index].value);
    }
    for (size_t index = 0; index < builder->metadata_count; ++index) {
        free(builder->metadata[index].key);
        free(builder->metadata[index].value);
    }
    builder->node_count = 0;
    builder->edge_count = 0;
    builder->attribute_count = 0;
    builder->metadata_count = 0;
}

void atlas_builder_destroy(atlas_builder *builder) {
    if (builder == NULL) {
        return;
    }
    atlas_builder_clear(builder);
    free(builder->nodes);
    free(builder->edges);
    free(builder->attributes);
    free(builder->metadata);
    free(builder);
}

static int atlas_builder_has_node(const atlas_builder *builder, uint32_t id) {
    for (size_t index = 0; index < builder->node_count; ++index) {
        if (builder->nodes[index].id == id) {
            return 1;
        }
    }
    return 0;
}

atlas_status atlas_builder_add_node(atlas_builder *builder,
                                    uint32_t id,
                                    const char *name) {
    if (builder == NULL || name == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t length = strlen(name);
    if (length == 0 || !atlas_utf8_valid((const unsigned char *)name, length)) {
        return ATLAS_ERR_FORMAT;
    }
    if (atlas_builder_has_node(builder, id)) {
        return ATLAS_ERR_DUPLICATE;
    }
    atlas_status status = atlas_builder_reserve_nodes(builder, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    char *copy = atlas_strdup_plain(name);
    if (copy == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_builder_node *node = &builder->nodes[builder->node_count++];
    node->id = id;
    node->name = copy;
    return ATLAS_OK;
}

atlas_status atlas_builder_add_edge(atlas_builder *builder,
                                    uint32_t source_id,
                                    uint32_t target_id,
                                    const char *label) {
    if (builder == NULL || label == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t length = strlen(label);
    if (!atlas_utf8_valid((const unsigned char *)label, length)) {
        return ATLAS_ERR_FORMAT;
    }
    atlas_status status = atlas_builder_reserve_edges(builder, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    char *copy = atlas_strdup_plain(label);
    if (copy == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_builder_edge *edge = &builder->edges[builder->edge_count++];
    edge->source_id = source_id;
    edge->target_id = target_id;
    edge->label = copy;
    return ATLAS_OK;
}

atlas_status atlas_builder_add_attribute(atlas_builder *builder,
                                         uint32_t node_id,
                                         const char *key,
                                         const char *value) {
    if (builder == NULL || key == NULL || value == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t key_length = strlen(key);
    size_t value_length = strlen(value);
    if (key_length == 0 ||
        !atlas_utf8_valid((const unsigned char *)key, key_length) ||
        !atlas_utf8_valid((const unsigned char *)value, value_length)) {
        return ATLAS_ERR_FORMAT;
    }
    atlas_status status = atlas_builder_reserve_attributes(builder, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    char *key_copy = atlas_strdup_plain(key);
    char *value_copy = atlas_strdup_plain(value);
    if (key_copy == NULL || value_copy == NULL) {
        free(key_copy);
        free(value_copy);
        return ATLAS_ERR_NOMEM;
    }
    atlas_builder_attribute *attribute =
        &builder->attributes[builder->attribute_count++];
    attribute->node_id = node_id;
    attribute->key = key_copy;
    attribute->value = value_copy;
    return ATLAS_OK;
}

atlas_status atlas_builder_add_metadata(atlas_builder *builder,
                                        const char *key,
                                        const char *value) {
    if (builder == NULL || key == NULL || value == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t key_length = strlen(key);
    size_t value_length = strlen(value);
    if (key_length == 0 ||
        !atlas_utf8_valid((const unsigned char *)key, key_length) ||
        !atlas_utf8_valid((const unsigned char *)value, value_length)) {
        return ATLAS_ERR_FORMAT;
    }
    for (size_t index = 0; index < builder->metadata_count; ++index) {
        if (strcmp(builder->metadata[index].key, key) == 0) {
            return ATLAS_ERR_DUPLICATE;
        }
    }
    atlas_status status = atlas_builder_reserve_metadata(builder, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    char *key_copy = atlas_strdup_plain(key);
    char *value_copy = atlas_strdup_plain(value);
    if (key_copy == NULL || value_copy == NULL) {
        free(key_copy);
        free(value_copy);
        return ATLAS_ERR_NOMEM;
    }
    atlas_builder_metadata *metadata = &builder->metadata[builder->metadata_count++];
    metadata->key = key_copy;
    metadata->value = value_copy;
    return ATLAS_OK;
}

size_t atlas_builder_node_count(const atlas_builder *builder) {
    return builder == NULL ? 0 : builder->node_count;
}

size_t atlas_builder_edge_count(const atlas_builder *builder) {
    return builder == NULL ? 0 : builder->edge_count;
}

size_t atlas_builder_attribute_count(const atlas_builder *builder) {
    return builder == NULL ? 0 : builder->attribute_count;
}

size_t atlas_builder_metadata_count(const atlas_builder *builder) {
    return builder == NULL ? 0 : builder->metadata_count;
}

atlas_status atlas_builder_validate(const atlas_builder *builder, atlas_context *context) {
    if (builder == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (builder->node_count == 0) {
        atlas_set_error(context, "builder must contain at least one node");
        return ATLAS_ERR_FORMAT;
    }
    if (builder->node_count > ATLAS_MAX_NODES ||
        builder->edge_count > ATLAS_MAX_EDGES ||
        builder->attribute_count > ATLAS_MAX_ATTRIBUTES ||
        builder->metadata_count > ATLAS_MAX_METADATA) {
        atlas_set_error(context, "builder exceeds ATPK count limits");
        return ATLAS_ERR_RANGE;
    }
    for (size_t edge_index = 0; edge_index < builder->edge_count; ++edge_index) {
        const atlas_builder_edge *edge = &builder->edges[edge_index];
        if (!atlas_builder_has_node(builder, edge->source_id) ||
            !atlas_builder_has_node(builder, edge->target_id)) {
            atlas_set_error(context,
                            "edge %zu refers to missing nodes %u or %u",
                            edge_index,
                            edge->source_id,
                            edge->target_id);
            return ATLAS_ERR_REFERENCE;
        }
    }
    for (size_t attribute_index = 0;
         attribute_index < builder->attribute_count;
         ++attribute_index) {
        const atlas_builder_attribute *attribute =
            &builder->attributes[attribute_index];
        if (!atlas_builder_has_node(builder, attribute->node_id)) {
            atlas_set_error(context,
                            "attribute %zu refers to missing node %u",
                            attribute_index,
                            attribute->node_id);
            return ATLAS_ERR_REFERENCE;
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_builder_from_document(const atlas_document *document,
                                         atlas_builder **out_builder) {
    if (document == NULL || out_builder == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_builder = NULL;
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
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_builder_add_edge(builder,
                                        edge->source_id,
                                        edge->target_id,
                                        label->bytes);
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
    if (status != ATLAS_OK) {
        atlas_builder_destroy(builder);
        return status;
    }
    *out_builder = builder;
    return ATLAS_OK;
}
