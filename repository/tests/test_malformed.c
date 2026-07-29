#include "atlas.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr, "check failed at %s:%d: %s\n",                    \
                    __FILE__, __LINE__, #condition);                             \
            abort();                                                             \
        }                                                                        \
    } while (0)

#define HEADER_SIZE 32u
#define DIRECTORY_ENTRY_SIZE 32u
#define SECTION_COUNT 5u

static uint32_t read_u32(const uint8_t *data, size_t offset) {
    return (uint32_t)data[offset] |
           ((uint32_t)data[offset + 1] << 8) |
           ((uint32_t)data[offset + 2] << 16) |
           ((uint32_t)data[offset + 3] << 24);
}

static uint64_t read_u64(const uint8_t *data, size_t offset) {
    uint64_t value = 0;
    for (unsigned shift = 0; shift < 64; shift += 8) {
        value |= (uint64_t)data[offset + shift / 8] << shift;
    }
    return value;
}

static void write_u16(uint8_t *data, size_t offset, uint16_t value) {
    data[offset] = (uint8_t)value;
    data[offset + 1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *data, size_t offset, uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        data[offset + shift / 8] = (uint8_t)(value >> shift);
    }
}

static void write_u64(uint8_t *data, size_t offset, uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8) {
        data[offset + shift / 8] = (uint8_t)(value >> shift);
    }
}

static size_t section_offset(const uint8_t *data, size_t directory_index) {
    size_t entry = HEADER_SIZE + directory_index * DIRECTORY_ENTRY_SIZE;
    return (size_t)read_u64(data, entry + 8);
}

static uint8_t *copy_bytes(const uint8_t *data, size_t size) {
    uint8_t *copy = (uint8_t *)malloc(size == 0 ? 1 : size);
    CHECK(copy != NULL);
    if (size != 0) {
        memcpy(copy, data, size);
    }
    return copy;
}

static void expect_parse(atlas_context *context,
                         const uint8_t *data,
                         size_t size,
                         atlas_parse_options options,
                         atlas_status expected,
                         const char *label) {
    atlas_document *document = NULL;
    atlas_status actual = atlas_parse(context, data, size, &options, &document);
    if (actual != expected) {
        fprintf(stderr,
                "%s: expected %s, got %s (%s)\n",
                label,
                atlas_status_name(expected),
                atlas_status_name(actual),
                atlas_context_error(context));
        abort();
    }
    atlas_document_destroy(document);
}

static void make_fixture(atlas_context *context,
                         uint8_t **out_data,
                         size_t *out_size) {
    atlas_builder *builder = atlas_builder_create();
    CHECK(builder != NULL);
    CHECK(atlas_builder_add_node(builder, 10, "Frontend") == ATLAS_OK);
    CHECK(atlas_builder_add_node(builder, 20, "Backend") == ATLAS_OK);
    CHECK(atlas_builder_add_edge(builder, 10, 20, "calls") == ATLAS_OK);
    CHECK(atlas_builder_add_attribute(builder, 10, "tier", "edge") == ATLAS_OK);
    CHECK(atlas_builder_add_attribute(builder, 20, "tier", "service") == ATLAS_OK);
    CHECK(atlas_builder_add_metadata(builder, "environment", "test") == ATLAS_OK);
    CHECK(atlas_builder_add_metadata(builder, "owner", "quality") == ATLAS_OK);
    atlas_status status = atlas_builder_serialize(context, builder, out_data, out_size);
    if (status != ATLAS_OK) {
        fprintf(stderr, "fixture serialization failed: %s\n", atlas_context_error(context));
        abort();
    }
    atlas_builder_destroy(builder);
    CHECK(*out_data != NULL);
    CHECK(*out_size > HEADER_SIZE + SECTION_COUNT * DIRECTORY_ENTRY_SIZE);
}

static void test_argument_contracts(atlas_context *context,
                                    const uint8_t *data,
                                    size_t size) {
    atlas_document *document = NULL;
    atlas_parse_options options = atlas_parse_options_default();
    CHECK(atlas_parse(NULL, data, size, &options, &document) == ATLAS_ERR_ARGUMENT);
    CHECK(atlas_parse(context, NULL, size, &options, &document) == ATLAS_ERR_ARGUMENT);
    CHECK(atlas_parse(context, data, size, &options, NULL) == ATLAS_ERR_ARGUMENT);
    CHECK(atlas_parse(context, data, size, NULL, &document) == ATLAS_OK);
    CHECK(document != NULL);
    atlas_document_destroy(document);
}

