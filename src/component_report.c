#include "internal.h"

static atlas_status component_report_node(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const atlas_document *document,
                                          size_t node_index) {
    const atlas_node *node = &document->nodes[node_index];
    const atlas_string *name = &document->strings[node->name_index];
    atlas_status status = atlas_text_builder_printf(context,
                                                    builder,
                                                    "%u:",
                                                    node->id);
    if (status != ATLAS_OK) {
        return status;
    }
    return atlas_text_builder_append_n(context,
                                       builder,
                                       name->bytes,
                                       name->length);
}

static atlas_status component_report_weak(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const atlas_document *document) {
    atlas_components *components = NULL;
    atlas_status status = atlas_components_build(context,
                                                 document,
                                                 &components);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "weak_component_count=%zu\n",
                                       atlas_components_count(components));
    for (size_t component_index = 0;
         component_index < atlas_components_count(components) && status == ATLAS_OK;
         ++component_index) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "weak[%zu] size=%zu nodes=",
                                           component_index,
                                           atlas_components_size(components,
                                                                 component_index));
        size_t ordinal = 0;
        for (size_t node_index = 0;
             node_index < document->node_count && status == ATLAS_OK;
             ++node_index) {
            size_t assigned = ATLAS_NO_INDEX;
            status = atlas_components_node_component(components,
                                                      document->nodes[node_index].id,
                                                      &assigned);
            if (status != ATLAS_OK) {
                break;
            }
            if (assigned != component_index) {
                continue;
            }
            if (ordinal != 0) {
                status = atlas_text_builder_append_char(context, builder, ',');
            }
            if (status == ATLAS_OK) {
                status = component_report_node(context,
                                               builder,
                                               document,
                                               node_index);
            }
            ++ordinal;
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '\n');
        }
    }
    atlas_components_destroy(components);
    return status;
}

static int component_is_cyclic(const atlas_document *document,
                               const atlas_strong_components *components,
                               size_t component_index) {
    if (atlas_strong_components_size(components, component_index) > 1) {
        return 1;
    }
    for (size_t node_index = 0; node_index < document->node_count; ++node_index) {
        size_t assigned = ATLAS_NO_INDEX;
        if (atlas_strong_components_node_component(components,
                                                   document->nodes[node_index].id,
                                                   &assigned) != ATLAS_OK ||
            assigned != component_index) {
            continue;
        }
        const atlas_node *node = &document->nodes[node_index];
        for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
            size_t edge_index = document->edge_order[node->outgoing_start + ordinal];
            const atlas_edge *edge = &document->edges[edge_index];
            if (edge->target_index == node_index) {
                return 1;
            }
        }
    }
    return 0;
}

static atlas_status component_report_strong(atlas_context *context,
                                            atlas_text_builder *builder,
                                            const atlas_document *document) {
    atlas_strong_components *components = NULL;
    atlas_status status = atlas_strong_components_build(context,
                                                        document,
                                                        &components);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "strong_component_count=%zu\ncyclic_component_count=%zu\n",
                                       atlas_strong_components_count(components),
                                       atlas_strong_components_cyclic_count(components));
    for (size_t component_index = 0;
         component_index < atlas_strong_components_count(components) && status == ATLAS_OK;
         ++component_index) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "strong[%zu] size=%zu cyclic=%s nodes=",
                                           component_index,
                                           atlas_strong_components_size(components,
                                                                        component_index),
                                           component_is_cyclic(document,
                                                               components,
                                                               component_index)
                                               ? "true"
                                               : "false");
        size_t ordinal = 0;
        for (size_t node_index = 0;
             node_index < document->node_count && status == ATLAS_OK;
             ++node_index) {
            size_t assigned = ATLAS_NO_INDEX;
            status = atlas_strong_components_node_component(components,
                                                             document->nodes[node_index].id,
                                                             &assigned);
            if (status != ATLAS_OK) {
                break;
            }
            if (assigned != component_index) {
                continue;
            }
            if (ordinal != 0) {
                status = atlas_text_builder_append_char(context, builder, ',');
            }
            if (status == ATLAS_OK) {
                status = component_report_node(context,
                                               builder,
                                               document,
                                               node_index);
            }
            ++ordinal;
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '\n');
        }
    }
    atlas_strong_components_destroy(components);
    return status;
}

static atlas_status component_report_cross_edges(atlas_context *context,
                                                 atlas_text_builder *builder,
                                                 const atlas_document *document) {
    atlas_components *components = NULL;
    atlas_status status = atlas_components_build(context,
                                                 document,
                                                 &components);
    if (status != ATLAS_OK) {
        return status;
    }
    size_t cross_edges = 0;
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        size_t source_component = ATLAS_NO_INDEX;
        size_t target_component = ATLAS_NO_INDEX;
        status = atlas_components_node_component(components,
                                                  edge->source_id,
                                                  &source_component);
        if (status != ATLAS_OK) {
            break;
        }
        status = atlas_components_node_component(components,
                                                  edge->target_id,
                                                  &target_component);
        if (status != ATLAS_OK) {
            break;
        }
        if (source_component != target_component) {
            ++cross_edges;
        }
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "cross_weak_component_edges=%zu\n",
                                           cross_edges);
    }
    atlas_components_destroy(components);
    return status;
}

atlas_status atlas_document_component_report(atlas_context *context,
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
    atlas_status status = atlas_text_builder_append(context,
                                                    &builder,
                                                    "ATPK COMPONENT REPORT\n");
    if (status == ATLAS_OK) {
        status = component_report_weak(context,
                                       &builder,
                                       document);
    }
    if (status == ATLAS_OK) {
        status = component_report_strong(context,
                                         &builder,
                                         document);
    }
    if (status == ATLAS_OK) {
        status = component_report_cross_edges(context,
                                              &builder,
                                              document);
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
