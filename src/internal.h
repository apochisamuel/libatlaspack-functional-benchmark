#ifndef ATLAS_INTERNAL_H
#define ATLAS_INTERNAL_H

#include "atlas.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATLAS_MAGIC 0x4b505441u
#define ATLAS_VERSION ATLAS_FORMAT_VERSION
#define ATLAS_HEADER_SIZE 32u
#define ATLAS_DIRECTORY_ENTRY_SIZE 32u
#define ATLAS_MAX_SECTIONS 4096u
#define ATLAS_MAX_STRINGS 1048576u
#define ATLAS_MAX_NODES 1048576u
#define ATLAS_MAX_EDGES 4194304u
#define ATLAS_MAX_ATTRIBUTES 4194304u
#define ATLAS_MAX_METADATA 1048576u

#define ATLAS_SECTION_STRINGS 1u
#define ATLAS_SECTION_NODES 2u
#define ATLAS_SECTION_EDGES 3u
#define ATLAS_SECTION_ATTRIBUTES 4u
#define ATLAS_SECTION_METADATA 5u

#define ATLAS_INVALID_NODE_ID UINT32_MAX
#define ATLAS_DIAGNOSTIC_EDGE_NONE SIZE_MAX

typedef struct atlas_reader {
    const uint8_t *data;
    size_t size;
    size_t position;
} atlas_reader;

typedef struct atlas_writer {
    uint8_t *data;
    size_t size;
    size_t capacity;
} atlas_writer;

typedef struct atlas_text_builder {
    char *data;
    size_t size;
    size_t capacity;
} atlas_text_builder;

typedef struct atlas_header {
    uint16_t version;
    uint16_t flags;
    uint32_t section_count;
    uint64_t directory_offset;
    uint64_t declared_size;
    uint32_t checksum;
} atlas_header;

typedef struct atlas_section {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t size;
    uint64_t aux;
} atlas_section;

typedef struct atlas_string {
    char *bytes;
    size_t length;
    uint64_t hash;
} atlas_string;

typedef struct atlas_node {
    uint32_t id;
    uint32_t name_index;
    uint32_t first_attribute;
    uint32_t attribute_count;
    size_t outgoing_start;
    size_t outgoing_count;
    size_t incoming_start;
    size_t incoming_count;
} atlas_node;

typedef struct atlas_edge {
    uint32_t source_id;
    uint32_t target_id;
    uint32_t label_index;
    size_t source_index;
    size_t target_index;
} atlas_edge;

typedef struct atlas_attribute {
    uint32_t node_id;
    uint32_t key_index;
    uint32_t value_index;
    size_t node_index;
} atlas_attribute;

typedef struct atlas_metadata {
    uint32_t key_index;
    uint32_t value_index;
} atlas_metadata;

typedef struct atlas_id_slot {
    uint32_t key;
    size_t value;
    unsigned char used;
} atlas_id_slot;

typedef struct atlas_name_slot {
    uint64_t hash;
    size_t string_index;
    size_t node_index;
    unsigned char used;
} atlas_name_slot;

typedef struct atlas_string_slot {
    uint64_t hash;
    size_t index;
    unsigned char used;
} atlas_string_slot;

typedef struct atlas_diagnostic_entry {
    atlas_diagnostic_severity severity;
    char *code;
    char *message;
    uint32_t related_node_id;
    size_t related_edge_index;
} atlas_diagnostic_entry;

typedef struct atlas_builder_node {
    uint32_t id;
    char *name;
} atlas_builder_node;

typedef struct atlas_builder_edge {
    uint32_t source_id;
    uint32_t target_id;
    char *label;
} atlas_builder_edge;

typedef struct atlas_builder_attribute {
    uint32_t node_id;
    char *key;
    char *value;
} atlas_builder_attribute;

typedef struct atlas_builder_metadata {
    char *key;
    char *value;
} atlas_builder_metadata;

struct atlas_context {
    char error[512];
    size_t allocation_limit;
    size_t allocated;
};

struct atlas_document {
    atlas_context *owner_context;
    atlas_parse_options options;
    atlas_header header;
    atlas_section *sections;
    size_t section_count;
    atlas_string *strings;
    size_t string_count;
    atlas_node *nodes;
    size_t node_count;
    atlas_edge *edges;
    size_t edge_count;
    atlas_attribute *attributes;
    size_t attribute_count;
    atlas_metadata *metadata;
    size_t metadata_count;
    atlas_id_slot *id_index;
    size_t id_index_capacity;
    atlas_name_slot *name_index;
    size_t name_index_capacity;
    size_t *edge_order;
    size_t *incoming_order;
    unsigned normalized;
};

