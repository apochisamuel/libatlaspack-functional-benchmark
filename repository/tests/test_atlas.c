#include "atlas.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPECT_TRUE(condition)                                                   \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr,                                                      \
                    "expectation failed at %s:%d: %s\n",                       \
                    __FILE__,                                                    \
                    __LINE__,                                                    \
                    #condition);                                                 \
            abort();                                                             \
        }                                                                        \
    } while (0)

static void expect_status(atlas_context *context,
                          atlas_status actual,
                          atlas_status expected,
                          const char *operation) {
    if (actual != expected) {
        fprintf(stderr,
                "%s: expected %s, got %s (%s)\n",
                operation,
                atlas_status_name(expected),
                atlas_status_name(actual),
                atlas_context_error(context));
        abort();
    }
}

static void expect_ok(atlas_context *context,
                      atlas_status status,
                      const char *operation) {
    expect_status(context, status, ATLAS_OK, operation);
}

static void expect_text_contains(const char *text, const char *needle) {
    EXPECT_TRUE(text != NULL);
    if (strstr(text, needle) == NULL) {
        fprintf(stderr, "missing text needle: %s\n--- text ---\n%s\n--- end ---\n",
                needle,
                text);
        abort();
    }
}

static atlas_builder *build_primary_builder(void) {
    atlas_builder *builder = atlas_builder_create();
    EXPECT_TRUE(builder != NULL);
    EXPECT_TRUE(atlas_builder_add_node(builder, 10, "Root Service") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 20, "API Gateway") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 30, "Worker") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 40, "Database") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 50, "Audit Log") == ATLAS_OK);

    EXPECT_TRUE(atlas_builder_add_edge(builder, 10, 20, "depends") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 10, 30, "depends") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 20, 40, "uses") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 30, 40, "uses") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 20, 50, "emits") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 30, 50, "emits") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 10, 40, "depends") == ATLAS_OK);

    EXPECT_TRUE(atlas_builder_add_attribute(builder, 10, "kind", "entry") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 10, "tier", "edge") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 20, "kind", "service") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 20, "tier", "application") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 30, "kind", "service") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 30, "tier", "application") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 40, "kind", "storage") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 50, "kind", "storage") == ATLAS_OK);

    EXPECT_TRUE(atlas_builder_add_metadata(builder, "project", "libatlaspack") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_metadata(builder, "environment", "test") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_metadata(builder, "format", "ATPK") == ATLAS_OK);
    return builder;
}

static atlas_document *parse_builder(atlas_context *context,
                                     const atlas_builder *builder) {
    atlas_document *document = NULL;
    atlas_parse_options options = atlas_parse_options_default();
    expect_ok(context,
              atlas_builder_parse(context, builder, &options, &document),
              "builder parse");
    EXPECT_TRUE(document != NULL);
    return document;
}

static void test_basic_views(atlas_context *context,
                             const atlas_document *document) {
    EXPECT_TRUE(atlas_document_version(document) == ATLAS_FORMAT_VERSION);
    EXPECT_TRUE(atlas_document_node_count(document) == 5);
    EXPECT_TRUE(atlas_document_edge_count(document) == 7);
    EXPECT_TRUE(atlas_document_attribute_count(document) == 8);
    EXPECT_TRUE(atlas_document_metadata_count(document) == 3);
    EXPECT_TRUE(atlas_document_string_count(document) >= 15);
    EXPECT_TRUE(atlas_document_section_count(document) == 5);
    EXPECT_TRUE(atlas_document_is_normalized(document));

    atlas_node_view node;
    expect_ok(context,
              atlas_document_find_node(document, "root service", &node),
              "find normalized node");
    EXPECT_TRUE(node.id == 10);
    EXPECT_TRUE(node.outgoing_edges == 3);
    EXPECT_TRUE(node.incoming_edges == 0);
    EXPECT_TRUE(node.attribute_count == 2);

    expect_ok(context,
              atlas_document_find_node_by_id(document, 40, &node),
              "find node by id");
    EXPECT_TRUE(strcmp(node.name, "database") == 0);
    EXPECT_TRUE(node.incoming_edges == 3);
    EXPECT_TRUE(node.outgoing_edges == 0);

    atlas_metadata_view metadata;
    expect_ok(context,
              atlas_document_find_metadata(document, "project", &metadata),
              "find metadata");
    EXPECT_TRUE(metadata.value_length == strlen("libatlaspack"));
    EXPECT_TRUE(memcmp(metadata.value, "libatlaspack", metadata.value_length) == 0);

    atlas_attribute_view attribute;
    expect_ok(context,
              atlas_document_find_attribute(document, 20, "tier", &attribute),
              "find attribute");
    EXPECT_TRUE(strcmp(attribute.value, "application") == 0);

    atlas_edge_view edge;
    expect_ok(context,
              atlas_document_outgoing_edge_at(document, 10, 0, &edge),
              "outgoing edge");
    EXPECT_TRUE(edge.source_id == 10);
    expect_ok(context,
              atlas_document_incoming_edge_at(document, 40, 0, &edge),
              "incoming edge");
    EXPECT_TRUE(edge.target_id == 40);
    EXPECT_TRUE(atlas_document_out_degree(document, 10) == 3);
    EXPECT_TRUE(atlas_document_in_degree(document, 40) == 3);

    atlas_section_view section;
    expect_ok(context,
              atlas_document_section_at(document, 0, &section),
              "section at");
    EXPECT_TRUE(section.size > 0);
    const char *string_text = NULL;
    size_t string_length = 0;
    expect_ok(context,
              atlas_document_string_at(document,
                                       0,
                                       &string_text,
                                       &string_length),
              "string at");
    EXPECT_TRUE(string_text != NULL);
    EXPECT_TRUE(string_length > 0);
}

