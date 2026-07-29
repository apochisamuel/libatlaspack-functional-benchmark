#include "internal.h"

static atlas_status reachability_header(atlas_context *context,
                                        atlas_text_builder *builder,
                                        const atlas_document *document) {
    atlas_status status = atlas_text_builder_append(context, builder, "source");
    for (size_t index = 0;
         index < document->node_count && status == ATLAS_OK;
         ++index) {
        const atlas_node *node = &document->nodes[index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_append_char(context, builder, ',');
        if (status == ATLAS_OK) {
            status = atlas_text_builder_csv_field(context,
                                                  builder,
                                                  name->bytes,
                                                  name->length);
        }
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    return status;
}

static atlas_status reachability_row(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const atlas_document *document,
                                     size_t source_index) {
    const atlas_node *source = &document->nodes[source_index];
    const atlas_string *source_name = &document->strings[source->name_index];
    atlas_status status = atlas_text_builder_csv_field(context,
                                                       builder,
                                                       source_name->bytes,
                                                       source_name->length);
    for (size_t target_index = 0;
         target_index < document->node_count && status == ATLAS_OK;
         ++target_index) {
        int reachable = 0;
        status = atlas_document_reachable(document,
                                          source->id,
                                          document->nodes[target_index].id,
                                          &reachable);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_printf(context,
                                               builder,
                                               ",%d",
                                               reachable ? 1 : 0);
        }
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    return status;
}

atlas_status atlas_document_reachability_csv(atlas_context *context,
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
    atlas_status status = reachability_header(context,
                                              &builder,
                                              document);
    for (size_t source_index = 0;
         source_index < document->node_count && status == ATLAS_OK;
         ++source_index) {
        status = reachability_row(context,
                                  &builder,
                                  document,
                                  source_index);
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
