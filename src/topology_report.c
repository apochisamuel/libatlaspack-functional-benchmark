#include "internal.h"

static atlas_status topology_summary(atlas_context *context,
                                     atlas_text_builder *builder,
                                     const atlas_document *document) {
    atlas_statistics statistics;
    atlas_status status = atlas_document_statistics(document, &statistics);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_append(context,
                                       builder,
                                       "ATPK TOPOLOGY REPORT\n");
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_printf(context,
                                     builder,
                                     "nodes=%zu\nedges=%zu\nsources=%zu\nsinks=%zu\nisolated=%zu\nself_loops=%zu\nduplicate_edges=%zu\nmax_out_degree=%zu\nmax_in_degree=%zu\naverage_out_degree=%.6f\ndensity=%.6f\n",
                                     statistics.node_count,
                                     statistics.edge_count,
                                     statistics.source_count,
                                     statistics.sink_count,
                                     statistics.isolated_count,
                                     statistics.self_loop_count,
                                     statistics.duplicate_edge_count,
                                     statistics.maximum_out_degree,
                                     statistics.maximum_in_degree,
                                     statistics.average_out_degree,
                                     statistics.density);
}

static atlas_status topology_components(atlas_context *context,
                                        atlas_text_builder *builder,
                                        const atlas_document *document) {
    atlas_components *weak = NULL;
    atlas_status status = atlas_components_build(context, document, &weak);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "weak_components=%zu\nweak_component_sizes:",
                                       atlas_components_count(weak));
    for (size_t index = 0;
         index < atlas_components_count(weak) && status == ATLAS_OK;
         ++index) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "%s%zu",
                                           index == 0 ? " " : ",",
                                           atlas_components_size(weak, index));
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    atlas_components_destroy(weak);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_strong_components *strong = NULL;
    status = atlas_strong_components_build(context, document, &strong);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "strong_components=%zu\ncyclic_components=%zu\nstrong_component_sizes:",
                                       atlas_strong_components_count(strong),
                                       atlas_strong_components_cyclic_count(strong));
    for (size_t index = 0;
         index < atlas_strong_components_count(strong) && status == ATLAS_OK;
         ++index) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "%s%zu",
                                           index == 0 ? " " : ",",
                                           atlas_strong_components_size(strong, index));
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    atlas_strong_components_destroy(strong);
    return status;
}

static atlas_status topology_layers(atlas_context *context,
                                    atlas_text_builder *builder,
                                    const atlas_document *document) {
    atlas_layers *layers = NULL;
    atlas_status status = atlas_layers_build(context, document, &layers);
    if (status == ATLAS_ERR_CYCLE) {
        return atlas_text_builder_append(context,
                                         builder,
                                         "acyclic=false\nlayers=unavailable\nlongest_path=unavailable\n");
    }
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_append(context, builder, "acyclic=true\n");
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "layers=%zu\nlayer_sizes:",
                                           atlas_layers_count(layers));
    }
    for (size_t index = 0;
         index < atlas_layers_count(layers) && status == ATLAS_OK;
         ++index) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "%s%zu",
                                           index == 0 ? " " : ",",
                                           atlas_layers_size(layers, index));
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    atlas_layers_destroy(layers);
    if (status != ATLAS_OK) {
        return status;
    }
    size_t longest = 0;
    status = atlas_document_longest_path_length(context, document, &longest);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_printf(context,
                                     builder,
                                     "longest_path=%zu\n",
                                     longest);
}

static atlas_status topology_histogram(atlas_context *context,
                                       atlas_text_builder *builder,
                                       const char *heading,
                                       atlas_degree_bucket *buckets,
                                       size_t count) {
    atlas_status status = atlas_text_builder_printf(context,
                                                    builder,
                                                    "%s:",
                                                    heading);
    for (size_t index = 0; index < count && status == ATLAS_OK; ++index) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "%s%zu=%zu",
                                           index == 0 ? " " : ",",
                                           buckets[index].degree,
                                           buckets[index].node_count);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    return status;
}

static atlas_status topology_histograms(atlas_context *context,
                                        atlas_text_builder *builder,
                                        const atlas_document *document) {
    atlas_degree_bucket *outgoing = NULL;
    size_t outgoing_count = 0;
    atlas_status status = atlas_document_out_degree_histogram(context,
                                                              document,
                                                              &outgoing,
                                                              &outgoing_count);
    if (status != ATLAS_OK) {
        return status;
    }
    status = topology_histogram(context,
                                builder,
                                "out_degree_histogram",
                                outgoing,
                                outgoing_count);
    atlas_free(outgoing);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_degree_bucket *incoming = NULL;
    size_t incoming_count = 0;
    status = atlas_document_in_degree_histogram(context,
                                                document,
                                                &incoming,
                                                &incoming_count);
    if (status != ATLAS_OK) {
        return status;
    }
    status = topology_histogram(context,
                                builder,
                                "in_degree_histogram",
                                incoming,
                                incoming_count);
    atlas_free(incoming);
    return status;
}

static atlas_status topology_sources_and_sinks(atlas_context *context,
                                                atlas_text_builder *builder,
                                                const atlas_document *document) {
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "source_nodes:");
    size_t source_ordinal = 0;
    for (size_t index = 0;
         index < document->node_count && status == ATLAS_OK;
         ++index) {
        const atlas_node *node = &document->nodes[index];
        if (node->incoming_count != 0) {
            continue;
        }
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "%s%u:",
                                           source_ordinal == 0 ? " " : ",",
                                           node->id);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_n(context,
                                                 builder,
                                                 name->bytes,
                                                 name->length);
        }
        ++source_ordinal;
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context, builder, "\nsink_nodes:");
    }
    size_t sink_ordinal = 0;
    for (size_t index = 0;
         index < document->node_count && status == ATLAS_OK;
         ++index) {
        const atlas_node *node = &document->nodes[index];
        if (node->outgoing_count != 0) {
            continue;
        }
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "%s%u:",
                                           sink_ordinal == 0 ? " " : ",",
                                           node->id);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_n(context,
                                                 builder,
                                                 name->bytes,
                                                 name->length);
        }
        ++sink_ordinal;
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    return status;
}

atlas_status atlas_document_topology_report(atlas_context *context,
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
    atlas_status status = topology_summary(context, &builder, document);
    if (status == ATLAS_OK) {
        status = topology_components(context, &builder, document);
    }
    if (status == ATLAS_OK) {
        status = topology_layers(context, &builder, document);
    }
    if (status == ATLAS_OK) {
        status = topology_histograms(context, &builder, document);
    }
    if (status == ATLAS_OK) {
        status = topology_sources_and_sinks(context, &builder, document);
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
