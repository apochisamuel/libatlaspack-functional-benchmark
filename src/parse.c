#include "internal.h"

extern atlas_status atlas_build_name_index(atlas_context *context, atlas_document *document);

atlas_status atlas_parse(atlas_context *context,
                         const uint8_t *data,
                         size_t size,
                         const atlas_parse_options *options,
                         atlas_document **out_document) {
    if (context == NULL || data == NULL || out_document == NULL) return ATLAS_ERR_ARGUMENT;
    *out_document = NULL;
    atlas_context_reset(context);
    atlas_parse_options effective = options == NULL ? atlas_parse_options_default() : *options;
    if (effective.allocation_limit == 0) effective.allocation_limit = 64u * 1024u * 1024u;
    context->allocation_limit = effective.allocation_limit;
    if (size < ATLAS_HEADER_SIZE) {
        atlas_set_error(context, "input shorter than ATPK header");
        return ATLAS_ERR_TRUNCATED;
    }
    atlas_document *document = atlas_calloc(context, 1, sizeof(*document));
    if (document == NULL) return ATLAS_ERR_NOMEM;
    document->owner_context = context;
    document->options = effective;
    atlas_reader reader;
    atlas_reader_init(&reader, data, size);
    atlas_status status = atlas_parse_header(context, &reader, &document->header);
    if (status != ATLAS_OK) goto fail;
    if (document->header.declared_size != size) {
        atlas_set_error(context, "declared size does not match input size");
        status = ATLAS_ERR_FORMAT;
        goto fail;
    }
    if (effective.validate_checksum) {
        uint32_t actual = atlas_checksum32(data + ATLAS_HEADER_SIZE, size - ATLAS_HEADER_SIZE);
        if (actual != document->header.checksum) {
            atlas_set_error(context, "checksum mismatch");
            status = ATLAS_ERR_FORMAT;
            goto fail;
        }
    }
    status = atlas_parse_directory(context, data, size, &document->header,
                                   &document->sections, &document->section_count,
                                   effective.allow_unknown_sections);
    if (status != ATLAS_OK) goto fail;
    status = atlas_parse_strings(context, data, size,
                                 atlas_find_section(document, ATLAS_SECTION_STRINGS), document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_parse_nodes(context, data, size,
                               atlas_find_section(document, ATLAS_SECTION_NODES), document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_parse_edges(context, data, size,
                               atlas_find_section(document, ATLAS_SECTION_EDGES), document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_parse_attributes(context, data, size,
                                    atlas_find_section(document, ATLAS_SECTION_ATTRIBUTES), document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_parse_metadata(context, data, size,
                                  atlas_find_section(document, ATLAS_SECTION_METADATA), document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_build_registry(context, document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_resolve_references(context, document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_recover_optional(context, document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_normalize(context, document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_validate_document(context, document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_build_graph(context, document);
    if (status != ATLAS_OK) goto fail;
    status = atlas_build_name_index(context, document);
    if (status != ATLAS_OK) goto fail;
    *out_document = document;
    return ATLAS_OK;
fail:
    atlas_document_destroy(document);
    return status;
}
