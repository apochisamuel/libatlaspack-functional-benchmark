#ifndef ATLAS_H
#define ATLAS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATLAS_FORMAT_VERSION 1u
#define ATLAS_NO_INDEX ((size_t)-1)

typedef enum atlas_status {
    ATLAS_OK = 0,
    ATLAS_ERR_ARGUMENT,
    ATLAS_ERR_NOMEM,
    ATLAS_ERR_TRUNCATED,
    ATLAS_ERR_MAGIC,
    ATLAS_ERR_VERSION,
    ATLAS_ERR_FORMAT,
    ATLAS_ERR_RANGE,
    ATLAS_ERR_DUPLICATE,
    ATLAS_ERR_REFERENCE,
    ATLAS_ERR_CYCLE,
    ATLAS_ERR_NOT_FOUND,
    ATLAS_ERR_CONFLICT,
    ATLAS_ERR_POLICY,
    ATLAS_ERR_IO,
    ATLAS_ERR_UNSUPPORTED
} atlas_status;

typedef enum atlas_visit_order {
    ATLAS_VISIT_BREADTH_FIRST = 0,
    ATLAS_VISIT_DEPTH_FIRST = 1
} atlas_visit_order;

typedef enum atlas_compare_result {
    ATLAS_COMPARE_EQUAL = 0,
    ATLAS_COMPARE_DIFFERENT = 1
} atlas_compare_result;

typedef enum atlas_diagnostic_severity {
    ATLAS_DIAGNOSTIC_INFO = 0,
    ATLAS_DIAGNOSTIC_WARNING = 1,
    ATLAS_DIAGNOSTIC_ERROR = 2
} atlas_diagnostic_severity;

typedef struct atlas_context atlas_context;
typedef struct atlas_document atlas_document;
typedef struct atlas_plan atlas_plan;
typedef struct atlas_path atlas_path;
typedef struct atlas_traversal atlas_traversal;
typedef struct atlas_components atlas_components;
typedef struct atlas_layers atlas_layers;
typedef struct atlas_selection atlas_selection;
typedef struct atlas_builder atlas_builder;
typedef struct atlas_diagnostics atlas_diagnostics;
typedef struct atlas_diff atlas_diff;
typedef struct atlas_strong_components atlas_strong_components;
typedef struct atlas_distance_map atlas_distance_map;
typedef struct atlas_rank atlas_rank;
typedef struct atlas_edge_selection atlas_edge_selection;
typedef struct atlas_catalog atlas_catalog;

typedef struct atlas_node_view {
    uint32_t id;
    const char *name;
    size_t name_length;
    size_t outgoing_edges;
    size_t incoming_edges;
    size_t attribute_count;
} atlas_node_view;

typedef struct atlas_edge_view {
    uint32_t source_id;
    uint32_t target_id;
    const char *label;
    size_t label_length;
} atlas_edge_view;

typedef struct atlas_attribute_view {
    uint32_t node_id;
    const char *key;
    size_t key_length;
    const char *value;
    size_t value_length;
} atlas_attribute_view;

typedef struct atlas_metadata_view {
    const char *key;
    size_t key_length;
    const char *value;
    size_t value_length;
} atlas_metadata_view;

typedef struct atlas_parse_options {
    int validate_checksum;
    int allow_unknown_sections;
    int normalize_names;
    int require_metadata_keys_unique;
    size_t allocation_limit;
} atlas_parse_options;

typedef struct atlas_statistics {
    size_t node_count;
    size_t edge_count;
    size_t attribute_count;
    size_t metadata_count;
    size_t source_count;
    size_t sink_count;
    size_t isolated_count;
    size_t self_loop_count;
    size_t duplicate_edge_count;
    size_t maximum_out_degree;
    size_t maximum_in_degree;
    double average_out_degree;
    double density;
} atlas_statistics;

typedef struct atlas_node_filter {
    const char *name_prefix;
    const char *attribute_key;
    const char *attribute_value;
    size_t minimum_out_degree;
    size_t maximum_out_degree;
    size_t minimum_in_degree;
    size_t maximum_in_degree;
    int include_isolated;
} atlas_node_filter;