struct atlas_plan {
    const atlas_document *document;
    size_t *node_order;
    size_t count;
};

struct atlas_path {
    const atlas_document *document;
    size_t *node_indices;
    size_t *edge_indices;
    size_t node_count;
};

struct atlas_traversal {
    const atlas_document *document;
    size_t *node_indices;
    size_t count;
};

struct atlas_components {
    const atlas_document *document;
    size_t *component_by_node;
    size_t *component_sizes;
    size_t count;
};

struct atlas_layers {
    const atlas_document *document;
    size_t *node_indices;
    size_t *offsets;
    size_t count;
};

struct atlas_selection {
    const atlas_document *document;
    size_t *node_indices;
    size_t count;
};

struct atlas_builder {
    atlas_builder_node *nodes;
    size_t node_count;
    size_t node_capacity;
    atlas_builder_edge *edges;
    size_t edge_count;
    size_t edge_capacity;
    atlas_builder_attribute *attributes;
    size_t attribute_count;
    size_t attribute_capacity;
    atlas_builder_metadata *metadata;
    size_t metadata_count;
    size_t metadata_capacity;
};

struct atlas_diagnostics {
    atlas_diagnostic_entry *entries;
    size_t count;
    size_t capacity;
    size_t error_count;
    size_t warning_count;
};

struct atlas_diff {
    atlas_diff_summary summary;
};

struct atlas_strong_components {
    const atlas_document *document;
    size_t *component_by_node;
    size_t *component_sizes;
    unsigned char *component_cyclic;
    size_t count;
    size_t cyclic_count;
};

struct atlas_distance_map {
    const atlas_document *document;
    size_t *distances;
    size_t reachable_count;
    size_t maximum;
};

struct atlas_rank {
    const atlas_document *document;
    double *scores;
    size_t *order;
};

struct atlas_edge_selection {
    const atlas_document *document;
    size_t *edge_indices;
    size_t count;
};

typedef struct atlas_catalog_entry {
    char *text;
    size_t length;
    size_t usage_count;
} atlas_catalog_entry;

struct atlas_catalog {
    atlas_catalog_entry *entries;
    size_t count;
    size_t capacity;
};

void atlas_set_error(atlas_context *context, const char *format, ...);
void *atlas_alloc(atlas_context *context, size_t size);
void *atlas_calloc(atlas_context *context, size_t count, size_t size);
char *atlas_strdup_n(atlas_context *context, const char *data, size_t size);
char *atlas_strdup_plain(const char *text);

void atlas_reader_init(atlas_reader *reader, const uint8_t *data, size_t size);
bool atlas_reader_seek(atlas_reader *reader, size_t position);
bool atlas_reader_skip(atlas_reader *reader, size_t amount);
bool atlas_reader_u16(atlas_reader *reader, uint16_t *value);
bool atlas_reader_u32(atlas_reader *reader, uint32_t *value);
bool atlas_reader_u64(atlas_reader *reader, uint64_t *value);
bool atlas_reader_bytes(atlas_reader *reader, const uint8_t **bytes, size_t amount);
size_t atlas_reader_remaining(const atlas_reader *reader);

void atlas_writer_init(atlas_writer *writer);
void atlas_writer_dispose(atlas_writer *writer);
atlas_status atlas_writer_reserve(atlas_context *context, atlas_writer *writer, size_t additional);
atlas_status atlas_writer_u8(atlas_context *context, atlas_writer *writer, uint8_t value);
atlas_status atlas_writer_u16(atlas_context *context, atlas_writer *writer, uint16_t value);
atlas_status atlas_writer_u32(atlas_context *context, atlas_writer *writer, uint32_t value);
atlas_status atlas_writer_u64(atlas_context *context, atlas_writer *writer, uint64_t value);
atlas_status atlas_writer_bytes(atlas_context *context, atlas_writer *writer, const void *data, size_t size);
atlas_status atlas_writer_patch_u32(atlas_writer *writer, size_t offset, uint32_t value);
atlas_status atlas_writer_patch_u64(atlas_writer *writer, size_t offset, uint64_t value);
uint8_t *atlas_writer_take(atlas_writer *writer, size_t *out_size);

