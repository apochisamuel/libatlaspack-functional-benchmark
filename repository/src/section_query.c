#include "internal.h"

size_t atlas_document_section_count(const atlas_document *document) {
    return document == NULL ? 0 : document->section_count;
}

atlas_status atlas_document_section_at(const atlas_document *document,
                                       size_t index,
                                       atlas_section_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= document->section_count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_section *section = &document->sections[index];
    out_view->type = section->type;
    out_view->flags = section->flags;
    out_view->offset = section->offset;
    out_view->size = section->size;
    out_view->auxiliary = section->aux;
    return ATLAS_OK;
}

atlas_status atlas_document_string_at(const atlas_document *document,
                                      size_t index,
                                      const char **out_text,
                                      size_t *out_length) {
    if (document == NULL || out_text == NULL || out_length == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= document->string_count) {
        return ATLAS_ERR_RANGE;
    }
    *out_text = document->strings[index].bytes;
    *out_length = document->strings[index].length;
    return ATLAS_OK;
}
