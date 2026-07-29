#include "internal.h"

static atlas_status node_report_heading(atlas_context *context,
                                        atlas_text_builder *builder,
                                        const atlas_document *document,
                                        size_t node_index) {
    const atlas_node *node = &document->nodes[node_index];
    const atlas_string *name = &document->strings[node->name_index];
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "ATPK NODE REPORT\n");
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "id=%u\nname=",
                                       node->id);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_append_n(context,
                                         builder,
                                         name->bytes,
                                         name->length);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_printf(context,
                                     builder,
                                     "\nout_degree=%zu\nin_degree=%zu\nattributes=%u\n",
                                     node->outgoing_count,
                                     node->incoming_count,
                                     node->attribute_count);
}

static atlas_status node_report_attributes(atlas_context *context,
                                           atlas_text_builder *builder,
                                           const atlas_document *document,
                                           const atlas_node *node) {
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "attribute_values:\n");
    for (size_t ordinal = 0;
         ordinal < node->attribute_count && status == ATLAS_OK;
         ++ordinal) {
        size_t attribute_index = node->first_attribute + ordinal;
        const atlas_attribute *attribute = &document->attributes[attribute_index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        status = atlas_text_builder_append(context, builder, "  - key=");
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    key->bytes,
                                                    key->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append(context, builder, " value=");
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    value->bytes,
                                                    value->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '\n');
        }
    }
    return status;
}

static atlas_status node_report_edge(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const atlas_document *document,
                                     size_t edge_index,
                                     int outgoing) {
    const atlas_edge *edge = &document->edges[edge_index];
    size_t peer_index = outgoing ? edge->target_index : edge->source_index;
    const atlas_node *peer = &document->nodes[peer_index];
    const atlas_string *peer_name = &document->strings[peer->name_index];
    const atlas_string *label = &document->strings[edge->label_index];
    atlas_status status = atlas_text_builder_printf(context,
                                                    builder,
                                                    "  - edge=%zu peer_id=%u peer_name=",
                                                    edge_index,
                                                    peer->id);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_json_string(context,
                                            builder,
                                            peer_name->bytes,
                                            peer_name->length);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_append(context, builder, " label=");
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_json_string(context,
                                            builder,
                                            label->bytes,
                                            label->length);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_append_char(context, builder, '\n');
}

static atlas_status node_report_outgoing(atlas_context *context,
                                         atlas_text_builder *builder,
                                         const atlas_document *document,
                                         const atlas_node *node) {
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "outgoing_edges:\n");
    for (size_t ordinal = 0;
         ordinal < node->outgoing_count && status == ATLAS_OK;
         ++ordinal) {
        size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
        status = node_report_edge(context,
                                  builder,
                                  document,
                                  edge_index,
                                  1);
    }
    return status;
}

static atlas_status node_report_incoming(atlas_context *context,
                                         atlas_text_builder *builder,
                                         const atlas_document *document,
                                         const atlas_node *node) {
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "incoming_edges:\n");
    for (size_t ordinal = 0;
         ordinal < node->incoming_count && status == ATLAS_OK;
         ++ordinal) {
        size_t edge_index = document->incoming_order[node->incoming_start + ordinal];
        status = node_report_edge(context,
                                  builder,
                                  document,
                                  edge_index,
                                  0);
    }
    return status;
}

static atlas_status node_report_reachability(atlas_context *context,
                                             atlas_text_builder *builder,
                                             const atlas_document *document,
                                             uint32_t node_id) {
    size_t descendants = 0;
    size_t ancestors = 0;
    atlas_status status = atlas_document_descendant_count(document,
                                                          node_id,
                                                          &descendants);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_document_ancestor_count(document,
                                           node_id,
                                           &ancestors);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_printf(context,
                                     builder,
                                     "descendants=%zu\nancestors=%zu\n",
                                     descendants,
                                     ancestors);
}

atlas_status atlas_document_node_report(atlas_context *context,
                                        const atlas_document *document,
                                        uint32_t node_id,
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
    int found = 0;
    size_t node_index = atlas_id_lookup(document, node_id, &found);
    if (!found) {
        atlas_set_error(context, "node %u was not found", node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    const atlas_node *node = &document->nodes[node_index];
    atlas_status status = node_report_heading(context,
                                              &builder,
                                              document,
                                              node_index);
    if (status == ATLAS_OK) {
        status = node_report_reachability(context,
                                          &builder,
                                          document,
                                          node_id);
    }
    if (status == ATLAS_OK) {
        status = node_report_attributes(context,
                                        &builder,
                                        document,
                                        node);
    }
    if (status == ATLAS_OK) {
        status = node_report_outgoing(context,
                                      &builder,
                                      document,
                                      node);
    }
    if (status == ATLAS_OK) {
        status = node_report_incoming(context,
                                      &builder,
                                      document,
                                      node);
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
