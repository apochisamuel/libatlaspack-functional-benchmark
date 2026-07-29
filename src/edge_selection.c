#include "internal.h"

atlas_status atlas_document_select_edges_by_label(
    atlas_context *context,
    const atlas_document *document,
    const char *label,
    atlas_edge_selection **out_selection) {
    if (context == NULL || document == NULL || label == NULL ||
        out_selection == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_selection = NULL;
    atlas_edge_selection *selection = atlas_calloc(context, 1, sizeof(*selection));
    if (selection == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    selection->edge_indices = atlas_calloc(context,
                                           document->edge_count,
                                           sizeof(*selection->edge_indices));
    if (selection->edge_indices == NULL && document->edge_count != 0) {
        free(selection);
        return ATLAS_ERR_NOMEM;
    }
    selection->document = document;
    size_t label_length = strlen(label);
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        const atlas_string *candidate = &document->strings[edge->label_index];
        if (candidate->length == label_length &&
            memcmp(candidate->bytes, label, label_length) == 0) {
            selection->edge_indices[selection->count++] = edge_index;
        }
    }
    *out_selection = selection;
    return ATLAS_OK;
}

void atlas_edge_selection_destroy(atlas_edge_selection *selection) {
    if (selection == NULL) {
        return;
    }
    free(selection->edge_indices);
    free(selection);
}

size_t atlas_edge_selection_count(const atlas_edge_selection *selection) {
    return selection == NULL ? 0 : selection->count;
}

atlas_status atlas_edge_selection_edge_at(const atlas_edge_selection *selection,
                                          size_t index,
                                          atlas_edge_view *out_view) {
    if (selection == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= selection->count) {
        return ATLAS_ERR_RANGE;
    }
    return atlas_document_edge_at(selection->document,
                                  selection->edge_indices[index],
                                  out_view);
}