typedef struct atlas_policy {
    size_t maximum_nodes;
    size_t maximum_edges;
    size_t maximum_attributes;
    size_t maximum_metadata;
    size_t maximum_out_degree;
    size_t maximum_in_degree;
    int allow_cycles;
    int allow_self_loops;
    int allow_duplicate_edges;
    int require_node_attributes;
    int require_metadata;
} atlas_policy;

typedef struct atlas_diagnostic_view {
    atlas_diagnostic_severity severity;
    const char *code;
    const char *message;
    uint32_t related_node_id;
    size_t related_edge_index;
} atlas_diagnostic_view;


typedef struct atlas_section_view {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t size;
    uint64_t auxiliary;
} atlas_section_view;

typedef struct atlas_catalog_entry_view {
    const char *text;
    size_t text_length;
    size_t usage_count;
} atlas_catalog_entry_view;

typedef enum atlas_merge_policy {
    ATLAS_MERGE_REJECT_CONFLICTS = 0,
    ATLAS_MERGE_PREFER_LEFT = 1,
    ATLAS_MERGE_PREFER_RIGHT = 2
} atlas_merge_policy;

typedef struct atlas_degree_bucket {
    size_t degree;
    size_t node_count;
} atlas_degree_bucket;

typedef struct atlas_diff_summary {
    size_t added_nodes;
    size_t removed_nodes;
    size_t changed_nodes;
    size_t added_edges;
    size_t removed_edges;
    size_t added_attributes;
    size_t removed_attributes;
    size_t changed_metadata;
} atlas_diff_summary;

atlas_context *atlas_context_create(void);
void atlas_context_destroy(atlas_context *context);
void atlas_context_reset(atlas_context *context);
const char *atlas_context_error(const atlas_context *context);
size_t atlas_context_allocation_limit(const atlas_context *context);
void atlas_context_set_allocation_limit(atlas_context *context, size_t limit);

atlas_parse_options atlas_parse_options_default(void);
atlas_status atlas_parse(atlas_context *context,
                         const uint8_t *data,
                         size_t size,
                         const atlas_parse_options *options,
                         atlas_document **out_document);
atlas_status atlas_parse_file(atlas_context *context,
                              const char *path,
                              const atlas_parse_options *options,
                              atlas_document **out_document);
void atlas_document_destroy(atlas_document *document);

size_t atlas_document_node_count(const atlas_document *document);
size_t atlas_document_edge_count(const atlas_document *document);
size_t atlas_document_string_count(const atlas_document *document);
size_t atlas_document_attribute_count(const atlas_document *document);
size_t atlas_document_metadata_count(const atlas_document *document);
uint16_t atlas_document_version(const atlas_document *document);
uint16_t atlas_document_flags(const atlas_document *document);
int atlas_document_is_normalized(const atlas_document *document);

size_t atlas_document_section_count(const atlas_document *document);
atlas_status atlas_document_section_at(const atlas_document *document,
                                       size_t index,
                                       atlas_section_view *out_view);
atlas_status atlas_document_string_at(const atlas_document *document,
                                      size_t index,
                                      const char **out_text,
                                      size_t *out_length);

atlas_status atlas_document_node_at(const atlas_document *document,
                                    size_t index,
                                    atlas_node_view *out_view);
atlas_status atlas_document_edge_at(const atlas_document *document,
                                    size_t index,
                                    atlas_edge_view *out_view);
atlas_status atlas_document_attribute_at(const atlas_document *document,
                                         size_t index,
                                         atlas_attribute_view *out_view);
atlas_status atlas_document_metadata_at(const atlas_document *document,
                                        size_t index,
                                        atlas_metadata_view *out_view);
atlas_status atlas_document_find_node(const atlas_document *document,
                                      const char *name,
                                      atlas_node_view *out_view);
atlas_status atlas_document_find_node_by_id(const atlas_document *document,
                                            uint32_t id,
                                            atlas_node_view *out_view);
atlas_status atlas_document_find_metadata(const atlas_document *document,
                                          const char *key,
                                          atlas_metadata_view *out_view);
atlas_status atlas_document_find_attribute(const atlas_document *document,
                                           uint32_t node_id,
                                           const char *key,
                                           atlas_attribute_view *out_view);

