#include "internal.h"

static atlas_status atlas_mermaid_label(atlas_context *context,
                                        atlas_text_builder *builder,
                                        const char *text,
                                        size_t length) {
    for (size_t index = 0; index < length; ++index) {
        char value = text[index];
        if (value == '"') {
            atlas_status status = atlas_text_builder_append(context, builder, "&quot;");
            if (status != ATLAS_OK) return status;
        } else if (value == '\r' || value == '\n') {
            atlas_status status = atlas_text_builder_append(context, builder, "<br/>");
            if (status != ATLAS_OK) return status;
        } else if (value == '[' || value == ']' || value == '{' || value == '}') {
            atlas_status status = atlas_text_builder_append_char(context, builder, ' ');
            if (status != ATLAS_OK) return status;
        } else {
            atlas_status status = atlas_text_builder_append_char(context, builder, value);
            if (status != ATLAS_OK) return status;
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_document_to_mermaid(atlas_context *context,
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
    atlas_status status = atlas_text_builder_append(context, &builder, "flowchart LR\n");
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context, &builder, "  n%u[\"", node->id);
        if (status == ATLAS_OK) status = atlas_mermaid_label(context, &builder, name->bytes, name->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "\"]\n");
    }
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "  n%u -->|\"",
                                           edge->source_id);
        if (status == ATLAS_OK) status = atlas_mermaid_label(context, &builder, label->bytes, label->length);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_printf(context,
                                               &builder,
                                               "\"| n%u\n",
                                               edge->target_id);
        }
    }
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
