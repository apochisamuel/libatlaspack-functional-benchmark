#include "internal.h"

static atlas_status atlas_dot_string(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const char *text,
                                     size_t length) {
    atlas_status status = atlas_text_builder_append_char(context, builder, '"');
    for (size_t index = 0; index < length && status == ATLAS_OK; ++index) {
        unsigned char value = (unsigned char)text[index];
        if (value == '"' || value == '\\') {
            status = atlas_text_builder_append_char(context, builder, '\\');
            if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, (char)value);
        } else if (value == '\n' || value == '\r') {
            status = atlas_text_builder_append(context, builder, "\\n");
        } else if (value < 0x20) {
            status = atlas_text_builder_append_char(context, builder, ' ');
        } else {
            status = atlas_text_builder_append_char(context, builder, (char)value);
        }
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, '"');
    return status;
}

atlas_status atlas_document_to_dot(atlas_context *context,
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
    atlas_status status = atlas_text_builder_append(context, &builder, "digraph atlaspack {\n");
    if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "  graph [rankdir=LR];\n");
    if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "  node [shape=box];\n");
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context, &builder, "  n%u [label=", node->id);
        if (status == ATLAS_OK) status = atlas_dot_string(context, &builder, name->bytes, name->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "];\n");
    }
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "  n%u -> n%u [label=",
                                           edge->source_id,
                                           edge->target_id);
        if (status == ATLAS_OK) status = atlas_dot_string(context, &builder, label->bytes, label->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "];\n");
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "}\n");
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