size_t atlas_document_out_degree(const atlas_document *document, uint32_t node_id);
size_t atlas_document_in_degree(const atlas_document *document, uint32_t node_id);
atlas_status atlas_document_outgoing_edge_at(const atlas_document *document,
                                             uint32_t node_id,
                                             size_t ordinal,
                                             atlas_edge_view *out_view);
atlas_status atlas_document_incoming_edge_at(const atlas_document *document,
                                             uint32_t node_id,
                                             size_t ordinal,
                                             atlas_edge_view *out_view);
atlas_status atlas_document_node_attribute_at(const atlas_document *document,
                                              uint32_t node_id,
                                              size_t ordinal,
                                              atlas_attribute_view *out_view);

atlas_status atlas_plan_build(atlas_context *context,
                              const atlas_document *document,
                              atlas_plan **out_plan);
void atlas_plan_destroy(atlas_plan *plan);
size_t atlas_plan_count(const atlas_plan *plan);
atlas_status atlas_plan_node_at(const atlas_plan *plan,
                                size_t index,
                                atlas_node_view *out_view);

atlas_status atlas_traversal_build(atlas_context *context,
                                   const atlas_document *document,
                                   uint32_t start_node_id,
                                   atlas_visit_order order,
                                   atlas_traversal **out_traversal);
void atlas_traversal_destroy(atlas_traversal *traversal);
size_t atlas_traversal_count(const atlas_traversal *traversal);
atlas_status atlas_traversal_node_at(const atlas_traversal *traversal,
                                     size_t index,
                                     atlas_node_view *out_view);

atlas_status atlas_path_find(atlas_context *context,
                             const atlas_document *document,
                             uint32_t source_node_id,
                             uint32_t target_node_id,
                             atlas_path **out_path);
void atlas_path_destroy(atlas_path *path);
size_t atlas_path_count(const atlas_path *path);
atlas_status atlas_path_node_at(const atlas_path *path,
                                size_t index,
                                atlas_node_view *out_view);
size_t atlas_path_edge_count(const atlas_path *path);
atlas_status atlas_path_edge_at(const atlas_path *path,
                                size_t index,
                                atlas_edge_view *out_view);

atlas_status atlas_components_build(atlas_context *context,
                                    const atlas_document *document,
                                    atlas_components **out_components);
void atlas_components_destroy(atlas_components *components);
size_t atlas_components_count(const atlas_components *components);
size_t atlas_components_size(const atlas_components *components, size_t component_index);
atlas_status atlas_components_node_component(const atlas_components *components,
                                              uint32_t node_id,
                                              size_t *out_component_index);

atlas_status atlas_layers_build(atlas_context *context,
                                const atlas_document *document,
                                atlas_layers **out_layers);
void atlas_layers_destroy(atlas_layers *layers);
size_t atlas_layers_count(const atlas_layers *layers);
size_t atlas_layers_size(const atlas_layers *layers, size_t layer_index);
atlas_status atlas_layers_node_at(const atlas_layers *layers,
                                  size_t layer_index,
                                  size_t ordinal,
                                  atlas_node_view *out_view);

atlas_status atlas_document_reachable(const atlas_document *document,
                                      uint32_t source_node_id,
                                      uint32_t target_node_id,
                                      int *out_reachable);
atlas_status atlas_document_descendant_count(const atlas_document *document,
                                             uint32_t source_node_id,
                                             size_t *out_count);
atlas_status atlas_document_ancestor_count(const atlas_document *document,
                                           uint32_t target_node_id,
                                           size_t *out_count);
atlas_status atlas_strong_components_build(atlas_context *context,
                                           const atlas_document *document,
                                           atlas_strong_components **out_components);
void atlas_strong_components_destroy(atlas_strong_components *components);
size_t atlas_strong_components_count(const atlas_strong_components *components);
size_t atlas_strong_components_size(const atlas_strong_components *components,
                                    size_t component_index);
atlas_status atlas_strong_components_node_component(
    const atlas_strong_components *components,
    uint32_t node_id,
    size_t *out_component_index);
size_t atlas_strong_components_cyclic_count(
    const atlas_strong_components *components);

