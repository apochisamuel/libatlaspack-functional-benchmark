#include "internal.h"

atlas_node_filter atlas_node_filter_default(void) {
    atlas_node_filter filter;
    filter.name_prefix = NULL;
    filter.attribute_key = NULL;
    filter.attribute_value = NULL;
    filter.minimum_out_degree = 0;
    filter.maximum_out_degree = SIZE_MAX;
    filter.minimum_in_degree = 0;
    filter.maximum_in_degree = SIZE_MAX;
    filter.include_isolated = 1;
    return filter;
}

static int atlas_selection_attribute_matches(const atlas_document *document,
                                             const atlas_node *node,
                                             const atlas_node_filter *filter) {
    if (filter->attribute_key == NULL) {
        return 1;
    }
    size_t key_length = strlen(filter->attribute_key);
    size_t value_length = filter->attribute_value == NULL
                              ? 0
                              : strlen(filter->attribute_value);
    for (size_t ordinal = 0; ordinal < node->attribute_count; ++ordinal) {
        const atlas_attribute *attribute =
            &document->attributes[node->first_attribute + ordinal];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        if (key->length != key_length ||
            memcmp(key->bytes, filter->attribute_key, key_length) != 0) {
            continue;
        }
        if (filter->attribute_value == NULL) {
            return 1;
        }
        if (value->length == value_length &&
            memcmp(value->bytes, filter->attribute_value, value_length) == 0) {
            return 1;
        }
    }
    return 0;
}

static int atlas_selection_node_matches(const atlas_document *document,
                                        size_t node_index,
                                        const atlas_node_filter *filter) {
    const atlas_node *node = &document->nodes[node_index];
    if (node->outgoing_count < filter->minimum_out_degree ||
        node->outgoing_count > filter->maximum_out_degree ||
        node->incoming_count < filter->minimum_in_degree ||
        node->incoming_count > filter->maximum_in_degree) {
        return 0;
    }
    if (!filter->include_isolated &&
        node->outgoing_count == 0 && node->incoming_count == 0) {
        return 0;
    }
    if (filter->name_prefix != NULL) {
        const atlas_string *name = &document->strings[node->name_index];
        if (!atlas_string_has_prefix(name, filter->name_prefix)) {
            return 0;
        }
    }
    return atlas_selection_attribute_matches(document, node, filter);
}

atlas_status atlas_document_select(atlas_context *context,
                                   const atlas_document *document,
                                   const atlas_node_filter *filter,
                                   atlas_selection **out_selection) {
    if (context == NULL || document == NULL || out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    atlas_node_filter effective = filter == NULL
                                      ? atlas_node_filter_default()
                                      : *filter;
    if (effective.maximum_out_degree < effective.minimum_out_degree ||
        effective.maximum_in_degree < effective.minimum_in_degree) {
        atlas_set_error(context, "selection degree bounds are inconsistent");
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_selection *selection = atlas_calloc(context, 1, sizeof(*selection));
    if (selection == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    selection->node_indices = atlas_calloc(context,
                                           document->node_count,
                                           sizeof(*selection->node_indices));
    if (selection->node_indices == NULL && document->node_count != 0) {
        free(selection);
        return ATLAS_ERR_NOMEM;
    }
    selection->document = document;
    for (size_t node_index = 0; node_index < document->node_count; ++node_index) {
        if (atlas_selection_node_matches(document, node_index, &effective)) {
            selection->node_indices[selection->count++] = node_index;
        }
    }
    *out_selection = selection;
    return ATLAS_OK;
}

void atlas_selection_destroy(atlas_selection *selection) {
    if (selection == NULL) {
        return;
    }
    free(selection->node_indices);
    free(selection);
}

size_t atlas_selection_count(const atlas_selection *selection) {
    return selection == NULL ? 0 : selection->count;
}

atlas_status atlas_selection_node_at(const atlas_selection *selection,
                                     size_t index,
                                     atlas_node_view *out_view) {
    if (selection == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= selection->count) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_node_at(selection->document,
                                  selection->node_indices[index],
                                  out_view);
}
