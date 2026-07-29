#include "internal.h"

static atlas_status atlas_xml_escape(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const char *text,
                                     size_t length) {
    for (size_t index = 0; index < length; ++index) {
        const char *replacement = NULL;
        switch (text[index]) {
            case '&': replacement = "&amp;"; break;
            case '<': replacement = "&lt;"; break;
            case '>': replacement = "&gt;"; break;
            case '"': replacement = "&quot;"; break;
            case '\'': replacement = "&apos;"; break;
            default: break;
        }
        atlas_status status;
        if (replacement != NULL) {
            status = atlas_text_builder_append(context, builder, replacement);
        } else if ((unsigned char)text[index] < 0x20 &&
                   text[index] != '\t' && text[index] != '\n' && text[index] != '\r') {
            status = atlas_text_builder_append_char(context, builder, ' ');
        } else {
            status = atlas_text_builder_append_char(context, builder, text[index]);
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_document_to_graphml(atlas_context *context,
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
    atlas_status status = atlas_text_builder_append(
        context,
        &builder,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\">\n"
        "  <key id=\"name\" for=\"node\" attr.name=\"name\" attr.type=\"string\"/>\n"
        "  <key id=\"label\" for=\"edge\" attr.name=\"label\" attr.type=\"string\"/>\n"
        "  <graph id=\"atlaspack\" edgedefault=\"directed\">\n");
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "    <node id=\"n%u\"><data key=\"name\">",
                                           node->id);
        if (status == ATLAS_OK) status = atlas_xml_escape(context, &builder, name->bytes, name->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "</data></node>\n");
    }
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "    <edge id=\"e%zu\" source=\"n%u\" target=\"n%u\"><data key=\"label\">",
                                           index,
                                           edge->source_id,
                                           edge->target_id);
        if (status == ATLAS_OK) status = atlas_xml_escape(context, &builder, label->bytes, label->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "</data></edge>\n");
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context,
                                           &builder,
                                           "  </graph>\n</graphml>\n");
    }
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
