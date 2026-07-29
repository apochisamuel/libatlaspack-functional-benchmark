#include "internal.h"

static size_t power_of_two(size_t value) {
    size_t result = 8;
    while (result < value) {
        if (result > SIZE_MAX / 2) {
            return 0;
        }
        result *= 2;
    }
    return result;
}

static int atlas_strings_equal_at(const atlas_document *document,
                                  size_t left_index,
                                  size_t right_index) {
    const atlas_string *left = &document->strings[left_index];
    const atlas_string *right = &document->strings[right_index];
    return left->hash == right->hash &&
           left->length == right->length &&
           memcmp(left->bytes, right->bytes, left->length) == 0;
}

static atlas_status atlas_validate_node_names(atlas_context *context,
                                              const atlas_document *document) {
    for (size_t index = 0; index < document->node_count; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        if (name->length == 0) {
            atlas_set_error(context, "node %u has an empty name", node->id);
            return ATLAS_ERR_FORMAT;
        }
        if (!atlas_utf8_valid((const unsigned char *)name->bytes,
                              name->length)) {
            atlas_set_error(context,
                            "node %u has a non-UTF-8 name",
                            node->id);
            return ATLAS_ERR_FORMAT;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_validate_attribute_slices(atlas_context *context,
                                                    const atlas_document *document) {
    unsigned char *seen = NULL;
    if (document->attribute_count != 0) {
        seen = atlas_calloc(context,
                            document->attribute_count,
                            sizeof(*seen));
        if (seen == NULL) {
            return ATLAS_ERR_NOMEM;
        }
    }
    atlas_status status = ATLAS_OK;
    for (size_t node_index = 0;
         node_index < document->node_count && status == ATLAS_OK;
         ++node_index) {
        const atlas_node *node = &document->nodes[node_index];
        for (size_t ordinal = 0; ordinal < node->attribute_count; ++ordinal) {
            size_t attribute_index = node->first_attribute + ordinal;
            const atlas_attribute *attribute = &document->attributes[attribute_index];
            if (attribute->node_id != node->id ||
                attribute->node_index != node_index) {
                atlas_set_error(context,
                                "node %u attribute slice is inconsistent at entry %zu",
                                node->id,
                                attribute_index);
                status = ATLAS_ERR_FORMAT;
                break;
            }
            if (seen[attribute_index]) {
                atlas_set_error(context,
                                "attribute %zu belongs to more than one node slice",
                                attribute_index);
                status = ATLAS_ERR_FORMAT;
                break;
            }
            seen[attribute_index] = 1;
        }
    }
    for (size_t index = 0;
         index < document->attribute_count && status == ATLAS_OK;
         ++index) {
        if (!seen[index]) {
            atlas_set_error(context,
                            "attribute %zu is not covered by a node slice",
                            index);
            status = ATLAS_ERR_FORMAT;
        }
    }
    free(seen);
    return status;
}

static atlas_status atlas_validate_attribute_text(atlas_context *context,
                                                  const atlas_document *document) {
    for (size_t index = 0; index < document->attribute_count; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        if (key->length == 0) {
            atlas_set_error(context,
                            "attribute %zu has an empty key",
                            index);
            return ATLAS_ERR_FORMAT;
        }
        if (!atlas_utf8_valid((const unsigned char *)key->bytes, key->length) ||
            !atlas_utf8_valid((const unsigned char *)value->bytes, value->length)) {
            atlas_set_error(context,
                            "attribute %zu contains non-UTF-8 text",
                            index);
            return ATLAS_ERR_FORMAT;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_validate_metadata_text(atlas_context *context,
                                                 const atlas_document *document) {
    for (size_t index = 0; index < document->metadata_count; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        if (key->length == 0) {
            atlas_set_error(context,
                            "metadata %zu has an empty key",
                            index);
            return ATLAS_ERR_FORMAT;
        }
        if (!atlas_utf8_valid((const unsigned char *)key->bytes, key->length) ||
            !atlas_utf8_valid((const unsigned char *)value->bytes, value->length)) {
            atlas_set_error(context,
                            "metadata %zu contains non-UTF-8 text",
                            index);
            return ATLAS_ERR_FORMAT;
        }
        if (!document->options.require_metadata_keys_unique) {
            continue;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (atlas_strings_equal_at(document,
                                       metadata->key_index,
                                       document->metadata[previous].key_index)) {
                atlas_set_error(context,
                                "metadata key '%s' occurs more than once",
                                key->bytes);
                return ATLAS_ERR_DUPLICATE;
            }
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_validate_edge_labels(atlas_context *context,
                                               const atlas_document *document) {
    for (size_t index = 0; index < document->edge_count; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        if (label->length == 0) {
            atlas_set_error(context,
                            "edge %zu has an empty label",
                            index);
            return ATLAS_ERR_FORMAT;
        }
        if (!atlas_utf8_valid((const unsigned char *)label->bytes,
                              label->length)) {
            atlas_set_error(context,
                            "edge %zu has a non-UTF-8 label",
                            index);
            return ATLAS_ERR_FORMAT;
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_validate_document(atlas_context *context,
                                     const atlas_document *document) {
    if (context == NULL || document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (document->string_count == 0) {
        atlas_set_error(context, "document has no strings");
        return ATLAS_ERR_FORMAT;
    }
    if (document->node_count == 0) {
        atlas_set_error(context, "document has no nodes");
        return ATLAS_ERR_FORMAT;
    }
    atlas_status status = atlas_validate_node_names(context, document);
    if (status == ATLAS_OK) {
        status = atlas_validate_attribute_slices(context, document);
    }
    if (status == ATLAS_OK) {
        status = atlas_validate_attribute_text(context, document);
    }
    if (status == ATLAS_OK) {
        status = atlas_validate_metadata_text(context, document);
    }
    if (status == ATLAS_OK) {
        status = atlas_validate_edge_labels(context, document);
    }
    return status;
}

atlas_status atlas_recover_optional(atlas_context *context,
                                    atlas_document *document) {
    if (context == NULL || document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    return ATLAS_OK;
}

atlas_status atlas_build_name_index(atlas_context *context,
                                    atlas_document *document) {
    if (context == NULL || document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t capacity = power_of_two(document->node_count * 2 + 1);
    if (capacity == 0) {
        atlas_set_error(context, "name index capacity overflow");
        return ATLAS_ERR_RANGE;
    }
    atlas_name_slot *slots = atlas_calloc(context,
                                          capacity,
                                          sizeof(*slots));
    if (slots == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        size_t string_index = document->nodes[index].name_index;
        const atlas_string *string = &document->strings[string_index];
        size_t mask = capacity - 1;
        size_t position = (size_t)string->hash & mask;
        while (slots[position].used) {
            const atlas_string *existing =
                &document->strings[slots[position].string_index];
            if (existing->hash == string->hash &&
                existing->length == string->length &&
                memcmp(existing->bytes,
                       string->bytes,
                       string->length) == 0) {
                free(slots);
                atlas_set_error(context,
                                "duplicate canonical node name '%s'",
                                string->bytes);
                return ATLAS_ERR_DUPLICATE;
            }
            position = (position + 1) & mask;
        }
        slots[position].used = 1;
        slots[position].hash = string->hash;
        slots[position].string_index = string_index;
        slots[position].node_index = index;
    }
    document->name_index = slots;
    document->name_index_capacity = capacity;
    return ATLAS_OK;
}