static void test_graph_algorithms(atlas_context *context,
                                  const atlas_document *document) {
    atlas_plan *plan = NULL;
    expect_ok(context,
              atlas_plan_build(context, document, &plan),
              "build plan");
    EXPECT_TRUE(atlas_plan_count(plan) == 5);
    atlas_node_view node;
    expect_ok(context,
              atlas_plan_node_at(plan, 0, &node),
              "plan node");
    EXPECT_TRUE(node.id == 10);
    atlas_plan_destroy(plan);

    atlas_layers *layers = NULL;
    expect_ok(context,
              atlas_layers_build(context, document, &layers),
              "build layers");
    EXPECT_TRUE(atlas_layers_count(layers) == 3);
    EXPECT_TRUE(atlas_layers_size(layers, 0) == 1);
    EXPECT_TRUE(atlas_layers_size(layers, 1) == 2);
    EXPECT_TRUE(atlas_layers_size(layers, 2) == 2);
    atlas_layers_destroy(layers);

    atlas_traversal *traversal = NULL;
    expect_ok(context,
              atlas_traversal_build(context,
                                    document,
                                    10,
                                    ATLAS_VISIT_BREADTH_FIRST,
                                    &traversal),
              "breadth traversal");
    EXPECT_TRUE(atlas_traversal_count(traversal) == 5);
    atlas_traversal_destroy(traversal);
    expect_ok(context,
              atlas_traversal_build(context,
                                    document,
                                    10,
                                    ATLAS_VISIT_DEPTH_FIRST,
                                    &traversal),
              "depth traversal");
    EXPECT_TRUE(atlas_traversal_count(traversal) == 5);
    atlas_traversal_destroy(traversal);

    atlas_path *path = NULL;
    expect_ok(context,
              atlas_path_find(context, document, 10, 50, &path),
              "find path");
    EXPECT_TRUE(atlas_path_count(path) == 3);
    EXPECT_TRUE(atlas_path_edge_count(path) == 2);
    atlas_edge_view path_edge;
    expect_ok(context,
              atlas_path_edge_at(path, 1, &path_edge),
              "path edge");
    EXPECT_TRUE(path_edge.target_id == 50);
    atlas_path_destroy(path);

    int reachable = 0;
    expect_ok(context,
              atlas_document_reachable(document, 10, 50, &reachable),
              "reachable");
    EXPECT_TRUE(reachable);
    expect_ok(context,
              atlas_document_reachable(document, 50, 10, &reachable),
              "not reachable");
    EXPECT_TRUE(!reachable);
    size_t count = 0;
    expect_ok(context,
              atlas_document_descendant_count(document, 10, &count),
              "descendant count");
    EXPECT_TRUE(count == 4);
    expect_ok(context,
              atlas_document_ancestor_count(document, 40, &count),
              "ancestor count");
    EXPECT_TRUE(count == 3);
    expect_ok(context,
              atlas_document_longest_path_length(context, document, &count),
              "longest path");
    EXPECT_TRUE(count == 2);

    atlas_components *components = NULL;
    expect_ok(context,
              atlas_components_build(context, document, &components),
              "weak components");
    EXPECT_TRUE(atlas_components_count(components) == 1);
    EXPECT_TRUE(atlas_components_size(components, 0) == 5);
    atlas_components_destroy(components);

    atlas_strong_components *strong = NULL;
    expect_ok(context,
              atlas_strong_components_build(context, document, &strong),
              "strong components");
    EXPECT_TRUE(atlas_strong_components_count(strong) == 5);
    EXPECT_TRUE(atlas_strong_components_cyclic_count(strong) == 0);
    atlas_strong_components_destroy(strong);

    atlas_distance_map *distance = NULL;
    expect_ok(context,
              atlas_distance_map_build(context, document, 10, 0, &distance),
              "distance map");
    EXPECT_TRUE(atlas_distance_map_reachable_count(distance) == 5);
    EXPECT_TRUE(atlas_distance_map_maximum(distance) == 2);
    expect_ok(context,
              atlas_distance_map_get(distance, 50, &count),
              "distance get");
    EXPECT_TRUE(count == 2);
    atlas_distance_map_destroy(distance);

    atlas_rank *rank = NULL;
    expect_ok(context,
              atlas_rank_compute(context, document, 0.85, 20, &rank),
              "rank compute");
    double score = 0.0;
    expect_ok(context,
              atlas_rank_score(rank, 40, &score),
              "rank score");
    EXPECT_TRUE(score > 0.0 && score < 1.0);
    expect_ok(context,
              atlas_rank_node_at(rank, 0, &node, &score),
              "rank node");
    EXPECT_TRUE(score > 0.0);
    atlas_rank_destroy(rank);
}