static void test_header_failures(atlas_context *context,
                                 const uint8_t *data,
                                 size_t size) {
    atlas_parse_options unchecked = atlas_parse_options_default();
    unchecked.validate_checksum = 0;

    for (size_t truncated = 0; truncated < HEADER_SIZE; ++truncated) {
        expect_parse(context,
                     data,
                     truncated,
                     unchecked,
                     ATLAS_ERR_TRUNCATED,
                     "truncated header");
    }

    uint8_t *copy = copy_bytes(data, size);
    write_u32(copy, 0, UINT32_C(0x12345678));
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_MAGIC, "bad magic");
    free(copy);

    copy = copy_bytes(data, size);
    write_u16(copy, 4, UINT16_C(99));
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_VERSION, "bad version");
    free(copy);

    copy = copy_bytes(data, size);
    write_u32(copy, 8, 0);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_RANGE, "zero sections");
    free(copy);

    copy = copy_bytes(data, size);
    write_u32(copy, 8, 4097);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_RANGE, "too many sections");
    free(copy);

    copy = copy_bytes(data, size);
    write_u64(copy, 12, (uint64_t)size + 1);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_RANGE, "directory outside file");
    free(copy);

    copy = copy_bytes(data, size);
    write_u64(copy, 20, (uint64_t)size - 1);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_FORMAT, "declared size mismatch");
    free(copy);

    copy = copy_bytes(data, size);
    copy[size - 1] ^= UINT8_C(0x01);
    expect_parse(context,
                 copy,
                 size,
                 atlas_parse_options_default(),
                 ATLAS_ERR_FORMAT,
                 "checksum mismatch");
    free(copy);
}

static void test_directory_failures(atlas_context *context,
                                    const uint8_t *data,
                                    size_t size) {
    atlas_parse_options unchecked = atlas_parse_options_default();
    unchecked.validate_checksum = 0;

    uint8_t *copy = copy_bytes(data, size);
    write_u64(copy, 12, (uint64_t)size - 16);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_TRUNCATED, "truncated directory");
    free(copy);

    copy = copy_bytes(data, size);
    write_u32(copy, HEADER_SIZE, 9001);
    unchecked.allow_unknown_sections = 0;
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_FORMAT, "unknown section");
    unchecked.allow_unknown_sections = 1;
    free(copy);

    copy = copy_bytes(data, size);
    write_u64(copy, HEADER_SIZE + 8, (uint64_t)size + 1);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_RANGE, "section offset outside file");
    free(copy);

    copy = copy_bytes(data, size);
    write_u64(copy, HEADER_SIZE + 16, UINT64_MAX);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_RANGE, "section size outside file");
    free(copy);

    copy = copy_bytes(data, size);
    size_t first_offset = section_offset(copy, 0);
    write_u64(copy, HEADER_SIZE + DIRECTORY_ENTRY_SIZE + 8, first_offset);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_FORMAT, "overlapping sections");
    free(copy);
}

static void test_section_failures(atlas_context *context,
                                  const uint8_t *data,
                                  size_t size) {
    const uint32_t excessive_counts[SECTION_COUNT] = {
        1048577u,
        1048577u,
        4194305u,
        4194305u,
        1048577u
    };
    atlas_parse_options unchecked = atlas_parse_options_default();
    unchecked.validate_checksum = 0;

    for (size_t index = 0; index < SECTION_COUNT; ++index) {
        uint8_t *copy = copy_bytes(data, size);
        write_u32(copy, section_offset(copy, index), excessive_counts[index]);
        expect_parse(context, copy, size, unchecked, ATLAS_ERR_RANGE, "excessive section count");
        free(copy);
    }

    uint8_t *copy = copy_bytes(data, size);
    size_t strings = section_offset(copy, 0);
    CHECK(read_u32(copy, strings) > 0);
    CHECK(read_u32(copy, strings + 4) > 0);
    copy[strings + 8] = UINT8_C(0xff);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_FORMAT, "invalid UTF-8");
    free(copy);

    copy = copy_bytes(data, size);
    size_t nodes = section_offset(copy, 1);
    CHECK(read_u32(copy, nodes) >= 2);
    uint32_t first_id = read_u32(copy, nodes + 4);
    write_u32(copy, nodes + 4 + 16, first_id);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_DUPLICATE, "duplicate node id");
    free(copy);

    copy = copy_bytes(data, size);
    size_t edges = section_offset(copy, 2);
    CHECK(read_u32(copy, edges) >= 1);
    write_u32(copy, edges + 8, UINT32_C(0xf0f0f0f0));
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_REFERENCE, "missing edge target");
    free(copy);

    copy = copy_bytes(data, size);
    size_t metadata = section_offset(copy, 4);
    CHECK(read_u32(copy, metadata) >= 2);
    uint32_t first_key = read_u32(copy, metadata + 4);
    write_u32(copy, metadata + 4 + 8, first_key);
    expect_parse(context, copy, size, unchecked, ATLAS_ERR_DUPLICATE, "duplicate metadata key");
    unchecked.require_metadata_keys_unique = 0;
    expect_parse(context, copy, size, unchecked, ATLAS_OK, "duplicate metadata allowed");
    free(copy);
}

