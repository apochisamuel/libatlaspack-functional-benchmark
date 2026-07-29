#include "internal.h"

static atlas_status atlas_csv_append_row(atlas_context *context,
                                         atlas_text_builder *builder,
                                         const char *kind,
                                         const char *id,
                                         const char *name,
                                         const char *source,
                                         const char *target,
                                         const char *key,
                                         const char *value) {
    const char *fields[7] = {kind, id, name, source, target, key, value};
    atlas_status status = ATLAS_OK;
    for (size_t index = 0; index < 7 && status == ATLAS_OK; ++index) {
        if (index != 0) status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK) {
            const char *field = fields[index] == NULL ? "" : fields[index];
            status = atlas_text_builder_csv_field(context,
                                                  builder,
                                                  field,
                                                  strlen(field));
        }
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append_char(context, builder, '\n');
    return status;
}

atlas_status atlas_document_to_csv(atlas_context *context,
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
    atlas_status status = atlas_text_builder_append(context,
                                                    &builder,
                                                    "kind,id,name,source,target,key,value\n");
    char id_buffer[32];
    char source_buffer[32];
    char target_buffer[32];
    for (size_t index = 0; index < document->node_count && status == ATLAS_OK; ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        snprintf(id_buffer, sizeof(id_buffer), "%u", node->id);
        status = atlas_csv_append_row(context,
                                      &builder,
                                      "node",
                                      id_buffer,
                                      name->bytes,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
    }
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_edge *edge = &document->edges[index];
        const atlas_string *label = &document->strings[edge->label_index];
        snprintf(source_buffer, sizeof(source_buffer), "%u", edge->source_id);
        snprintf(target_buffer, sizeof(target_buffer), "%u", edge->target_id);
        status = atlas_csv_append_row(context,
                                      &builder,
                                      "edge",
                                      NULL,
                                      label->bytes,
                                      source_buffer,
                                      target_buffer,
                                      NULL,
                                      NULL);
    }
    for (size_t index = 0; index < document->attribute_count && status == ATLAS_OK; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        const atlas_string *key = &document->strings[attribute->key_index];
        const atlas_string *value = &document->strings[attribute->value_index];
        snprintf(id_buffer, sizeof(id_buffer), "%u", attribute->node_id);
        status = atlas_csv_append_row(context,
                                      &builder,
                                      "attribute",
                                      id_buffer,
                                      NULL,
                                      NULL,
                                      NULL,
                                      key->bytes,
                                      value->bytes);
    }
    for (size_t index = 0; index < document->metadata_count && status == ATLAS_OK; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        status = atlas_csv_append_row(context,
                                      &builder,
                                      "metadata",
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      key->bytes,
                                      value->bytes);
    }
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