static void test_statistics_and_selections(atlas_context *context,
                                           const atlas_document *document) {
    atlas_statistics statistics;
    expect_ok(context,
              atlas_document_statistics(document, &statistics),
              "statistics");
    EXPECT_TRUE(statistics.node_count == 5);
    EXPECT_TRUE(statistics.edge_count == 7);
    EXPECT_TRUE(statistics.source_count == 1);
    EXPECT_TRUE(statistics.sink_count == 2);
    EXPECT_TRUE(statistics.isolated_count == 0);
    EXPECT_TRUE(statistics.maximum_out_degree == 3);
    EXPECT_TRUE(statistics.maximum_in_degree == 3);

    atlas_degree_bucket *buckets = NULL;
    size_t bucket_count = 0;
    expect_ok(context,
              atlas_document_out_degree_histogram(context,
                                                  document,
                                                  &buckets,
                                                  &bucket_count),
              "out degree histogram");
    EXPECT_TRUE(bucket_count > 0);
    atlas_free(buckets);
    expect_ok(context,
              atlas_document_in_degree_histogram(context,
                                                 document,
                                                 &buckets,
                                                 &bucket_count),
              "in degree histogram");
    EXPECT_TRUE(bucket_count > 0);
    atlas_free(buckets);

    atlas_selection *selection = NULL;
    expect_ok(context,
              atlas_document_select_sources(context, document, &selection),
              "select sources");
    EXPECT_TRUE(atlas_selection_count(selection) == 1);
    atlas_selection_destroy(selection);
    expect_ok(context,
              atlas_document_select_sinks(context, document, &selection),
              "select sinks");
    EXPECT_TRUE(atlas_selection_count(selection) == 2);
    atlas_selection_destroy(selection);
    expect_ok(context,
              atlas_document_select_reachable(context,
                                              document,
                                              20,
                                              1,
                                              &selection),
              "select reachable");
    EXPECT_TRUE(atlas_selection_count(selection) == 3);
    atlas_selection_destroy(selection);

    atlas_node_filter filter = atlas_node_filter_default();
    filter.attribute_key = "kind";
    filter.attribute_value = "storage";
    expect_ok(context,
              atlas_document_select(context, document, &filter, &selection),
              "attribute selection");
    EXPECT_TRUE(atlas_selection_count(selection) == 2);
    atlas_document *subgraph = NULL;
    expect_ok(context,
              atlas_document_subgraph(context,
                                      document,
                                      selection,
                                      &subgraph),
              "subgraph");
    EXPECT_TRUE(atlas_document_node_count(subgraph) == 2);
    atlas_document_destroy(subgraph);
    atlas_selection_destroy(selection);

    expect_ok(context,
              atlas_document_search_names(context,
                                          document,
                                          "service",
                                          0,
                                          &selection),
              "search names");
    EXPECT_TRUE(atlas_selection_count(selection) == 1);
    atlas_selection_destroy(selection);

    expect_ok(context,
              atlas_document_dependency_order(context,
                                              document,
                                              40,
                                              &selection),
              "dependency order");
    EXPECT_TRUE(atlas_selection_count(selection) >= 3);
    atlas_selection_destroy(selection);

    atlas_edge_selection *edges = NULL;
    expect_ok(context,
              atlas_document_select_edges_by_label(context,
                                                   document,
                                                   "uses",
                                                   &edges),
              "edge label selection");
    EXPECT_TRUE(atlas_edge_selection_count(edges) == 2);
    atlas_edge_selection_destroy(edges);

    atlas_catalog *catalog = NULL;
    expect_ok(context,
              atlas_document_label_catalog(context, document, &catalog),
              "label catalog");
    EXPECT_TRUE(atlas_catalog_count(catalog) == 3);
    atlas_catalog_entry_view catalog_entry;
    expect_ok(context,
              atlas_catalog_at(catalog, 0, &catalog_entry),
              "catalog entry");
    EXPECT_TRUE(catalog_entry.usage_count >= 2);
    atlas_catalog_destroy(catalog);
    expect_ok(context,
              atlas_document_attribute_key_catalog(context,
                                                   document,
                                                   &catalog),
              "attribute catalog");
    EXPECT_TRUE(atlas_catalog_count(catalog) == 2);
    atlas_catalog_destroy(catalog);
}

