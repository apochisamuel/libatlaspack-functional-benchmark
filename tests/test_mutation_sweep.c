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

static uint32_t next_random(uint32_t *state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static void make_fixture(atlas_context *context,
                         uint8_t **out_data,
                         size_t *out_size) {
    atlas_builder *builder = atlas_builder_create();
    CHECK(builder != NULL);
    for (uint32_t id = 1; id <= 12; ++id) {
        char name[32];
        snprintf(name, sizeof(name), "component-%u", id);
        CHECK(atlas_builder_add_node(builder, id, name) == ATLAS_OK);
        CHECK(atlas_builder_add_attribute(builder, id, "class", id % 2 ? "worker" : "service") == ATLAS_OK);
    }
    for (uint32_t id = 1; id < 12; ++id) {
        CHECK(atlas_builder_add_edge(builder, id, id + 1, "depends") == ATLAS_OK);
    }
    CHECK(atlas_builder_add_edge(builder, 1, 6, "observes") == ATLAS_OK);
    CHECK(atlas_builder_add_edge(builder, 3, 10, "publishes") == ATLAS_OK);
    CHECK(atlas_builder_add_metadata(builder, "suite", "mutation-sweep") == ATLAS_OK);
    CHECK(atlas_builder_add_metadata(builder, "revision", "r3") == ATLAS_OK);
    CHECK(atlas_builder_serialize(context, builder, out_data, out_size) == ATLAS_OK);
    atlas_builder_destroy(builder);
}

static void parse_and_discard(atlas_context *context,
                              const uint8_t *data,
                              size_t size,
                              atlas_parse_options *options,
                              size_t *accepted,
                              size_t *rejected) {
    atlas_document *document = NULL;
    atlas_status status = atlas_parse(context, data, size, options, &document);
    CHECK(status >= ATLAS_OK && status <= ATLAS_ERR_UNSUPPORTED);
    if (status == ATLAS_OK) {
        ++*accepted;
        CHECK(document != NULL);
        atlas_statistics statistics;
        CHECK(atlas_document_statistics(document, &statistics) == ATLAS_OK);
        uint64_t fingerprint = atlas_document_fingerprint(document);
        (void)fingerprint;
        char *manifest = NULL;
        size_t manifest_size = 0;
        CHECK(atlas_document_manifest(context,
                                      document,
                                      &manifest,
                                      &manifest_size) == ATLAS_OK);
        CHECK(manifest != NULL && manifest_size > 0);
        atlas_free(manifest);
    } else {
        ++*rejected;
        CHECK(document == NULL);
    }
    atlas_document_destroy(document);
}

int main(void) {
    atlas_context *context = atlas_context_create();
    CHECK(context != NULL);
    uint8_t *fixture = NULL;
    size_t fixture_size = 0;
    make_fixture(context, &fixture, &fixture_size);
    CHECK(fixture != NULL && fixture_size > 64);

    atlas_parse_options options = atlas_parse_options_default();
    options.validate_checksum = 0;
    options.allow_unknown_sections = 1;
    size_t accepted = 0;
    size_t rejected = 0;

    for (size_t length = 0; length < fixture_size; ++length) {
        parse_and_discard(context,
                          fixture,
                          length,
                          &options,
                          &accepted,
                          &rejected);
    }

    uint8_t *working = (uint8_t *)malloc(fixture_size);
    CHECK(working != NULL);
    for (size_t offset = 0; offset < fixture_size; ++offset) {
        memcpy(working, fixture, fixture_size);
        working[offset] ^= (uint8_t)(UINT8_C(1) << (offset % 8));
        parse_and_discard(context,
                          working,
                          fixture_size,
                          &options,
                          &accepted,
                          &rejected);
    }

    uint32_t random_state = UINT32_C(0x6d2b79f5);
    for (size_t trial = 0; trial < 10000; ++trial) {
        memcpy(working, fixture, fixture_size);
        size_t changes = 1 + next_random(&random_state) % 6;
        for (size_t change = 0; change < changes; ++change) {
            size_t offset = next_random(&random_state) % fixture_size;
            uint8_t mask = (uint8_t)(1u << (next_random(&random_state) % 8));
            working[offset] ^= mask;
        }
        options.allocation_limit = 64u + next_random(&random_state) % (2u * 1024u * 1024u);
        size_t effective_size = fixture_size;
        if ((trial % 7) == 0) {
            effective_size = next_random(&random_state) % fixture_size;
        }
        parse_and_discard(context,
                          working,
                          effective_size,
                          &options,
                          &accepted,
                          &rejected);
    }

    free(working);
    atlas_free(fixture);
    atlas_context_destroy(context);
    printf("mutation sweep passed: accepted=%zu rejected=%zu\n", accepted, rejected);
    CHECK(accepted > 0);
    CHECK(rejected > 0);
    return 0;
}
