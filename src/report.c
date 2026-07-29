#include "internal.h"

atlas_status atlas_document_report(atlas_context *context,
                                   const atlas_document *document,
                                   char **out_text,
                                   size_t *out_length) {
    if (context == NULL || document == NULL || out_text == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_statistics statistics;
    atlas_status status = atlas_document_statistics(document, &statistics);
    if (status != ATLAS_OK) return status;
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    status = atlas_text_builder_append_char(context, &builder, '{');
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "\"nodes\":%zu", statistics.node_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"edges\":%zu", statistics.edge_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"attributes\":%zu", statistics.attribute_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"metadata\":%zu", statistics.metadata_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"sources\":%zu", statistics.source_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"sinks\":%zu", statistics.sink_count);
    if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, ",\"items\":[");
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        if (index != 0) status = atlas_text_builder_append_char(context, &builder, ',');
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "{\"id\":");
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "%u", node->id);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, ",\"name\":");
        if (status == ATLAS_OK) status = atlas_text_builder_json_string(context, &builder, name->bytes, name->length);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"out\":%zu", node->outgoing_count);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"in\":%zu", node->incoming_count);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, ",\"attrs\":%zu}", node->attribute_count);
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "]}");
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
