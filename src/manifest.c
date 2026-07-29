#include "internal.h"

atlas_status atlas_document_manifest(atlas_context *context,
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
    if (status != ATLAS_OK) return status;
    atlas_components *components = NULL;
    status = atlas_components_build(context, document, &components);
    if (status != ATLAS_OK) return status;
    size_t longest_path = 0;
    atlas_status path_status = atlas_document_longest_path_length(context,
                                                                  document,
                                                                  &longest_path);
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    status = atlas_text_builder_append(context, &builder, "ATPK MANIFEST\n");
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "format_version=%u\n", document->header.version);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "flags=%u\n", document->header.flags);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "normalized=%s\n", document->normalized ? "true" : "false");
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "nodes=%zu\n", statistics.node_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "edges=%zu\n", statistics.edge_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "attributes=%zu\n", statistics.attribute_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "metadata=%zu\n", statistics.metadata_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "components=%zu\n", atlas_components_count(components));
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "sources=%zu\n", statistics.source_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "sinks=%zu\n", statistics.sink_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "isolated=%zu\n", statistics.isolated_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "self_loops=%zu\n", statistics.self_loop_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "duplicate_edges=%zu\n", statistics.duplicate_edge_count);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "max_out_degree=%zu\n", statistics.maximum_out_degree);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "max_in_degree=%zu\n", statistics.maximum_in_degree);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "average_out_degree=%.6f\n", statistics.average_out_degree);
    if (status == ATLAS_OK) status = atlas_text_builder_printf(context, &builder, "density=%.6f\n", statistics.density);
    if (status == ATLAS_OK) {
        if (path_status == ATLAS_OK) {
            status = atlas_text_builder_printf(context, &builder, "longest_path=%zu\n", longest_path);
        } else if (path_status == ATLAS_ERR_CYCLE) {
            status = atlas_text_builder_append(context, &builder, "longest_path=cyclic\n");
        } else {
            status = path_status;
        }
    }
    if (status == ATLAS_OK) status = atlas_text_builder_append(context, &builder, "metadata_entries:\n");
    for (size_t index = 0; index < document->metadata_count && status == ATLAS_OK; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "  %s=%s\n",
                                           key->bytes,
                                           value->bytes);
    }
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) status = ATLAS_ERR_NOMEM;
    }
    atlas_text_builder_dispose(&builder);
    atlas_components_destroy(components);
    return status;
}
