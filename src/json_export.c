#include "internal.h"

static atlas_status atlas_json_indent(atlas_context *context,
                                      atlas_text_builder *builder,
                                      int pretty,
                                      unsigned depth) {
    if (!pretty) {
        return ATLAS_OK;
    }
    atlas_status status = atlas_text_builder_append_char(context, builder, '\n');
    for (unsigned index = 0; index < depth && status == ATLAS_OK; ++index) {
        status = atlas_text_builder_append(context, builder, "  ");
    }
    return status;
}

static atlas_status atlas_json_key(atlas_context *context,
                                   atlas_text_builder *builder,
                                   const char *key,
                                   int pretty) {
    atlas_status status = atlas_text_builder_json_string(context,
                                                         builder,
                                                         key,
                                                         strlen(key));
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context, builder, pretty ? ": " : ":");
    }
    return status;
}

static atlas_status atlas_json_metadata(atlas_context *context,
                                        atlas_text_builder *builder,
                                        const atlas_document *document,
                                        int pretty,
                                        unsigned depth) {
    atlas_status status = atlas_text_builder_append_char(context, builder, '[');
    for (size_t index = 0; index < document->metadata_count && status == ATLAS_OK; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        if (index != 0) {
            status = atlas_text_builder_append_char(context, builder, ',');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_indent(context, builder, pretty, depth + 1);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '{');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_key(context, builder, "key", pretty);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    key->bytes,
                                                    key->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, ',');
        }
        if (status == ATLAS_OK && pretty) {
            status = atlas_text_builder_append_char(context, builder, ' ');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_key(context, builder, "value", pretty);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    value->bytes,
                                                    value->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '}');
        }
    }
    if (status == ATLAS_OK && document->metadata_count != 0) {
        status = atlas_json_indent(context, builder, pretty, depth);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, ']');
    }
    return status;
}

static atlas_status atlas_json_attributes(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const atlas_document *document,
                                          const atlas_node *node,
                                          int pretty,
                                          unsigned depth) {
    atlas_status status = atlas_text_builder_append_char(context, builder, '[');
    for (size_t ordinal = 0; ordinal < node->attribute_count && status == ATLAS_OK; ++ordinal) {
        const atlas_attribute *attribute =
            &document->attributes[node->first_attribute + ordinal];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        if (ordinal != 0) {
            status = atlas_text_builder_append_char(context, builder, ',');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_indent(context, builder, pretty, depth + 1);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '{');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_key(context, builder, "key", pretty);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    key->bytes,
                                                    key->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, ',');
        }
        if (status == ATLAS_OK && pretty) {
            status = atlas_text_builder_append_char(context, builder, ' ');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_key(context, builder, "value", pretty);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    value->bytes,
                                                    value->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '}');
        }
    }
    if (status == ATLAS_OK && node->attribute_count != 0) {
        status = atlas_json_indent(context, builder, pretty, depth);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, ']');
    }
    return status;
}

static atlas_status atlas_json_nodes(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const atlas_document *document,
                                     int pretty,
                                     unsigned depth) {
    atlas_status status = atlas_text_builder_append_char(context, builder, '[');
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        if (index != 0) {
            status = atlas_text_builder_append_char(context, builder, ',');
        }
        if (status == ATLAS_OK) {
            status = atlas_json_indent(context, builder, pretty, depth + 1);
        }
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, '{');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "id", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, builder, "%u", node->id);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK && pretty) status = atlas_text_builder_append_char(context, builder, ' ');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "name", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_json_string(context, builder, name->bytes, name->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK && pretty) status = atlas_text_builder_append_char(context, builder, ' ');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "inDegree", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, builder, "%zu", node->incoming_count);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK && pretty) status = atlas_text_builder_append_char(context, builder, ' ');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "outDegree", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, builder, "%zu", node->outgoing_count);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK && pretty) status = atlas_text_builder_append_char(context, builder, ' ');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "attributes", pretty);
        if (status == ATLAS_OK) status = atlas_json_attributes(context, builder, document, node, pretty, depth + 1);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, '}');
    }
    if (status == ATLAS_OK && document->node_count != 0) {
        status = atlas_json_indent(context, builder, pretty, depth);
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ']');
    return status;
}

static atlas_status atlas_json_edges(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const atlas_document *document,
                                     int pretty,
                                     unsigned depth) {
    atlas_status status = atlas_text_builder_append_char(context, builder, '[');
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        if (index != 0) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK) status = atlas_json_indent(context, builder, pretty, depth + 1);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, '{');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "source", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, builder, "%u", edge->source_id);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK && pretty) status = atlas_text_builder_append_char(context, builder, ' ');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "target", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_printf(context, builder, "%u", edge->target_id);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK && pretty) status = atlas_text_builder_append_char(context, builder, ' ');
        if (status == ATLAS_OK) status = atlas_json_key(context, builder, "label", pretty);
        if (status == ATLAS_OK) status = atlas_text_builder_json_string(context, builder, label->bytes, label->length);
        if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, '}');
    }
    if (status == ATLAS_OK && document->edge_count != 0) {
        status = atlas_json_indent(context, builder, pretty, depth);
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, ']');
    return status;
}

atlas_status atlas_document_to_json(atlas_context *context,
                                    const atlas_document *document,
                                    int pretty,
                                    char **out_text,
                                    size_t *out_length) {
    if (context == NULL || document == NULL || out_text == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_text = NULL;
    *out_length = 0;
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    atlas_status status = atlas_text_builder_append_char(context, &builder, '{');
    if (status == ATLAS_OK) status = atlas_json_indent(context, &builder, pretty, 1);
    if (status == ATLAS_OK) status = atlas_json_key(context, &builder, "version", pretty);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "%u", document->header.version);
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, &builder, ',');
    if (status == ATLAS_OK) status = atlas_json_indent(context, &builder, pretty, 1);
    if (status == ATLAS_OK) status = atlas_json_key(context, &builder, "metadata", pretty);
    if (status == ATLAS_OK) status = atlas_json_metadata(context, &builder, document, pretty, 1);
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, &builder, ',');
    if (status == ATLAS_OK) status = atlas_json_indent(context, &builder, pretty, 1);
    if (status == ATLAS_OK) status = atlas_json_key(context, &builder, "nodes", pretty);
    if (status == ATLAS_OK) status = atlas_json_nodes(context, &builder, document, pretty, 1);
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, &builder, ',');
    if (status == ATLAS_OK) status = atlas_json_indent(context, &builder, pretty, 1);
    if (status == ATLAS_OK) status = atlas_json_key(context, &builder, "edges", pretty);
    if (status == ATLAS_OK) status = atlas_json_edges(context, &builder, document, pretty, 1);
    if (status == ATLAS_OK) status = atlas_json_indent(context, &builder, pretty, 0);
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, &builder, '}');
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