atlas_status atlas_distance_map_build(atlas_context *context,
                                      const atlas_document *document,
                                      uint32_t start_node_id,
                                      int reverse,
                                      atlas_distance_map **out_map);
void atlas_distance_map_destroy(atlas_distance_map *map);
atlas_status atlas_distance_map_get(const atlas_distance_map *map,
                                    uint32_t node_id,
                                    size_t *out_distance);
size_t atlas_distance_map_reachable_count(const atlas_distance_map *map);
size_t atlas_distance_map_maximum(const atlas_distance_map *map);

atlas_status atlas_rank_compute(atlas_context *context,
                                const atlas_document *document,
                                double damping,
                                size_t iterations,
                                atlas_rank **out_rank);
void atlas_rank_destroy(atlas_rank *rank);
atlas_status atlas_rank_score(const atlas_rank *rank,
                              uint32_t node_id,
                              double *out_score);
atlas_status atlas_rank_node_at(const atlas_rank *rank,
                                size_t ordinal,
                                atlas_node_view *out_node,
                                double *out_score);

atlas_status atlas_document_find_cycle(atlas_context *context,
                                       const atlas_document *document,
                                       atlas_path **out_cycle);
atlas_status atlas_document_out_degree_histogram(
    atlas_context *context,
    const atlas_document *document,
    atlas_degree_bucket **out_buckets,
    size_t *out_count);
atlas_status atlas_document_in_degree_histogram(
    atlas_context *context,
    const atlas_document *document,
    atlas_degree_bucket **out_buckets,
    size_t *out_count);

atlas_status atlas_document_longest_path_length(atlas_context *context,
                                                const atlas_document *document,
                                                size_t *out_length);

atlas_status atlas_document_statistics(const atlas_document *document,
                                       atlas_statistics *out_statistics);
atlas_policy atlas_policy_default(void);
atlas_status atlas_document_check_policy(atlas_context *context,
                                         const atlas_document *document,
                                         const atlas_policy *policy,
                                         atlas_diagnostics **out_diagnostics);

atlas_node_filter atlas_node_filter_default(void);
atlas_status atlas_document_select(atlas_context *context,
                                   const atlas_document *document,
                                   const atlas_node_filter *filter,
                                   atlas_selection **out_selection);
void atlas_selection_destroy(atlas_selection *selection);
size_t atlas_selection_count(const atlas_selection *selection);
atlas_status atlas_selection_node_at(const atlas_selection *selection,
                                     size_t index,
                                     atlas_node_view *out_view);
atlas_status atlas_document_subgraph(atlas_context *context,
                                     const atlas_document *document,
                                     const atlas_selection *selection,
                                     atlas_document **out_document);

atlas_status atlas_document_search_names(atlas_context *context,
                                         const atlas_document *document,
                                         const char *needle,
                                         int case_sensitive,
                                         atlas_selection **out_selection);
atlas_status atlas_document_dependency_order(atlas_context *context,
                                             const atlas_document *document,
                                             uint32_t node_id,
                                             atlas_selection **out_selection);

atlas_status atlas_document_select_ids(atlas_context *context,
                                       const atlas_document *document,
                                       const uint32_t *node_ids,
                                       size_t node_id_count,
                                       atlas_selection **out_selection);
atlas_status atlas_document_select_sources(atlas_context *context,
                                           const atlas_document *document,
                                           atlas_selection **out_selection);
atlas_status atlas_document_select_sinks(atlas_context *context,
                                         const atlas_document *document,
                                         atlas_selection **out_selection);
atlas_status atlas_document_select_reachable(atlas_context *context,
                                             const atlas_document *document,
                                             uint32_t start_node_id,
                                             int include_start,
                                             atlas_selection **out_selection);

atlas_status atlas_document_select_edges_by_label(
    atlas_context *context,
    const atlas_document *document,
    const char *label,
    atlas_edge_selection **out_selection);
void atlas_edge_selection_destroy(atlas_edge_selection *selection);
size_t atlas_edge_selection_count(const atlas_edge_selection *selection);
atlas_status atlas_edge_selection_edge_at(const atlas_edge_selection *selection,
                                          size_t index,
                                          atlas_edge_view *out_view);

