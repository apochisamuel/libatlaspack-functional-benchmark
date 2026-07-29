#include "internal.h"

static const char *atlas_section_type_name(uint32_t type) {
    switch (type) {
        case ATLAS_SECTION_STRINGS: return "strings";
        case ATLAS_SECTION_NODES: return "nodes";
        case ATLAS_SECTION_EDGES: return "edges";
        case ATLAS_SECTION_ATTRIBUTES: return "attributes";
        case ATLAS_SECTION_METADATA: return "metadata";
        default: return "unknown";
    }
}

static size_t atlas_string_usage_count(const atlas_document *document, size_t string_index) {
    size_t count = 0;
    for (size_t index = 0; index < document->node_count; ++index) {
        if (document->nodes[index].name_index == string_index) {
            ++count;
        }
    }
    for (size_t index = 0; index < document->edge_count; ++index) {
        if (document->edges[index].label_index == string_index) {
            ++count;
        }
    }
    for (size_t index = 0; index < document->attribute_count; ++index) {
        if (document->attributes[index].key_index == string_index) {
            ++count;
        }
        if (document->attributes[index].value_index == string_index) {
            ++count;
        }
    }
    for (size_t index = 0; index < document->metadata_count; ++index) {
        if (document->metadata[index].key_index == string_index) {
            ++count;
        }
        if (document->metadata[index].value_index == string_index) {
            ++count;
        }
    }
    return count;
}

static size_t atlas_duplicate_section_count(const atlas_document *document) {
    size_t duplicates = 0;
    for (size_t index = 0; index < document->section_count; ++index) {
        for (size_t previous = 0; previous < index; ++previous) {
            if (document->sections[index].type == document->sections[previous].type) {
                ++duplicates;
                break;
            }
        }
    }
    return duplicates;
}

static size_t atlas_unused_string_count(const atlas_document *document) {
    size_t count = 0;
    for (size_t index = 0; index < document->string_count; ++index) {
        if (atlas_string_usage_count(document, index) == 0) {
            ++count;
        }
    }
    return count;
}

atlas_status atlas_document_schema_report(atlas_context *context,
                                          const atlas_document *document,
                                          char **out_text,
                                          size_t *out_length) {
    if (context == NULL || document == NULL || out_text == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_text = NULL;
    *out_length = 0;
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    atlas_status status = atlas_text_builder_append(context,
                                                    &builder,
                                                    "ATPK SCHEMA REPORT\n");
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "version=%u\nflags=%u\ndeclared_size=%llu\n",
                                           document->header.version,
                                           document->header.flags,
                                           (unsigned long long)document->header.declared_size);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "sections=%zu\nduplicate_section_types=%zu\n",
                                           document->section_count,
                                           atlas_duplicate_section_count(document));
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context, &builder, "section_directory:\n");
    }
    for (size_t index = 0; index < document->section_count && status == ATLAS_OK; ++index) {
        const atlas_section *section = &document->sections[index];
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "  [%zu] type=%u(%s) flags=%u offset=%llu size=%llu aux=%llu\n",
                                           index,
                                           section->type,
                                           atlas_section_type_name(section->type),
                                           section->flags,
                                           (unsigned long long)section->offset,
                                           (unsigned long long)section->size,
                                           (unsigned long long)section->aux);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "strings=%zu\nunused_strings=%zu\n",
                                           document->string_count,
                                           atlas_unused_string_count(document));
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "nodes=%zu\nedges=%zu\nattributes=%zu\nmetadata=%zu\n",
                                           document->node_count,
                                           document->edge_count,
                                           document->attribute_count,
                                           document->metadata_count);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append(context, &builder, "string_usage:\n");
    }
    for (size_t index = 0; index < document->string_count && status == ATLAS_OK; ++index) {
        const atlas_string *string = &document->strings[index];
        size_t usage = atlas_string_usage_count(document, index);
        status = atlas_text_builder_printf(context,
                                           &builder,
                                           "  [%zu] bytes=%zu hash=%016llx uses=%zu text=",
                                           index,
                                           string->length,
                                           (unsigned long long)string->hash,
                                           usage);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    &builder,
                                                    string->bytes,
                                                    string->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, &builder, '\n');
        }
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
