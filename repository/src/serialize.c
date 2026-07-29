#include "internal.h"

typedef struct atlas_serial_string {
    const char *text;
    size_t length;
} atlas_serial_string;

typedef struct atlas_serial_pool {
    atlas_serial_string *items;
    size_t count;
    size_t capacity;
} atlas_serial_pool;

static void atlas_serial_pool_dispose(atlas_serial_pool *pool) {
    if (pool == NULL) {
        return;
    }
    free(pool->items);
    pool->items = NULL;
    pool->count = 0;
    pool->capacity = 0;
}

static atlas_status atlas_serial_pool_reserve(atlas_serial_pool *pool, size_t additional) {
    if (additional > SIZE_MAX - pool->count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = pool->count + additional;
    if (required <= pool->capacity) {
        return ATLAS_OK;
    }
    size_t capacity = pool->capacity == 0 ? 16 : pool->capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_serial_string *replacement =
        (atlas_serial_string *)realloc(pool->items,
                                       capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    pool->items = replacement;
    pool->capacity = capacity;
    return ATLAS_OK;
}

static atlas_status atlas_serial_pool_intern(atlas_serial_pool *pool,
                                             const char *text,
                                             uint32_t *out_index) {
    if (pool == NULL || text == NULL || out_index == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t length = strlen(text);
    for (size_t index = 0; index < pool->count; ++index) {
        if (pool->items[index].length == length &&
            memcmp(pool->items[index].text, text, length) == 0) {
            if (index > UINT32_MAX) {
                return ATLAS_ERR_RANGE;
            }
            *out_index = (uint32_t)index;
            return ATLAS_OK;
        }
    }
    if (pool->count >= ATLAS_MAX_STRINGS || pool->count > UINT32_MAX) {
        return ATLAS_ERR_RANGE;
    }
    atlas_status status = atlas_serial_pool_reserve(pool, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    pool->items[pool->count].text = text;
    pool->items[pool->count].length = length;
    *out_index = (uint32_t)pool->count;
    ++pool->count;
    return ATLAS_OK;
}

static atlas_status atlas_collect_builder_strings(const atlas_builder *builder,
                                                  atlas_serial_pool *pool) {
    uint32_t ignored = 0;
    for (size_t index = 0; index < builder->node_count; ++index) {
        atlas_status status = atlas_serial_pool_intern(pool,
                                                       builder->nodes[index].name,
                                                       &ignored);
        if (status != ATLAS_OK) {
            return status;
        }
    }
    for (size_t index = 0; index < builder->edge_count; ++index) {
        atlas_status status = atlas_serial_pool_intern(pool,
                                                       builder->edges[index].label,
                                                       &ignored);
        if (status != ATLAS_OK) {
            return status;
        }
    }
    for (size_t index = 0; index < builder->attribute_count; ++index) {
        atlas_status status = atlas_serial_pool_intern(pool,
                                                       builder->attributes[index].key,
                                                       &ignored);
        if (status == ATLAS_OK) {
            status = atlas_serial_pool_intern(pool,
                                              builder->attributes[index].value,
                                              &ignored);
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    for (size_t index = 0; index < builder->metadata_count; ++index) {
        atlas_status status = atlas_serial_pool_intern(pool,
                                                       builder->metadata[index].key,
                                                       &ignored);
        if (status == ATLAS_OK) {
            status = atlas_serial_pool_intern(pool,
                                              builder->metadata[index].value,
                                              &ignored);
        }
        if (status != ATLAS_OK) {
            return status;
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_write_string_section(atlas_context *context,
                                               atlas_writer *writer,
                                               const atlas_serial_pool *pool) {
    atlas_status status = atlas_writer_u32(context, writer, (uint32_t)pool->count);
    for (size_t index = 0; index < pool->count && status == ATLAS_OK; ++index) {
        if (pool->items[index].length > UINT32_MAX) {
            atlas_set_error(context, "string %zu is too large for ATPK", index);
            return ATLAS_ERR_RANGE;
        }
        status = atlas_writer_u32(context,
                                  writer,
                                  (uint32_t)pool->items[index].length);
        if (status == ATLAS_OK) {
            status = atlas_writer_bytes(context,
                                        writer,
                                        pool->items[index].text,
                                        pool->items[index].length);
        }
    }
    return status;
}

static atlas_status atlas_write_node_and_attribute_sections(
    atlas_context *context,
    atlas_writer *node_writer,
    atlas_writer *attribute_writer,
    const atlas_builder *builder,
    atlas_serial_pool *pool) {
    atlas_status status = atlas_writer_u32(context,
                                           node_writer,
                                           (uint32_t)builder->node_count);
    if (status != ATLAS_OK) {
        return status;
    }
    status = atlas_writer_u32(context,
                              attribute_writer,
                              (uint32_t)builder->attribute_count);
    if (status != ATLAS_OK) {
        return status;
    }
    size_t attribute_cursor = 0;
    for (size_t node_index = 0;
         node_index < builder->node_count && status == ATLAS_OK;
         ++node_index) {
        const atlas_builder_node *node = &builder->nodes[node_index];
        uint32_t name_index = 0;
        status = atlas_serial_pool_intern(pool, node->name, &name_index);
        if (status != ATLAS_OK) {
            return status;
        }
        size_t count = 0;
        for (size_t index = 0; index < builder->attribute_count; ++index) {
            if (builder->attributes[index].node_id == node->id) {
                ++count;
            }
        }
        if (attribute_cursor > UINT32_MAX || count > UINT32_MAX) {
            return ATLAS_ERR_RANGE;
        }
        status = atlas_writer_u32(context, node_writer, node->id);
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, node_writer, name_index);
        }
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context,
                                      node_writer,
                                      (uint32_t)attribute_cursor);
        }
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, node_writer, (uint32_t)count);
        }
        for (size_t index = 0;
             index < builder->attribute_count && status == ATLAS_OK;
             ++index) {
            const atlas_builder_attribute *attribute = &builder->attributes[index];
            if (attribute->node_id != node->id) {
                continue;
            }
            uint32_t key_index = 0;
            uint32_t value_index = 0;
            status = atlas_serial_pool_intern(pool, attribute->key, &key_index);
            if (status == ATLAS_OK) {
                status = atlas_serial_pool_intern(pool,
                                                  attribute->value,
                                                  &value_index);
            }
            if (status == ATLAS_OK) {
                status = atlas_writer_u32(context,
                                          attribute_writer,
                                          attribute->node_id);
            }
            if (status == ATLAS_OK) {
                status = atlas_writer_u32(context, attribute_writer, key_index);
            }
            if (status == ATLAS_OK) {
                status = atlas_writer_u32(context, attribute_writer, value_index);
            }
            ++attribute_cursor;
        }
    }
    return status;
}

static atlas_status atlas_write_edge_section(atlas_context *context,
                                             atlas_writer *writer,
                                             const atlas_builder *builder,
                                             atlas_serial_pool *pool) {
    atlas_status status = atlas_writer_u32(context,
                                           writer,
                                           (uint32_t)builder->edge_count);
    for (size_t index = 0; index < builder->edge_count && status == ATLAS_OK; ++index) {
        const atlas_builder_edge *edge = &builder->edges[index];
        uint32_t label_index = 0;
        status = atlas_serial_pool_intern(pool, edge->label, &label_index);
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, writer, edge->source_id);
        }
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, writer, edge->target_id);
        }
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, writer, label_index);
        }
    }
    return status;
}

