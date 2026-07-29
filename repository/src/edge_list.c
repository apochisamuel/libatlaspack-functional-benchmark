#include "internal.h"

static atlas_status edge_list_write_header(atlas_context *context,
                                           atlas_text_builder *builder,
                                           const atlas_document *document) {
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "ATPK EDGE LIST\n");
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "nodes=%zu edges=%zu\n",
                                       document->node_count,
                                       document->edge_count);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_append(context,
                                     builder,
                                     "index\tsource_id\tsource_name\ttarget_id\ttarget_name\tlabel\n");
}

static atlas_status edge_list_write_field(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const atlas_string *string) {
    for (size_t index = 0; index < string->length; ++index) {
        unsigned char value = (unsigned char)string->bytes[index];
        atlas_status status;
        switch (value) {
            case '\\':
                status = atlas_text_builder_append(context, builder, "\\\\");
                break;
            case '\t':
                status = atlas_text_builder_append(context, builder, "\\t");
                break;
            case '\n':
                status = atlas_text_builder_append(context, builder, "\\n");
                break;
            case '\r':
                status = atlas_text_builder_append(context, builder, "\\r");
                break;
            default:
                if (value < 0x20u) {
                    status = atlas_text_builder_printf(context,
                                                       builder,
                                                       "\\x%02x",
                                                       (unsigned)value);
                } else {
                    status = atlas_text_builder_append_char(context,
                                                            builder,
                                                            (char)value);
                }
                break;
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return ATLAS_OK;
}

static atlas_status edge_list_write_edge(atlas_context *context,
                                         atlas_text_builder *builder,
                                         const atlas_document *document,
                                         size_t index) {
    const atlas_edge *edge = &document->edges[index];
    const atlas_node *source = &document->nodes[edge->source_index];
    const atlas_node *target = &document->nodes[edge->target_index];
    const atlas_string *source_name = &document->strings[source->name_index];
    const atlas_string *target_name = &document->strings[target->name_index];
    const atlas_string *label = &document->strings[edge->label_index];
    atlas_status status = atlas_text_builder_printf(context,
                                                    builder,
                                                    "%zu\t%u\t",
                                                    index,
                                                    edge->source_id);
    if (status != ATLAS_OK) {
        return status;
    }
    status = edge_list_write_field(context, builder, source_name);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "\t%u\t",
                                       edge->target_id);
    if (status != ATLAS_OK) {
        return status;
    }
    status = edge_list_write_field(context, builder, target_name);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_append_char(context, builder, '\t');
    if (status != ATLAS_OK) {
        return status;
    }
    status = edge_list_write_field(context, builder, label);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_append_char(context, builder, '\n');
}

atlas_status atlas_document_edge_list(atlas_context *context,
                                      const atlas_document *document,
                                      char **out_text,
                                      size_t *out_length) {
    if (context == NULL ||
        document == NULL ||
        out_text == NULL ||
        out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_text = NULL;
    *out_length = 0;
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    atlas_status status = edge_list_write_header(context,
                                                 &builder,
                                                 document);
    for (size_t index = 0;
         index < document->edge_count && status == ATLAS_OK;
         ++index) {
        status = edge_list_write_edge(context,
                                      &builder,
                                      document,
                                      index);
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
