#include "internal.h"

atlas_status atlas_document_find_attribute(const atlas_document *document,
                                           uint32_t node_id,
                                           const char *key,
                                           atlas_attribute_view *out_view) {
    if (document == NULL || key == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    const atlas_node *node = &document->nodes[node_index];
    size_t key_length = strlen(key);
    for (size_t ordinal = 0; ordinal < node->attribute_count; ++ordinal) {
        size_t attribute_index = node->first_attribute + ordinal;
        const atlas_attribute *attribute = &document->attributes[attribute_index];
        const atlas_string *candidate = &document->strings[attribute->key_index];
        if (candidate->length == key_length &&
            memcmp(candidate->bytes, key, key_length) == 0) {
            return atlas_document_attribute_at(document, attribute_index, out_view);
        }
    }
    return ATLAS_ERR_NOT_FOUND;
}