static void check_generated_text(atlas_context *context,
                                 const atlas_document *document) {
    char *text = NULL;
    size_t length = 0;
#define CHECK_TEXT(call, needle)                                                  \
    do {                                                                         \
        expect_ok(context, (call), #call);                                        \
        EXPECT_TRUE(length > 0);                                                  \
        expect_text_contains(text, (needle));                                     \
        atlas_free(text);                                                         \
        text = NULL;                                                              \
        length = 0;                                                               \
    } while (0)

    CHECK_TEXT(atlas_document_report(context, document, &text, &length),
               "\"nodes\":5");
    CHECK_TEXT(atlas_document_to_json(context, document, 1, &text, &length),
               "\"metadata\"");
    CHECK_TEXT(atlas_document_to_csv(context, document, &text, &length),
               "kind,id,name");
    CHECK_TEXT(atlas_document_adjacency_csv(context, document, &text, &length),
               "root service");
    CHECK_TEXT(atlas_document_schema_report(context, document, &text, &length),
               "ATPK SCHEMA REPORT");
    CHECK_TEXT(atlas_document_to_markdown(context, document, &text, &length),
               "# ATPK Document");
    CHECK_TEXT(atlas_document_to_graphml(context, document, &text, &length),
               "<graphml");
    CHECK_TEXT(atlas_document_to_mermaid(context, document, &text, &length),
               "flowchart");
    CHECK_TEXT(atlas_document_edge_list(context, document, &text, &length),
               "ATPK EDGE LIST");
    CHECK_TEXT(atlas_document_topology_report(context, document, &text, &length),
               "acyclic=true");
    CHECK_TEXT(atlas_document_node_report(context, document, 20, &text, &length),
               "ATPK NODE REPORT");
    CHECK_TEXT(atlas_document_reachability_csv(context, document, &text, &length),
               "root service");
    CHECK_TEXT(atlas_document_component_report(context, document, &text, &length),
               "weak_component_count=1");
    CHECK_TEXT(atlas_document_attribute_report(context, document, &text, &length),
               "distinct_keys=2");
    CHECK_TEXT(atlas_document_integrity_report(context, document, &text, &length),
               "ATPK INTEGRITY REPORT");
    CHECK_TEXT(atlas_document_to_dot(context, document, &text, &length),
               "digraph");
    CHECK_TEXT(atlas_document_manifest(context, document, &text, &length),
               "longest_path=2");
#undef CHECK_TEXT
}

static void test_round_trip_and_transforms(atlas_context *context,
                                           const atlas_document *document) {
    uint8_t *bytes = NULL;
    size_t byte_count = 0;
    expect_ok(context,
              atlas_document_serialize(context,
                                       document,
                                       &bytes,
                                       &byte_count),
              "document serialize");
    EXPECT_TRUE(bytes != NULL && byte_count > 32);
    atlas_document *round_trip = NULL;
    expect_ok(context,
              atlas_parse(context,
                          bytes,
                          byte_count,
                          NULL,
                          &round_trip),
              "round trip parse");
    atlas_free(bytes);
    atlas_compare_result comparison = ATLAS_COMPARE_DIFFERENT;
    expect_ok(context,
              atlas_document_compare(document,
                                     round_trip,
                                     &comparison),
              "round trip compare");
    EXPECT_TRUE(comparison == ATLAS_COMPARE_EQUAL);
    EXPECT_TRUE(atlas_document_fingerprint(document) ==
                atlas_document_fingerprint(round_trip));

    atlas_document *clone = NULL;
    expect_ok(context,
              atlas_document_clone(context, document, &clone),
              "clone");
    expect_ok(context,
              atlas_document_compare(document, clone, &comparison),
              "clone compare");
    EXPECT_TRUE(comparison == ATLAS_COMPARE_EQUAL);
    atlas_document_destroy(clone);

    atlas_document *reduced = NULL;
    expect_ok(context,
              atlas_document_transitive_reduction(context,
                                                  document,
                                                  &reduced),
              "transitive reduction");
    EXPECT_TRUE(atlas_document_edge_count(reduced) == 6);
    atlas_diff *diff = NULL;
    expect_ok(context,
              atlas_diff_compute(context, document, reduced, &diff),
              "diff");
    atlas_diff_summary summary = atlas_diff_get_summary(diff);
    EXPECT_TRUE(summary.removed_edges == 1);
    EXPECT_TRUE(summary.added_edges == 0);
    atlas_diff_destroy(diff);

    atlas_document *reversed = NULL;
    expect_ok(context,
              atlas_document_reverse(context, document, &reversed),
              "reverse");
    EXPECT_TRUE(atlas_document_out_degree(reversed, 40) == 3);
    EXPECT_TRUE(atlas_document_in_degree(reversed, 10) == 3);
    atlas_document_destroy(reversed);

    atlas_document *filtered = NULL;
    expect_ok(context,
              atlas_document_filter_edge_label(context,
                                               document,
                                               "uses",
                                               1,
                                               &filtered),
              "filter edges");
    EXPECT_TRUE(atlas_document_edge_count(filtered) == 2);
    atlas_document_destroy(filtered);

    atlas_document *pruned = NULL;
    expect_ok(context,
              atlas_document_prune_reachable(context,
                                             document,
                                             20,
                                             &pruned),
              "prune reachable");
    EXPECT_TRUE(atlas_document_node_count(pruned) == 3);
    EXPECT_TRUE(atlas_document_edge_count(pruned) == 2);
    atlas_document_destroy(pruned);

    atlas_document *compacted = NULL;
    expect_ok(context,
              atlas_document_compact_ids(context,
                                         document,
                                         100,
                                         &compacted),
              "compact ids");
    atlas_node_view node;
    expect_ok(context,
              atlas_document_find_node_by_id(compacted, 100, &node),
              "compacted first node");
    EXPECT_TRUE(strcmp(node.name, "root service") == 0);
    atlas_document_destroy(compacted);

    atlas_document *merged = NULL;
    expect_ok(context,
              atlas_document_merge(context,
                                   document,
                                   reduced,
                                   ATLAS_MERGE_REJECT_CONFLICTS,
                                   &merged),
              "merge compatible documents");
    EXPECT_TRUE(atlas_document_edge_count(merged) == 7);
    atlas_document_destroy(merged);
    atlas_document_destroy(reduced);
    atlas_document_destroy(round_trip);
}

static void test_diagnostics_and_policy(atlas_context *context,
                                        const atlas_document *document) {
    atlas_diagnostics *diagnostics = NULL;
    expect_ok(context,
              atlas_document_validate(context, document, &diagnostics),
              "validate document");
    EXPECT_TRUE(atlas_diagnostics_error_count(diagnostics) == 0);
    atlas_diagnostics_destroy(diagnostics);

    atlas_policy policy = atlas_policy_default();
    policy.maximum_edges = 2;
    atlas_status status = atlas_document_check_policy(context,
                                                      document,
                                                      &policy,
                                                      &diagnostics);
    expect_status(context, status, ATLAS_ERR_POLICY, "policy rejection");
    EXPECT_TRUE(diagnostics != NULL);
    EXPECT_TRUE(atlas_diagnostics_error_count(diagnostics) >= 1);
    atlas_diagnostic_view view;
    expect_ok(context,
              atlas_diagnostics_at(diagnostics, 0, &view),
              "policy diagnostic");
    EXPECT_TRUE(view.code != NULL && view.message != NULL);
    atlas_diagnostics_destroy(diagnostics);
}

static void test_builder_mutations(atlas_context *context) {
    atlas_builder *builder = atlas_builder_create();
    EXPECT_TRUE(builder != NULL);
    EXPECT_TRUE(atlas_builder_add_node(builder, 1, "One") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 2, "Two") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 1, 2, "links") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 1, "kind", "old") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_metadata(builder, "owner", "first") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_rename_node(builder, 2, "Second") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_set_attribute(builder, 1, "kind", "new") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_set_metadata(builder, "owner", "second") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_remove_edge(builder, 1, 2, "links") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 2, 1, "returns") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_remove_attribute(builder, 1, "kind") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_attribute(builder, 2, "kind", "replacement") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_remove_metadata(builder, "owner") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_metadata(builder, "owner", "final") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_node_count(builder) == 2);
    EXPECT_TRUE(atlas_builder_edge_count(builder) == 1);
    EXPECT_TRUE(atlas_builder_attribute_count(builder) == 1);
    EXPECT_TRUE(atlas_builder_metadata_count(builder) == 1);
    atlas_document *document = parse_builder(context, builder);
    atlas_node_view node;
    expect_ok(context,
              atlas_document_find_node(document, "second", &node),
              "mutated node");
    EXPECT_TRUE(node.id == 2);
    atlas_document_destroy(document);
    EXPECT_TRUE(atlas_builder_remove_node(builder, 1, 1) == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_node_count(builder) == 1);
    EXPECT_TRUE(atlas_builder_edge_count(builder) == 0);
    atlas_builder_destroy(builder);
}

