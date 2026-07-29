#include "internal.h"

static unsigned char atlas_ascii_fold(unsigned char value) {
    if (value >= 'A' && value <= 'Z') {
        return (unsigned char)(value - 'A' + 'a');
    }
    return value;
}

static int atlas_contains_text(const char *haystack,
                               size_t haystack_length,
                               const char *needle,
                               size_t needle_length,
                               int case_sensitive) {
    if (needle_length == 0) {
        return 1;
    }
    if (needle_length > haystack_length) {
        return 0;
    }
    for (size_t start = 0; start <= haystack_length - needle_length; ++start) {
        size_t offset = 0;
        for (; offset < needle_length; ++offset) {
            unsigned char left = (unsigned char)haystack[start + offset];
            unsigned char right = (unsigned char)needle[offset];
            if (!case_sensitive) {
                left = atlas_ascii_fold(left);
                right = atlas_ascii_fold(right);
            }
            if (left != right) {
                break;
            }
        }
        if (offset == needle_length) {
            return 1;
        }
    }
    return 0;
}

atlas_status atlas_document_search_names(atlas_context *context,
                                         const atlas_document *document,
                                         const char *needle,
                                         int case_sensitive,
                                         atlas_selection **out_selection) {
    if (context == NULL || document == NULL || needle == NULL ||
        out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    size_t needle_length = strlen(needle);
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
    for (size_t index = 0; index < document->node_count; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        if (atlas_contains_text(name->bytes,
                                name->length,
                                needle,
                                needle_length,
                                case_sensitive)) {
            selection->node_indices[selection->count++] = index;
        }
    }
    *out_selection = selection;
    return ATLAS_OK;
}

atlas_status atlas_document_dependency_order(atlas_context *context,
                                             const atlas_document *document,
                                             uint32_t node_id,
                                             atlas_selection **out_selection) {
    if (context == NULL || document == NULL || out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    size_t target_index = atlas_node_index_by_id(document, node_id);
    if (target_index == ATLAS_NO_INDEX) {
        atlas_set_error(context, "dependency target node %u does not exist", node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    atlas_plan *plan = NULL;
    atlas_status status = atlas_plan_build(context, document, &plan);
    if (status != ATLAS_OK) {
        return status;
    }
    unsigned char *ancestor = atlas_calloc(context,
                                           document->node_count,
                                           sizeof(*ancestor));
    size_t *queue = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*queue));
    if ((ancestor == NULL || queue == NULL) && document->node_count != 0) {
        free(ancestor);
        free(queue);
        atlas_plan_destroy(plan);
        return ATLAS_ERR_NOMEM;
    }
    size_t head = 0;
    size_t tail = 0;
    ancestor[target_index] = 1;
    queue[tail++] = target_index;
    while (head < tail) {
        size_t current = queue[head++];
        const atlas_node *node = &document->nodes[current];
        for (size_t ordinal = 0; ordinal < node->incoming_count; ++ordinal) {
            size_t edge_index = document->incoming_order[node->incoming_start + ordinal];
            size_t source = document->edges[edge_index].source_index;
            if (!ancestor[source]) {
                ancestor[source] = 1;
                queue[tail++] = source;
            }
        }
    }
    atlas_selection *selection = atlas_calloc(context, 1, sizeof(*selection));
    if (selection == NULL) {
        free(ancestor);
        free(queue);
        atlas_plan_destroy(plan);
        return ATLAS_ERR_NOMEM;
    }
    selection->node_indices = atlas_calloc(context,
                                           document->node_count,
                                           sizeof(*selection->node_indices));
    if (selection->node_indices == NULL && document->node_count != 0) {
        atlas_selection_destroy(selection);
        free(ancestor);
        free(queue);
        atlas_plan_destroy(plan);
        return ATLAS_ERR_NOMEM;
    }
    selection->document = document;
    for (size_t ordinal = 0; ordinal < plan->count; ++ordinal) {
        size_t node_index = plan->node_order[ordinal];
        if (ancestor[node_index]) {
            selection->node_indices[selection->count++] = node_index;
        }
    }
    free(ancestor);
    free(queue);
    atlas_plan_destroy(plan);
    *out_selection = selection;
    return ATLAS_OK;
}