atlas_builder *atlas_builder_create(void);
void atlas_builder_destroy(atlas_builder *builder);
void atlas_builder_clear(atlas_builder *builder);
atlas_status atlas_builder_add_node(atlas_builder *builder,
                                    uint32_t id,
                                    const char *name);
atlas_status atlas_builder_add_edge(atlas_builder *builder,
                                    uint32_t source_id,
                                    uint32_t target_id,
                                    const char *label);
atlas_status atlas_builder_add_attribute(atlas_builder *builder,
                                         uint32_t node_id,
                                         const char *key,
                                         const char *value);
atlas_status atlas_builder_add_metadata(atlas_builder *builder,
                                        const char *key,
                                        const char *value);
size_t atlas_builder_node_count(const atlas_builder *builder);
size_t atlas_builder_edge_count(const atlas_builder *builder);
size_t atlas_builder_attribute_count(const atlas_builder *builder);
size_t atlas_builder_metadata_count(const atlas_builder *builder);
atlas_status atlas_builder_rename_node(atlas_builder *builder,
                                       uint32_t node_id,
                                       const char *new_name);
atlas_status atlas_builder_remove_node(atlas_builder *builder,
                                       uint32_t node_id,
                                       int remove_incident_edges);
atlas_status atlas_builder_remove_edge(atlas_builder *builder,
                                       uint32_t source_id,
                                       uint32_t target_id,
                                       const char *label);
atlas_status atlas_builder_set_attribute(atlas_builder *builder,
                                         uint32_t node_id,
                                         const char *key,
                                         const char *value);
atlas_status atlas_builder_remove_attribute(atlas_builder *builder,
                                            uint32_t node_id,
                                            const char *key);
atlas_status atlas_builder_set_metadata(atlas_builder *builder,
                                        const char *key,
                                        const char *value);
atlas_status atlas_builder_remove_metadata(atlas_builder *builder,
                                           const char *key);
atlas_status atlas_builder_serialize(atlas_context *context,
                                     const atlas_builder *builder,
                                     uint8_t **out_data,
                                     size_t *out_size);
atlas_status atlas_builder_write_file(atlas_context *context,
                                      const atlas_builder *builder,
                                      const char *path);
atlas_status atlas_builder_parse(atlas_context *context,
                                 const atlas_builder *builder,
                                 const atlas_parse_options *options,
                                 atlas_document **out_document);

atlas_status atlas_document_serialize(atlas_context *context,
                                      const atlas_document *document,
                                      uint8_t **out_data,
                                      size_t *out_size);
atlas_status atlas_document_write_file(atlas_context *context,
                                       const atlas_document *document,
                                       const char *path);
atlas_status atlas_document_prune_reachable(atlas_context *context,
                                            const atlas_document *document,
                                            uint32_t start_node_id,
                                            atlas_document **out_document);
atlas_status atlas_document_compact_ids(atlas_context *context,
                                        const atlas_document *document,
                                        uint32_t first_id,
                                        atlas_document **out_document);
atlas_status atlas_document_filter_edge_label(atlas_context *context,
                                              const atlas_document *document,
                                              const char *label,
                                              int keep_matches,
                                              atlas_document **out_document);

atlas_status atlas_document_reverse(atlas_context *context,
                                    const atlas_document *document,
                                    atlas_document **out_document);
atlas_status atlas_document_transitive_reduction(atlas_context *context,
                                                 const atlas_document *document,
                                                 atlas_document **out_document);
atlas_status atlas_document_merge(atlas_context *context,
                                  const atlas_document *left,
                                  const atlas_document *right,
                                  atlas_merge_policy policy,
                                  atlas_document **out_document);
uint64_t atlas_document_fingerprint(const atlas_document *document);

atlas_status atlas_document_clone(atlas_context *context,
                                  const atlas_document *document,
                                  atlas_document **out_clone);
atlas_status atlas_document_compare(const atlas_document *left,
                                    const atlas_document *right,
                                    atlas_compare_result *out_result);

atlas_status atlas_diff_compute(atlas_context *context,
                                const atlas_document *before,
                                const atlas_document *after,
                                atlas_diff **out_diff);