static void test_cycle_behavior(atlas_context *context) {
    atlas_builder *builder = atlas_builder_create();
    EXPECT_TRUE(builder != NULL);
    EXPECT_TRUE(atlas_builder_add_node(builder, 1, "A") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 2, "B") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_node(builder, 3, "C") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 1, 2, "next") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 2, 3, "next") == ATLAS_OK);
    EXPECT_TRUE(atlas_builder_add_edge(builder, 3, 1, "next") == ATLAS_OK);
    atlas_document *document = parse_builder(context, builder);
    atlas_plan *plan = NULL;
    expect_status(context,
                  atlas_plan_build(context, document, &plan),
                  ATLAS_ERR_CYCLE,
                  "cycle plan");
    EXPECT_TRUE(plan == NULL);
    atlas_path *cycle = NULL;
    expect_ok(context,
              atlas_document_find_cycle(context, document, &cycle),
              "find cycle");
    EXPECT_TRUE(atlas_path_count(cycle) >= 3);
    atlas_path_destroy(cycle);
    atlas_strong_components *strong = NULL;
    expect_ok(context,
              atlas_strong_components_build(context, document, &strong),
              "cycle strong components");
    EXPECT_TRUE(atlas_strong_components_count(strong) == 1);
    EXPECT_TRUE(atlas_strong_components_cyclic_count(strong) == 1);
    atlas_strong_components_destroy(strong);
    char *topology = NULL;
    size_t length = 0;
    expect_ok(context,
              atlas_document_topology_report(context,
                                             document,
                                             &topology,
                                             &length),
              "cycle topology report");
    EXPECT_TRUE(length > 0);
    expect_text_contains(topology, "acyclic=false");
    atlas_free(topology);
    atlas_document_destroy(document);
    atlas_builder_destroy(builder);
}

int main(void) {
    atlas_context *context = atlas_context_create();
    EXPECT_TRUE(context != NULL);
    atlas_builder *builder = build_primary_builder();
    atlas_document *document = parse_builder(context, builder);

    test_basic_views(context, document);
    test_graph_algorithms(context, document);
    test_statistics_and_selections(context, document);
    check_generated_text(context, document);
    test_round_trip_and_transforms(context, document);
    test_diagnostics_and_policy(context, document);
    test_builder_mutations(context);
    test_cycle_behavior(context);

    atlas_document_destroy(document);
    atlas_builder_destroy(builder);
    atlas_context_destroy(context);
    puts("all tests passed");
    return 0;
}