static atlas_status atlas_write_metadata_section(atlas_context *context,
                                                 atlas_writer *writer,
                                                 const atlas_builder *builder,
                                                 atlas_serial_pool *pool) {
    atlas_status status = atlas_writer_u32(context,
                                           writer,
                                           (uint32_t)builder->metadata_count);
    for (size_t index = 0; index < builder->metadata_count && status == ATLAS_OK; ++index) {
        uint32_t key_index = 0;
        uint32_t value_index = 0;
        status = atlas_serial_pool_intern(pool,
                                          builder->metadata[index].key,
                                          &key_index);
        if (status == ATLAS_OK) {
            status = atlas_serial_pool_intern(pool,
                                              builder->metadata[index].value,
                                              &value_index);
        }
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, writer, key_index);
        }
        if (status == ATLAS_OK) {
            status = atlas_writer_u32(context, writer, value_index);
        }
    }
    return status;
}

static atlas_status atlas_write_directory_entry(atlas_context *context,
                                                atlas_writer *writer,
                                                uint32_t type,
                                                uint64_t offset,
                                                uint64_t size) {
    atlas_status status = atlas_writer_u32(context, writer, type);
    if (status == ATLAS_OK) {
        status = atlas_writer_u32(context, writer, 0);
    }
    if (status == ATLAS_OK) {
        status = atlas_writer_u64(context, writer, offset);
    }
    if (status == ATLAS_OK) {
        status = atlas_writer_u64(context, writer, size);
    }
    if (status == ATLAS_OK) {
        status = atlas_writer_u64(context, writer, 0);
    }
    return status;
}

