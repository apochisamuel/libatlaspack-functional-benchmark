#include "internal.h"

static int atlas_document_has_node_id(const atlas_document *document, uint32_t id) {
    return atlas_node_index_by_id(document, id) != ATLAS_NO_INDEX;
}

static int atlas_document_has_edge_value(const atlas_document *document,
                                         uint32_t source_id,
                                         uint32_t target_id,
                                         const char *label,
                                         size_t label_length) {
    for (size_t index = 0; index < document->edge_count; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *candidate = &document->strings[edge->label_index];
        if (edge->source_id == source_id && edge->target_id == target_id &&
            candidate->length == label_length &&
            memcmp(candidate->bytes, label, label_length) == 0) {
            return 1;
        }
    }
    return 0;
}

static int atlas_document_has_attribute_value(const atlas_document *document,
                                              uint32_t node_id,
                                              const char *key,
                                              size_t key_length,
                                              const char *value,
                                              size_t value_length) {
    for (size_t index = 0; index < document->attribute_count; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *candidate_key = &document->strings[attribute->key_index];
        const atlas_string *candidate_value = &document->strings[attribute->value_index];
        if (attribute->node_id == node_id &&
            candidate_key->length == key_length &&
            candidate_value->length == value_length &&
            memcmp(candidate_key->bytes, key, key_length) == 0 &&
            memcmp(candidate_value->bytes, value, value_length) == 0) {
            return 1;
        }
    }
    return 0;
}

static size_t atlas_diff_changed_nodes(const atlas_document *before,
                                       const atlas_document *after) {
    size_t changed = 0;
    for (size_t index = 0; index < before->node_count; ++index) {
        const atlas_node *before_node = &before->nodes[index];
        size_t after_index = atlas_node_index_by_id(after, before_node->id);
        if (after_index == ATLAS_NO_INDEX) {
            continue;
        }
        const atlas_node *after_node = &after->nodes[after_index];
        const atlas_string *before_name = &before->strings[before_node->name_index];
        const atlas_string *after_name = &after->strings[after_node->name_index];
        if (before_name->length != after_name->length ||
            memcmp(before_name->bytes, after_name->bytes, before_name->length) != 0 ||
            before_node->attribute_count != after_node->attribute_count) {
            ++changed;
        }
    }
    return changed;
}

static size_t atlas_diff_changed_metadata(const atlas_document *before,
                                          const atlas_document *after) {
    size_t changed = 0;
    for (size_t index = 0; index < before->metadata_count; ++index) {
        atlas_metadata_view before_view;
        if (atlas_document_metadata_at(before, index, &before_view) != ATLAS_OK) {
            continue;
        }
        atlas_metadata_view after_view;
        atlas_status status = atlas_document_find_metadata(after,
                                                           before_view.key,
                                                           &after_view);
        if (status == ATLAS_ERR_NOT_FOUND ||
            (status == ATLAS_OK &&
             (before_view.value_length != after_view.value_length ||
              memcmp(before_view.value,
                     after_view.value,
                     before_view.value_length) != 0))) {
            ++changed;
        }
    }
    for (size_t index = 0; index < after->metadata_count; ++index) {
        atlas_metadata_view after_view;
        if (atlas_document_metadata_at(after, index, &after_view) != ATLAS_OK) {
            continue;
        }
        atlas_metadata_view ignored;
        if (atlas_document_find_metadata(before, after_view.key, &ignored) ==
            ATLAS_ERR_NOT_FOUND) {
            ++changed;
        }
    }
    return changed;
}

atlas_status atlas_diff_compute(atlas_context *context,
                                const atlas_document *before,
                                const atlas_document *after,
                                atlas_diff **out_diff) {
    if (context == NULL || before == NULL || after == NULL || out_diff == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_diff = NULL;
    atlas_diff *diff = atlas_calloc(context, 1, sizeof(*diff));
    if (diff == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < before->node_count; ++index) {
        if (!atlas_document_has_node_id(after, before->nodes[index].id)) {
            ++diff->summary.removed_nodes;
        }
    }
    for (size_t index = 0; index < after->node_count; ++index) {
        if (!atlas_document_has_node_id(before, after->nodes[index].id)) {
            ++diff->summary.added_nodes;
        }
    }
    diff->summary.changed_nodes = atlas_diff_changed_nodes(before, after);
    for (size_t index = 0; index < before->edge_count; ++index) {
        const atlas_edge *edge = &before->edges[index];
        const atlas_string *label = &before->strings[edge->label_index];
        if (!atlas_document_has_edge_value(after,
                                           edge->source_id,
                                           edge->target_id,
                                           label->bytes,
                                           label->length)) {
            ++diff->summary.removed_edges;
        }
    }
    for (size_t index = 0; index < after->edge_count; ++index) {
        const atlas_edge *edge = &after->edges[index];
        const atlas_string *label = &after->strings[edge->label_index];
        if (!atlas_document_has_edge_value(before,
                                           edge->source_id,
                                           edge->target_id,
                                           label->bytes,
                                           label->length)) {
            ++diff->summary.added_edges;
        }
    }
    for (size_t index = 0; index < before->attribute_count; ++index) {
        const atlas_attribute *attribute = &before->attributes[index];
        const atlas_string *key = &before->strings[attribute->key_index];
        const atlas_string *value = &before->strings[attribute->value_index];
        if (!atlas_document_has_attribute_value(after,
                                                attribute->node_id,
                                                key->bytes,
                                                key->length,
                                                value->bytes,
                                                value->length)) {
            ++diff->summary.removed_attributes;
        }
    }
    for (size_t index = 0; index < after->attribute_count; ++index) {
        const atlas_attribute *attribute = &after->attributes[index];
        const atlas_string *key = &after->strings[attribute->key_index];
        const atlas_string *value = &after->strings[attribute->value_index];
        if (!atlas_document_has_attribute_value(before,
                                                attribute->node_id,
                                                key->bytes,
                                                key->length,
                                                value->bytes,
                                                value->length)) {
            ++diff->summary.added_attributes;
        }
    }
    diff->summary.changed_metadata = atlas_diff_changed_metadata(before, after);
    *out_diff = diff;
    return ATLAS_OK;
}

void atlas_diff_destroy(atlas_diff *diff) {
    free(diff);
}

atlas_diff_summary atlas_diff_get_summary(const atlas_diff *diff) {
    atlas_diff_summary summary;
    memset(&summary, 0, sizeof(summary));
    if (diff != NULL) {
        summary = diff->summary;
    }
    return summary;
}