static void test_unknown_section_serialization_policy(atlas_context *context,
                                                      const uint8_t *data,
                                                      size_t size) {
    atlas_parse_options options = atlas_parse_options_default();
    options.validate_checksum = 0;
    options.allow_unknown_sections = 1;

    uint8_t *copy = copy_bytes(data, size);
    size_t metadata_entry = HEADER_SIZE + (SECTION_COUNT - 1u) * DIRECTORY_ENTRY_SIZE;
    write_u32(copy, metadata_entry, UINT32_C(9001));

    atlas_document *document = NULL;
    CHECK(atlas_parse(context, copy, size, &options, &document) == ATLAS_OK);
    CHECK(document != NULL);
    CHECK(atlas_document_section_count(document) == SECTION_COUNT);

    int found_unknown = 0;
    for (size_t index = 0; index < atlas_document_section_count(document); ++index) {
        atlas_section_view section;
        CHECK(atlas_document_section_at(document, index, &section) == ATLAS_OK);
        if (section.type == UINT32_C(9001)) {
            found_unknown = 1;
        }
    }
    CHECK(found_unknown);
    CHECK(atlas_document_metadata_count(document) == 0);

    uint8_t *serialized = NULL;
    size_t serialized_size = 0;
    CHECK(atlas_document_serialize(context,
                                   document,
                                   &serialized,
                                   &serialized_size) == ATLAS_OK);
    CHECK(serialized != NULL);

    atlas_document *round_trip = NULL;
    CHECK(atlas_parse(context,
                      serialized,
                      serialized_size,
                      NULL,
                      &round_trip) == ATLAS_OK);
    CHECK(round_trip != NULL);
    CHECK(atlas_document_section_count(round_trip) == SECTION_COUNT);
    for (size_t index = 0; index < atlas_document_section_count(round_trip); ++index) {
        atlas_section_view section;
        CHECK(atlas_document_section_at(round_trip, index, &section) == ATLAS_OK);
        CHECK(section.type >= 1u && section.type <= SECTION_COUNT);
    }
    CHECK(atlas_document_metadata_count(round_trip) == 0);

    atlas_document_destroy(round_trip);
    atlas_free(serialized);
    atlas_document_destroy(document);
    free(copy);
}

static void test_resource_limits(atlas_context *context,
                                 const uint8_t *data,
                                 size_t size) {
    atlas_parse_options options = atlas_parse_options_default();
    options.allocation_limit = 1;
    expect_parse(context, data, size, options, ATLAS_ERR_NOMEM, "allocation limit");

    options.allocation_limit = 64u * 1024u * 1024u;
    expect_parse(context, data, size, options, ATLAS_OK, "normal allocation limit");
}

int main(void) {
    atlas_context *context = atlas_context_create();
    CHECK(context != NULL);
    uint8_t *data = NULL;
    size_t size = 0;
    make_fixture(context, &data, &size);
    test_argument_contracts(context, data, size);
    test_header_failures(context, data, size);
    test_directory_failures(context, data, size);
    test_section_failures(context, data, size);
    test_unknown_section_serialization_policy(context, data, size);
    test_resource_limits(context, data, size);
    atlas_free(data);
    atlas_context_destroy(context);
    puts("malformed-input tests passed");
    return 0;
}
