#include "internal.h"

static size_t integrity_unused_strings(const atlas_document *document) {
    size_t unused = 0;
    for (size_t string_index = 0;
         string_index < document->string_count;
         ++string_index) {
        int used = 0;
        for (size_t node_index = 0;
             node_index < document->node_count && !used;
             ++node_index) {
            used = document->nodes[node_index].name_index == string_index;
        }
        for (size_t edge_index = 0;
             edge_index < document->edge_count && !used;
             ++edge_index) {
            used = document->edges[edge_index].label_index == string_index;
        }
        for (size_t attribute_index = 0;
             attribute_index < document->attribute_count && !used;
             ++attribute_index) {
            const atlas_attribute *attribute = &document->attributes[attribute_index];
            used = attribute->key_index == string_index ||
                   attribute->value_index == string_index;
        }
        for (size_t metadata_index = 0;
             metadata_index < document->metadata_count && !used;
             ++metadata_index) {
            const atlas_metadata *metadata = &document->metadata[metadata_index];
            used = metadata->key_index == string_index ||
                   metadata->value_index == string_index;
        }
        if (!used) {
            ++unused;
        }
    }
    return unused;
}

static size_t integrity_duplicate_metadata(const atlas_document *document) {
    size_t duplicates = 0;
    for (size_t index = 0; index < document->metadata_count; ++index) {
        const atlas_metadata *entry = &document->metadata[index];
        const atlas_string *key = &document->strings[entry->key_index];
        for (size_t previous = 0; previous < index; ++previous) {
            const atlas_metadata *other = &document->metadata[previous];
            const atlas_string *other_key = &document->strings[other->key_index];
            if (key->length == other_key->length &&
                memcmp(key->bytes, other_key->bytes, key->length) == 0) {
                ++duplicates;
                break;
            }
        }
    }
    return duplicates;
}

static size_t integrity_duplicate_attributes(const atlas_document *document) {
    size_t duplicates = 0;
    for (size_t index = 0; index < document->attribute_count; ++index) {
        const atlas_attribute *entry = &document->attributes[index];
        const atlas_string *key = &document->strings[entry->key_index];
        for (size_t previous = 0; previous < index; ++previous) {
            const atlas_attribute *other = &document->attributes[previous];
            const atlas_string *other_key = &document->strings[other->key_index];
            if (entry->node_id == other->node_id &&
                key->length == other_key->length &&
                memcmp(key->bytes, other_key->bytes, key->length) == 0) {
                ++duplicates;
                break;
            }
        }
    }
    return duplicates;
}

static size_t integrity_empty_strings(const atlas_document *document) {
    size_t count = 0;
    for (size_t index = 0; index < document->string_count; ++index) {
        if (document->strings[index].length == 0) {
            ++count;
        }
    }
    return count;
}

static atlas_status integrity_diagnostics(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const atlas_document *document) {
    atlas_diagnostics *diagnostics = NULL;
    atlas_status status = atlas_document_validate(context,
                                                  document,
                                                  &diagnostics);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_text_builder_printf(context,
                                       builder,
                                       "diagnostics=%zu\nerrors=%zu\nwarnings=%zu\n",
                                       atlas_diagnostics_count(diagnostics),
                                       atlas_diagnostics_error_count(diagnostics),
                                       atlas_diagnostics_warning_count(diagnostics));
    for (size_t index = 0;
         index < atlas_diagnostics_count(diagnostics) && status == ATLAS_OK;
         ++index) {
        atlas_diagnostic_view view;
        status = atlas_diagnostics_at(diagnostics, index, &view);
        if (status != ATLAS_OK) {
            break;
        }
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "diagnostic[%zu] severity=%d code=%s node=%u edge=",
                                           index,
                                           (int)view.severity,
                                           view.code,
                                           view.related_node_id);
        if (status == ATLAS_OK) {
            if (view.related_edge_index == ATLAS_DIAGNOSTIC_EDGE_NONE) {
                status = atlas_text_builder_append(context, builder, "none");
            } else {
                status = atlas_text_builder_printf(context,
                                                   builder,
                                                   "%zu",
                                                   view.related_edge_index);
            }
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append(context, builder, " message=");
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    view.message,
                                                    strlen(view.message));
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '\n');
        }
    }
    atlas_diagnostics_destroy(diagnostics);
    return status;
}

static atlas_status integrity_sections(atlas_context *context,
                                       atlas_text_builder *builder,
                                       const atlas_document *document) {
    atlas_status status = atlas_text_builder_printf(context,
                                                    builder,
                                                    "sections=%zu\nsection_ranges:\n",
                                                    document->section_count);
    for (size_t index = 0;
         index < document->section_count && status == ATLAS_OK;
         ++index) {
        const atlas_section *section = &document->sections[index];
        uint64_t end = section->offset + section->size;
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "  [%zu] type=%u offset=%llu size=%llu end=%llu flags=%u aux=%llu\n",
                                           index,
                                           section->type,
                                           (unsigned long long)section->offset,
                                           (unsigned long long)section->size,
                                           (unsigned long long)end,
                                           section->flags,
                                           (unsigned long long)section->aux);
    }
    return status;
}

static atlas_status integrity_indexes(atlas_context *context,
                                      atlas_text_builder *builder,
                                      const atlas_document *document) {
    size_t id_slots = 0;
    for (size_t index = 0; index < document->id_index_capacity; ++index) {
        if (document->id_index[index].used) {
            ++id_slots;
        }
    }
    size_t name_slots = 0;
    for (size_t index = 0; index < document->name_index_capacity; ++index) {
        if (document->name_index[index].used) {
            ++name_slots;
        }
    }
    return atlas_text_builder_printf(context,
                                     builder,
                                     "id_index_capacity=%zu\nid_index_used=%zu\nname_index_capacity=%zu\nname_index_used=%zu\nedge_order_entries=%zu\nincoming_order_entries=%zu\n",
                                     document->id_index_capacity,
                                     id_slots,
                                     document->name_index_capacity,
                                     name_slots,
                                     document->edge_count,
                                     document->edge_count);
}

atlas_status atlas_document_integrity_report(atlas_context *context,
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
    atlas_status status = atlas_text_builder_printf(context,
                                                    &builder,
                                                    "ATPK INTEGRITY REPORT\nversion=%u\nflags=%u\nnormalized=%s\nstrings=%zu\nnodes=%zu\nedges=%zu\nattributes=%zu\nmetadata=%zu\nunused_strings=%zu\nempty_strings=%zu\nduplicate_metadata_keys=%zu\nduplicate_node_attribute_keys=%zu\n",
                                                    document->header.version,
                                                    document->header.flags,
                                                    document->normalized ? "true" : "false",
                                                    document->string_count,
                                                    document->node_count,
                                                    document->edge_count,
                                                    document->attribute_count,
                                                    document->metadata_count,
                                                    integrity_unused_strings(document),
                                                    integrity_empty_strings(document),
                                                    integrity_duplicate_metadata(document),
                                                    integrity_duplicate_attributes(document));
    if (status == ATLAS_OK) {
        status = integrity_sections(context, &builder, document);
    }
    if (status == ATLAS_OK) {
        status = integrity_indexes(context, &builder, document);
    }
    if (status == ATLAS_OK) {
        status = integrity_diagnostics(context, &builder, document);
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
