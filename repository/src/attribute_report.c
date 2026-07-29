#include "internal.h"

typedef struct attribute_key_summary {
    size_t string_index;
    size_t use_count;
    size_t distinct_value_count;
} attribute_key_summary;

static int attribute_value_seen(const atlas_document *document,
                                size_t key_index,
                                size_t value_index,
                                size_t before) {
    for (size_t index = 0; index < before; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        if (attribute->key_index == key_index &&
            attribute->value_index == value_index) {
            return 1;
        }
    }
    return 0;
}

static atlas_status attribute_collect(atlas_context *context,
                                      const atlas_document *document,
                                      attribute_key_summary **out_summaries,
                                      size_t *out_count) {
    *out_summaries = NULL;
    *out_count = 0;
    if (document->attribute_count == 0) {
        return ATLAS_OK;
    }
    attribute_key_summary *summaries = atlas_calloc(context,
                                                    document->attribute_count,
                                                    sizeof(*summaries));
    if (summaries == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    size_t count = 0;
    for (size_t attribute_index = 0;
         attribute_index < document->attribute_count;
         ++attribute_index) {
        const atlas_attribute *attribute = &document->attributes[attribute_index];
        size_t summary_index = ATLAS_NO_INDEX;
        for (size_t index = 0; index < count; ++index) {
            if (summaries[index].string_index == attribute->key_index) {
                summary_index = index;
                break;
            }
        }
        if (summary_index == ATLAS_NO_INDEX) {
            summary_index = count++;
            summaries[summary_index].string_index = attribute->key_index;
        }
        ++summaries[summary_index].use_count;
        if (!attribute_value_seen(document,
                                  attribute->key_index,
                                  attribute->value_index,
                                  attribute_index)) {
            ++summaries[summary_index].distinct_value_count;
        }
    }
    *out_summaries = summaries;
    *out_count = count;
    return ATLAS_OK;
}

static atlas_status attribute_write_summary(atlas_context *context,
                                            atlas_text_builder *builder,
                                            const atlas_document *document,
                                            const attribute_key_summary *summary) {
    const atlas_string *key = &document->strings[summary->string_index];
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "key=");
    if (status == ATLAS_OK) {
        status = atlas_text_builder_json_string(context,
                                                builder,
                                                key->bytes,
                                                key->length);
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_printf(context,
                                           builder,
                                           " uses=%zu distinct_values=%zu values=",
                                           summary->use_count,
                                           summary->distinct_value_count);
    }
    size_t ordinal = 0;
    for (size_t attribute_index = 0;
         attribute_index < document->attribute_count && status == ATLAS_OK;
         ++attribute_index) {
        const atlas_attribute *attribute = &document->attributes[attribute_index];
        if (attribute->key_index != summary->string_index ||
            attribute_value_seen(document,
                                 attribute->key_index,
                                 attribute->value_index,
                                 attribute_index)) {
            continue;
        }
        const atlas_string *value = &document->strings[attribute->value_index];
        if (ordinal != 0) {
            status = atlas_text_builder_append_char(context, builder, ',');
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    value->bytes,
                                                    value->length);
        }
        ++ordinal;
    }
    if (status == ATLAS_OK) {
        status = atlas_text_builder_append_char(context, builder, '\n');
    }
    return status;
}

static atlas_status attribute_write_nodes(atlas_context *context,
                                          atlas_text_builder *builder,
                                          const atlas_document *document) {
    atlas_status status = atlas_text_builder_append(context,
                                                    builder,
                                                    "node_attribute_counts:\n");
    for (size_t node_index = 0;
         node_index < document->node_count && status == ATLAS_OK;
         ++node_index) {
        const atlas_node *node = &document->nodes[node_index];
        const atlas_string *name = &document->strings[node->name_index];
        status = atlas_text_builder_printf(context,
                                           builder,
                                           "  %u:",
                                           node->id);
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_n(context,
                                                 builder,
                                                 name->bytes,
                                                 name->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_printf(context,
                                               builder,
                                               "=%u\n",
                                               node->attribute_count);
        }
    }
    return status;
}

static atlas_status attribute_write_metadata(atlas_context *context,
                                             atlas_text_builder *builder,
                                             const atlas_document *document) {
    atlas_status status = atlas_text_builder_printf(context,
                                                    builder,
                                                    "metadata_entries=%zu\n",
                                                    document->metadata_count);
    for (size_t index = 0;
         index < document->metadata_count && status == ATLAS_OK;
         ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        const atlas_string *key = &document->strings[metadata->key_index];
        const atlas_string *value = &document->strings[metadata->value_index];
        status = atlas_text_builder_append(context, builder, "  key=");
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    key->bytes,
                                                    key->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append(context, builder, " value=");
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_json_string(context,
                                                    builder,
                                                    value->bytes,
                                                    value->length);
        }
        if (status == ATLAS_OK) {
            status = atlas_text_builder_append_char(context, builder, '\n');
        }
    }
    return status;
}

atlas_status atlas_document_attribute_report(atlas_context *context,
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
    attribute_key_summary *summaries = NULL;
    size_t summary_count = 0;
    atlas_status status = attribute_collect(context,
                                            document,
                                            &summaries,
                                            &summary_count);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_text_builder builder;
    atlas_text_builder_init(&builder);
    status = atlas_text_builder_printf(context,
                                       &builder,
                                       "ATPK ATTRIBUTE REPORT\nattributes=%zu\ndistinct_keys=%zu\n",
                                       document->attribute_count,
                                       summary_count);
    for (size_t index = 0; index < summary_count && status == ATLAS_OK; ++index) {
        status = attribute_write_summary(context,
                                         &builder,
                                         document,
                                         &summaries[index]);
    }
    if (status == ATLAS_OK) {
        status = attribute_write_nodes(context,
                                       &builder,
                                       document);
    }
    if (status == ATLAS_OK) {
        status = attribute_write_metadata(context,
                                          &builder,
                                          document);
    }
    free(summaries);
    if (status == ATLAS_OK) {
        *out_text = atlas_text_builder_take(&builder, out_length);
        if (*out_text == NULL) {
            status = ATLAS_ERR_NOMEM;
        }
    }
    atlas_text_builder_dispose(&builder);
    return status;
}