static atlas_status atlas_assemble_sections(atlas_context *context,
                                            atlas_writer *output,
                                            atlas_writer sections[5]) {
    const uint32_t types[5] = {
        ATLAS_SECTION_STRINGS,
        ATLAS_SECTION_NODES,
        ATLAS_SECTION_EDGES,
        ATLAS_SECTION_ATTRIBUTES,
        ATLAS_SECTION_METADATA
    };
    const size_t section_count = 5;
    const size_t directory_offset = ATLAS_HEADER_SIZE;
    size_t payload_offset = directory_offset +
                            section_count * ATLAS_DIRECTORY_ENTRY_SIZE;
    atlas_status status = atlas_writer_u32(context, output, ATLAS_MAGIC);
    if (status == ATLAS_OK) status = atlas_writer_u16(context, output, ATLAS_VERSION);
    if (status == ATLAS_OK) status = atlas_writer_u16(context, output, 0);
    if (status == ATLAS_OK) status = atlas_writer_u32(context, output, (uint32_t)section_count);
    if (status == ATLAS_OK) status = atlas_writer_u64(context, output, directory_offset);
    size_t declared_size_offset = output->size;
    if (status == ATLAS_OK) status = atlas_writer_u64(context, output, 0);
    size_t checksum_offset = output->size;
    if (status == ATLAS_OK) status = atlas_writer_u32(context, output, 0);
    for (size_t index = 0; index < section_count && status == ATLAS_OK; ++index) {
        status = atlas_write_directory_entry(context,
                                             output,
                                             types[index],
                                             payload_offset,
                                             sections[index].size);
        if (sections[index].size > SIZE_MAX - payload_offset) {
            return ATLAS_ERR_RANGE;
        }
        payload_offset += sections[index].size;
    }
    for (size_t index = 0; index < section_count && status == ATLAS_OK; ++index) {
        status = atlas_writer_bytes(context,
                                    output,
                                    sections[index].data,
                                    sections[index].size);
    }
    if (status != ATLAS_OK) {
        return status;
    }
    if (output->size > UINT64_MAX) {
        return ATLAS_ERR_RANGE;
    }
    status = atlas_writer_patch_u64(output,
                                    declared_size_offset,
                                    (uint64_t)output->size);
    if (status != ATLAS_OK) {
        return status;
    }
    uint32_t checksum = atlas_checksum32(output->data + ATLAS_HEADER_SIZE,
                                         output->size - ATLAS_HEADER_SIZE);
    return atlas_writer_patch_u32(output, checksum_offset, checksum);
}

atlas_status atlas_builder_serialize(atlas_context *context,
                                     const atlas_builder *builder,
                                     uint8_t **out_data,
                                     size_t *out_size) {
    if (context == NULL || builder == NULL || out_data == NULL || out_size == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_data = NULL;
    *out_size = 0;
    atlas_context_reset(context);
    atlas_status status = atlas_builder_validate(builder, context);
    if (status != ATLAS_OK) {
        return status;
    }
    atlas_serial_pool pool = {0};
    status = atlas_collect_builder_strings(builder, &pool);
    atlas_writer sections[5];
    for (size_t index = 0; index < 5; ++index) {
        atlas_writer_init(&sections[index]);
    }
    if (status == ATLAS_OK) {
        status = atlas_write_node_and_attribute_sections(context,
                                                          &sections[1],
                                                          &sections[3],
                                                          builder,
                                                          &pool);
    }
    if (status == ATLAS_OK) {
        status = atlas_write_edge_section(context, &sections[2], builder, &pool);
    }
    if (status == ATLAS_OK) {
        status = atlas_write_metadata_section(context, &sections[4], builder, &pool);
    }
    if (status == ATLAS_OK) {
        status = atlas_write_string_section(context, &sections[0], &pool);
    }
    atlas_writer output;
    atlas_writer_init(&output);
    if (status == ATLAS_OK) {
        status = atlas_assemble_sections(context, &output, sections);
    }
    if (status == ATLAS_OK) {
        *out_data = atlas_writer_take(&output, out_size);
        if (*out_data == NULL) {
            status = ATLAS_ERR_NOMEM;
        }
    }
    atlas_writer_dispose(&output);
    for (size_t index = 0; index < 5; ++index) {
        atlas_writer_dispose(&sections[index]);
    }
    atlas_serial_pool_dispose(&pool);
    return status;
}

atlas_status atlas_builder_parse(atlas_context *context,
                                 const atlas_builder *builder,
                                 const atlas_parse_options *options,
                                 atlas_document **out_document) {
    if (context == NULL || builder == NULL || out_document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    uint8_t *data = NULL;
    size_t size = 0;
    atlas_status status = atlas_builder_serialize(context, builder, &data, &size);
    if (status == ATLAS_OK) {
        status = atlas_parse(context, data, size, options, out_document);
    }
    free(data);
    return status;
}

atlas_status atlas_document_serialize(atlas_context *context,
                                      const atlas_document *document,
                                      uint8_t **out_data,
                                      size_t *out_size) {
    if (context == NULL || document == NULL || out_data == NULL || out_size == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_builder *builder = NULL;
    atlas_status status = atlas_builder_from_document(document, &builder);
    if (status == ATLAS_OK) {
        status = atlas_builder_serialize(context, builder, out_data, out_size);
    }
    atlas_builder_destroy(builder);
    return status;
}