void atlas_diff_destroy(atlas_diff *diff);
atlas_diff_summary atlas_diff_get_summary(const atlas_diff *diff);

atlas_status atlas_document_label_catalog(atlas_context *context,
                                          const atlas_document *document,
                                          atlas_catalog **out_catalog);
atlas_status atlas_document_attribute_key_catalog(atlas_context *context,
                                                  const atlas_document *document,
                                                  atlas_catalog **out_catalog);
void atlas_catalog_destroy(atlas_catalog *catalog);
size_t atlas_catalog_count(const atlas_catalog *catalog);
atlas_status atlas_catalog_at(const atlas_catalog *catalog,
                              size_t index,
                              atlas_catalog_entry_view *out_view);

atlas_status atlas_document_validate(atlas_context *context,
                                     const atlas_document *document,
                                     atlas_diagnostics **out_diagnostics);
void atlas_diagnostics_destroy(atlas_diagnostics *diagnostics);
size_t atlas_diagnostics_count(const atlas_diagnostics *diagnostics);
size_t atlas_diagnostics_error_count(const atlas_diagnostics *diagnostics);
size_t atlas_diagnostics_warning_count(const atlas_diagnostics *diagnostics);
atlas_status atlas_diagnostics_at(const atlas_diagnostics *diagnostics,
                                  size_t index,
                                  atlas_diagnostic_view *out_view);

atlas_status atlas_document_report(atlas_context *context,
                                   const atlas_document *document,
                                   char **out_text,
                                   size_t *out_length);
atlas_status atlas_document_to_json(atlas_context *context,
                                    const atlas_document *document,
                                    int pretty,
                                    char **out_text,
                                    size_t *out_length);
atlas_status atlas_document_to_csv(atlas_context *context,
                                   const atlas_document *document,
                                   char **out_text,
                                   size_t *out_length);
atlas_status atlas_document_adjacency_csv(atlas_context *context,
                                          const atlas_document *document,
                                          char **out_text,
                                          size_t *out_length);
atlas_status atlas_document_schema_report(atlas_context *context,
                                          const atlas_document *document,
                                          char **out_text,
                                          size_t *out_length);

atlas_status atlas_document_to_markdown(atlas_context *context,
                                        const atlas_document *document,
                                        char **out_text,
                                        size_t *out_length);
atlas_status atlas_document_to_graphml(atlas_context *context,
                                       const atlas_document *document,
                                       char **out_text,
                                       size_t *out_length);
atlas_status atlas_document_to_mermaid(atlas_context *context,
                                       const atlas_document *document,
                                       char **out_text,
                                       size_t *out_length);
atlas_status atlas_document_edge_list(atlas_context *context,
                                      const atlas_document *document,
                                      char **out_text,
                                      size_t *out_length);
atlas_status atlas_document_topology_report(atlas_context *context,
                                            const atlas_document *document,
                                            char **out_text,
                                            size_t *out_length);
atlas_status atlas_document_node_report(atlas_context *context,
                                        const atlas_document *document,
                                        uint32_t node_id,
                                        char **out_text,
                                        size_t *out_length);

atlas_status atlas_document_reachability_csv(atlas_context *context,
                                             const atlas_document *document,
                                             char **out_text,
                                             size_t *out_length);
atlas_status atlas_document_component_report(atlas_context *context,
                                             const atlas_document *document,
                                             char **out_text,
                                             size_t *out_length);
atlas_status atlas_document_attribute_report(atlas_context *context,
                                             const atlas_document *document,
                                             char **out_text,
                                             size_t *out_length);
atlas_status atlas_document_integrity_report(atlas_context *context,
                                             const atlas_document *document,
                                             char **out_text,
                                             size_t *out_length);

atlas_status atlas_document_to_dot(atlas_context *context,
                                   const atlas_document *document,
                                   char **out_text,
                                   size_t *out_length);
atlas_status atlas_document_manifest(atlas_context *context,
                                     const atlas_document *document,
                                     char **out_text,
                                     size_t *out_length);

void atlas_free(void *pointer);
const char *atlas_status_name(atlas_status status);
const char *atlas_version_string(void);
uint32_t atlas_version_number(void);

#ifdef __cplusplus
}
#endif
#endif
