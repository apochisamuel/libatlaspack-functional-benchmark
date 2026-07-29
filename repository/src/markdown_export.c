#include "internal.h"

static atlas_status atlas_markdown_escape(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const char *text,
                                          size_t length) {
    for (size_t index = 0; index < length; ++index) {
        char value = text[index];
        atlas_status status = ATLAS_OK;
        if (value == '|' || value == '\\' || value == '`') {
            status = atlas_text_builder_append_char(context, builder, '\\');
            if (status == ATLAS_OK) {
                status = atlas_text_builder_append_char(context, builder, value);
            }
        } else if (value == '\r' || value == '\n') {
            status = atlas_text_builder_append_char(context, builder, ' ');
        } else {
            status = atlas_text_builder_append_char(context, builder, value);
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_document_to_markdown(atlas_context *context,
                                        const atlas_document *document,
                                        char **out_text,
                                        size_t *out_length) {
    if (context == NULL || document == NULL || out_text == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_text = NULL;
    *out_length = 0;
    atlas_statistics statistics;
    atlas_status status = atlas_document_statistics(document, &statistics);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    status = atlas_text_builder_append(context, &builder, "# ATPK Document\n\n");
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context, &builder, "## Summary\n\n");
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "- Format version: %u\n"
                                           "- Nodes: %zu\n"
                                           "- Edges: %zu\n"
                                           "- Attributes: %zu\n"
                                           "- Metadata entries: %zu\n"
                                           "- Sources: %zu\n"
                                           "- Sinks: %zu\n"
                                           "- Isolated nodes: %zu\n\n",
                                           document->header.version,
                                           statistics.node_count,
                                           statistics.edge_count,
                                           statistics.attribute_count,
                                           statistics.metadata_count,
                                           statistics.source_count,
                                           statistics.sink_count,
                                           statistics.isolated_count);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context,
                                           &builder,
                                           "## Metadata\n\n| Key | Value |\n|---|---|\n");
    }
    for (size_t index = 0; index < document->metadata_count && status == ATLAS_OK; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        status = atlas_text_builder_append(context, &builder, "| ");
        if (status == ATLAS_OK) status = atlas_markdown_escape(context, &builder, key->bytes, key->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, " | ");
        if (status == ATLAS_OK) status = atlas_markdown_escape(context, &builder, value->bytes, value->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, " |\n");
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context,
                                           &builder,
                                           "\n## Nodes\n\n| ID | Name | In | Out | Attributes |\n|---:|---|---:|---:|---:|\n");
    }
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context, &builder, "| %u | ", node->id);
        if (status == ATLAS_OK) status = atlas_markdown_escape(context, &builder, name->bytes, name->length);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_printf(context,
                                               &builder,
                                               " | %zu | %zu | %zu |\n",
                                               node->incoming_count,
                                               node->outgoing_count,
                                               node->attribute_count);
        }
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context,
                                           &builder,
                                           "\n## Edges\n\n| Source | Target | Label |\n|---:|---:|---|\n");
    }
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "| %u | %u | ",
                                           edge->source_id,
                                           edge->target_id);
        if (status == ATLAS_OK) status = atlas_markdown_escape(context, &builder, label->bytes, label->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, " |\n");
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context,
                                           &builder,
                                           "\n## Attributes\n\n| Node | Key | Value |\n|---:|---|---|\n");
    }
    for (size_t index = 0; index < document->attribute_count && status == ATLAS_OK; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        status = atlas_text_builder_printf(context, &builder, "| %u | ", attribute->node_id);
        if (status == ATLAS_OK) status = atlas_markdown_escape(context, &builder, key->bytes, key->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, " | ");
        if (status == ATLAS_OK) status = atlas_markdown_escape(context, &builder, value->bytes, value->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, " |\n");
    }
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
