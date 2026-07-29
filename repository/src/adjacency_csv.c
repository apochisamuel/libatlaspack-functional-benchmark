#include "internal.h"

static size_t atlas_adjacency_count(const atlas_document *document,
                                    size_t source_index,
                                    size_t target_index) {
    const atlas_node *source = &document->nodes[source_index];
    size_t count = 0;
    for (size_t ordinal = 0; ordinal < source->outgoing_count; ++ordinal) {
        size_t edge_index = document->edge_order[source->outgoing_start + ordinal];
        if (document->edges[edge_index].target_index == target_index) {
            ++count;
        }
    }
    return count;
}

atlas_status atlas_document_adjacency_csv(atlas_context *context,
                                          const atlas_document *document,
                                          char **out_text,
                                          size_t *out_length) {
    if (context == NULL || document == NULL || out_text == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_text = NULL;
    *out_length = 0;
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    atlas_status status = atlas_text_builder_append(context, &builder, "node");
    for (size_t column = 0; column < document->node_count && status == ATLAS_OK; ++column) {
        const atlas_node *node = &document->nodes[column];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_append_char(context, &builder, ',');
        if (status == ATLAS_OK) {
            status = atlas_text_builder_csv_field(context,
                                                  &builder,
                                                  name->bytes,
                                                  name->length);
        }
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, &builder, '\n');
    }
    for (size_t row = 0; row < document->node_count && status == ATLAS_OK; ++row) {
        const atlas_node *node = &document->nodes[row];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_csv_field(context,
                                              &builder,
                                              name->bytes,
                                              name->length);
        for (size_t column = 0;
             column < document->node_count && status == ATLAS_OK;
             ++column) {
            size_t count = atlas_adjacency_count(document, row, column);
            status = atlas_text_builder_printf(context, &builder, ",%zu", count);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, &builder, '\n');
        }
    }
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) {
            status = ATLAS_ERR_NOMEM;
        }
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
