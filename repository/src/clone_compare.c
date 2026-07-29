#include "internal.h"

atlas_status atlas_document_clone(atlas_context *context,
                                  const atlas_document *document,
                                  atlas_document **out_clone) {
    if (context == NULL || document == NULL || out_clone == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_clone = NULL;
    uint8_t *data = NULL;
    size_t size = 0;
    atlas_status status = atlas_document_serialize(context, document, &data, &size);
    if (status == ATLAS_OK) {
        atlas_parse_options options = document->options;
        status = atlas_parse(context, data, size, &options, out_clone);
    }
    free(data);
    return status;
}

static int atlas_compare_nodes(const atlas_document *left,
                               const atlas_document *right) {
    if (left->node_count != right->node_count) {
        return 0;
    }
    for (size_t index = 0; index < left->node_count; ++index) {
        const atlas_node *left_node = &left->nodes[index];
        const atlas_node *right_node = &right->nodes[index];
        const atlas_string *left_name = &left->strings[left_node->name_index];
        const atlas_string *right_name = &right->strings[right_node->name_index];
        if (left_node->id != right_node->id ||
            left_node->attribute_count != right_node->attribute_count ||
            left_name->length != right_name->length ||
            memcmp(left_name->bytes, right_name->bytes, left_name->length) != 0) {
            return 0;
        }
    }
    return 1;
}

static int atlas_compare_edges(const atlas_document *left,
                               const atlas_document *right) {
    if (left->edge_count != right->edge_count) {
        return 0;
    }
    for (size_t index = 0; index < left->edge_count; ++index) {
        const atlas_edge *left_edge = &left->edges[index];
        const atlas_edge *right_edge = &right->edges[index];
        const atlas_string *left_label = &left->strings[left_edge->label_index];
        const atlas_string *right_label = &right->strings[right_edge->label_index];
        if (left_edge->source_id != right_edge->source_id ||
            left_edge->target_id != right_edge->target_id ||
            left_label->length != right_label->length ||
            memcmp(left_label->bytes, right_label->bytes, left_label->length) != 0) {
            return 0;
        }
    }
    return 1;
}

static int atlas_compare_attributes(const atlas_document *left,
                                    const atlas_document *right) {
    if (left->attribute_count != right->attribute_count) {
        return 0;
    }
    for (size_t index = 0; index < left->attribute_count; ++index) {
        const atlas_attribute *left_attribute = &left->attributes[index];
        const atlas_attribute *right_attribute = &right->attributes[index];
        const atlas_string *left_key = &left->strings[left_attribute->key_index];
        const atlas_string *right_key = &right->strings[right_attribute->key_index];
        const atlas_string *left_value = &left->strings[left_attribute->value_index];
        const atlas_string *right_value = &right->strings[right_attribute->value_index];
        if (left_attribute->node_id != right_attribute->node_id ||
            left_key->length != right_key->length ||
            left_value->length != right_value->length ||
            memcmp(left_key->bytes, right_key->bytes, left_key->length) != 0 ||
            memcmp(left_value->bytes, right_value->bytes, left_value->length) != 0) {
            return 0;
        }
    }
    return 1;
}

static int atlas_compare_metadata(const atlas_document *left,
                                  const atlas_document *right) {
    if (left->metadata_count != right->metadata_count) {
        return 0;
    }
    for (size_t index = 0; index < left->metadata_count; ++index) {
        const atlas_metadata *left_metadata = &left->metadata[index];
        const atlas_metadata *right_metadata = &right->metadata[index];
        const atlas_string *left_key = &left->strings[left_metadata->key_index];
        const atlas_string *right_key = &right->strings[right_metadata->key_index];
        const atlas_string *left_value = &left->strings[left_metadata->value_index];
        const atlas_string *right_value = &right->strings[right_metadata->value_index];
        if (left_key->length != right_key->length ||
            left_value->length != right_value->length ||
            memcmp(left_key->bytes, right_key->bytes, left_key->length) != 0 ||
            memcmp(left_value->bytes, right_value->bytes, left_value->length) != 0) {
            return 0;
        }
    }
    return 1;
}

atlas_status atlas_document_compare(const atlas_document *left,
                                    const atlas_document *right,
                                    atlas_compare_result *out_result) {
    if (left == NULL || right == NULL || out_result == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    int equal = atlas_compare_nodes(left, right) &&
                atlas_compare_edges(left, right) &&
                atlas_compare_attributes(left, right) &&
                atlas_compare_metadata(left, right);
    *out_result = equal ? ATLAS_COMPARE_EQUAL : ATLAS_COMPARE_DIFFERENT;
    return ATLAS_OK;
}