void atlas_text_builder_init(atlas_text_builder *builder);
void atlas_text_builder_dispose(atlas_text_builder *builder);
atlas_status atlas_text_builder_reserve(atlas_context *context, atlas_text_builder *builder, size_t additional);
atlas_status atlas_text_builder_append(atlas_context *context, atlas_text_builder *builder, const char *text);
atlas_status atlas_text_builder_append_n(atlas_context *context, atlas_text_builder *builder, const char *text, size_t length);
atlas_status atlas_text_builder_append_char(atlas_context *context, atlas_text_builder *builder, char value);
atlas_status atlas_text_builder_printf(atlas_context *context, atlas_text_builder *builder, const char *format, ...);
atlas_status atlas_text_builder_json_string(atlas_context *context, atlas_text_builder *builder, const char *text, size_t length);
atlas_status atlas_text_builder_csv_field(atlas_context *context, atlas_text_builder *builder, const char *text, size_t length);
char *atlas_text_builder_take(atlas_text_builder *builder, size_t *out_size);

uint32_t atlas_checksum32(const uint8_t *data, size_t size);
uint64_t atlas_hash_bytes(const void *data, size_t size);
uint64_t atlas_hash_combine(uint64_t left, uint64_t right);

int atlas_utf8_valid(const unsigned char *text, size_t length);
size_t atlas_ascii_normalized_length(const char *text, size_t length);
void atlas_ascii_normalize(char *text);
int atlas_string_equal(const atlas_string *string, const char *text);
int atlas_string_has_prefix(const atlas_string *string, const char *prefix);

atlas_status atlas_parse_header(atlas_context *context, atlas_reader *reader, atlas_header *header);
atlas_status atlas_parse_directory(atlas_context *context,
                                   const uint8_t *data,
                                   size_t size,
                                   const atlas_header *header,
                                   atlas_section **out_sections,
                                   size_t *out_count,
                                   int allow_unknown);
const atlas_section *atlas_find_section(const atlas_document *document, uint32_t type);
atlas_status atlas_parse_strings(atlas_context *context,
                                 const uint8_t *data,
                                 size_t size,
                                 const atlas_section *section,
                                 atlas_document *document);
atlas_status atlas_parse_nodes(atlas_context *context,
                               const uint8_t *data,
                               size_t size,
                               const atlas_section *section,
                               atlas_document *document);
atlas_status atlas_parse_edges(atlas_context *context,
                               const uint8_t *data,
                               size_t size,
                               const atlas_section *section,
                               atlas_document *document);
atlas_status atlas_parse_attributes(atlas_context *context,
                                    const uint8_t *data,
                                    size_t size,
                                    const atlas_section *section,
                                    atlas_document *document);
atlas_status atlas_parse_metadata(atlas_context *context,
                                  const uint8_t *data,
                                  size_t size,
                                  const atlas_section *section,
                                  atlas_document *document);
atlas_status atlas_build_registry(atlas_context *context, atlas_document *document);
atlas_status atlas_resolve_references(atlas_context *context, atlas_document *document);
atlas_status atlas_build_graph(atlas_context *context, atlas_document *document);
atlas_status atlas_normalize(atlas_context *context, atlas_document *document);
atlas_status atlas_validate_document(atlas_context *context, const atlas_document *document);
atlas_status atlas_recover_optional(atlas_context *context, atlas_document *document);
atlas_status atlas_build_name_index(atlas_context *context, atlas_document *document);
size_t atlas_id_lookup(const atlas_document *document, uint32_t id, int *found);
size_t atlas_node_index_by_id(const atlas_document *document, uint32_t id);
size_t atlas_edge_index_from_view(const atlas_document *document, const atlas_edge *edge);

atlas_status atlas_document_from_builder(atlas_context *context,
                                         const atlas_builder *builder,
                                         atlas_document **out_document);
atlas_status atlas_builder_from_document(const atlas_document *document,
                                         atlas_builder **out_builder);
atlas_status atlas_builder_validate(const atlas_builder *builder, atlas_context *context);

atlas_status atlas_diagnostics_add(atlas_diagnostics *diagnostics,
                                   atlas_diagnostic_severity severity,
                                   const char *code,
                                   uint32_t node_id,
                                   size_t edge_index,
                                   const char *format,
                                   ...);

#endif
