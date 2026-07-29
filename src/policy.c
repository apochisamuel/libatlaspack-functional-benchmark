#include "internal.h"

atlas_policy atlas_policy_default(void) {
    atlas_policy policy;
    policy.maximum_nodes = ATLAS_MAX_NODES;
    policy.maximum_edges = ATLAS_MAX_EDGES;
    policy.maximum_attributes = ATLAS_MAX_ATTRIBUTES;
    policy.maximum_metadata = ATLAS_MAX_METADATA;
    policy.maximum_out_degree = SIZE_MAX;
    policy.maximum_in_degree = SIZE_MAX;
    policy.allow_cycles = 1;
    policy.allow_self_loops = 1;
    policy.allow_duplicate_edges = 1;
    policy.require_node_attributes = 0;
    policy.require_metadata = 0;
    return policy;
}

static atlas_status atlas_policy_add_count_errors(const atlas_document *document,
                                                  const atlas_policy *policy,
                                                  atlas_diagnostics *diagnostics) {
    atlas_status status = ATLAS_OK;
    if (document->node_count > policy->maximum_nodes) {
        status = atlas_diagnostics_add(diagnostics,
                                       ATLAS_DIAGNOSTIC_ERROR,
                                       "node-limit",
                                       ATLAS_INVALID_NODE_ID,
                                       ATLAS_DIAGNOSTIC_EDGE_NONE,
                                       "document has %zu nodes; policy allows %zu",
                                       document->node_count,
                                       policy->maximum_nodes);
    }
    if (status == ATLAS_OK && document->edge_count > policy->maximum_edges) {
        status = atlas_diagnostics_add(diagnostics,
                                       ATLAS_DIAGNOSTIC_ERROR,
                                       "edge-limit",
                                       ATLAS_INVALID_NODE_ID,
                                       ATLAS_DIAGNOSTIC_EDGE_NONE,
                                       "document has %zu edges; policy allows %zu",
                                       document->edge_count,
                                       policy->maximum_edges);
    }
    if (status == ATLAS_OK && document->attribute_count > policy->maximum_attributes) {
        status = atlas_diagnostics_add(diagnostics,
                                       ATLAS_DIAGNOSTIC_ERROR,
                                       "attribute-limit",
                                       ATLAS_INVALID_NODE_ID,
                                       ATLAS_DIAGNOSTIC_EDGE_NONE,
                                       "document has %zu attributes; policy allows %zu",
                                       document->attribute_count,
                                       policy->maximum_attributes);
    }
    if (status == ATLAS_OK && document->metadata_count > policy->maximum_metadata) {
        status = atlas_diagnostics_add(diagnostics,
                                       ATLAS_DIAGNOSTIC_ERROR,
                                       "metadata-limit",
                                       ATLAS_INVALID_NODE_ID,
                                       ATLAS_DIAGNOSTIC_EDGE_NONE,
                                       "document has %zu metadata entries; policy allows %zu",
                                       document->metadata_count,
                                       policy->maximum_metadata);
    }
    return status;
}

static atlas_status atlas_policy_add_node_errors(const atlas_document *document,
                                                 const atlas_policy *policy,
                                                 atlas_diagnostics *diagnostics) {
    for (size_t node_index = 0; node_index < document->node_count; ++node_index) {
        const atlas_node *node = &document->nodes[node_index];
        atlas_status status = ATLAS_OK;
        if (node->outgoing_count > policy->maximum_out_degree) {
            status = atlas_diagnostics_add(diagnostics,
                                           ATLAS_DIAGNOSTIC_ERROR,
                                           "out-degree-limit",
                                           node->id,
                                           ATLAS_DIAGNOSTIC_EDGE_NONE,
                                           "node %u has out-degree %zu; policy allows %zu",
                                           node->id,
                                           node->outgoing_count,
                                           policy->maximum_out_degree);
        }
        if (status == ATLAS_OK && node->incoming_count > policy->maximum_in_degree) {
            status = atlas_diagnostics_add(diagnostics,
                                           ATLAS_DIAGNOSTIC_ERROR,
                                           "in-degree-limit",
                                           node->id,
                                           ATLAS_DIAGNOSTIC_EDGE_NONE,
                                           "node %u has in-degree %zu; policy allows %zu",
                                           node->id,
                                           node->incoming_count,
                                           policy->maximum_in_degree);
        }
        if (status == ATLAS_OK && policy->require_node_attributes &&
            node->attribute_count == 0) {
            status = atlas_diagnostics_add(diagnostics,
                                           ATLAS_DIAGNOSTIC_ERROR,
                                           "missing-node-attributes",
                                           node->id,
                                           ATLAS_DIAGNOSTIC_EDGE_NONE,
                                           "node %u has no attributes",
                                           node->id);
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_policy_add_edge_errors(const atlas_document *document,
                                                 const atlas_policy *policy,
                                                 atlas_diagnostics *diagnostics) {
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        if (!policy->allow_self_loops && edge->source_index == edge->target_index) {
            atlas_status status = atlas_diagnostics_add(
                diagnostics,
                ATLAS_DIAGNOSTIC_ERROR,
                "self-loop-forbidden",
                edge->source_id,
                edge_index,
                "edge %zu is a forbidden self-loop on node %u",
                edge_index,
                edge->source_id);
            if (status != ATLAS_OK) {
                return status;
            }
        }
        if (!policy->allow_duplicate_edges) {
            const atlas_string *label = &document->strings[edge->label_index];
            for (size_t previous = 0; previous < edge_index; ++previous) {
                const atlas_edge *candidate = &document->edges[previous];
                const atlas_string *candidate_label =
                    &document->strings[candidate->label_index];
                if (edge->source_index == candidate->source_index &&
                    edge->target_index == candidate->target_index &&
                    label->length == candidate_label->length &&
                    memcmp(label->bytes, candidate_label->bytes, label->length) == 0) {
                    atlas_status status = atlas_diagnostics_add(
                        diagnostics,
                        ATLAS_DIAGNOSTIC_ERROR,
                        "duplicate-edge-forbidden",
                        edge->source_id,
                        edge_index,
                        "edge %zu duplicates edge %zu",
                        edge_index,
                        previous);
                    if (status != ATLAS_OK) {
                        return status;
                    }
                    break;
                }
            }
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_document_check_policy(atlas_context *context,
                                         const atlas_document *document,
                                         const atlas_policy *policy,
                                         atlas_diagnostics **out_diagnostics) {
    if (context == NULL || document == NULL || out_diagnostics == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_diagnostics = NULL;
    atlas_policy effective = policy == NULL ? atlas_policy_default() : *policy;
    atlas_diagnostics *diagnostics = atlas_calloc(context, 1, sizeof(*diagnostics));
    if (diagnostics == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = atlas_policy_add_count_errors(document,
                                                        &effective,
                                                        diagnostics);
    if (status == ATLAS_OK) {
        status = atlas_policy_add_node_errors(document, &effective, diagnostics);
    }
    if (status == ATLAS_OK) {
        status = atlas_policy_add_edge_errors(document, &effective, diagnostics);
    }
    if (status == ATLAS_OK && effective.require_metadata &&
        document->metadata_count == 0) {
        status = atlas_diagnostics_add(diagnostics,
                                       ATLAS_DIAGNOSTIC_ERROR,
                                       "metadata-required",
                                       ATLAS_INVALID_NODE_ID,
                                       ATLAS_DIAGNOSTIC_EDGE_NONE,
                                       "policy requires at least one metadata entry");
    }
    if (status == ATLAS_OK && !effective.allow_cycles) {
        atlas_plan *plan = NULL;
        atlas_status plan_status = atlas_plan_build(context, document, &plan);
        if (plan_status == ATLAS_ERR_CYCLE) {
            status = atlas_diagnostics_add(diagnostics,
                                           ATLAS_DIAGNOSTIC_ERROR,
                                           "cycle-forbidden",
                                           ATLAS_INVALID_NODE_ID,
                                           ATLAS_DIAGNOSTIC_EDGE_NONE,
                                           "document contains a cycle");
        } else if (plan_status != ATLAS_OK) {
            status = plan_status;
        }
        atlas_plan_destroy(plan);
    }
    if (status != ATLAS_OK) {
        atlas_diagnostics_destroy(diagnostics);
        return status;
    }
    *out_diagnostics = diagnostics;
    return diagnostics->error_count == 0 ? ATLAS_OK : ATLAS_ERR_POLICY;
}
