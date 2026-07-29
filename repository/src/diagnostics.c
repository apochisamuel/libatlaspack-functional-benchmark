#include "internal.h"

static atlas_status atlas_diagnostics_reserve(atlas_diagnostics *diagnostics,
                                              size_t additional) {
    if (diagnostics == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (additional > SIZE_MAX - diagnostics->count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = diagnostics->count + additional;
    if (required <= diagnostics->capacity) {
        return ATLAS_OK;
    }
    size_t capacity = diagnostics->capacity == 0 ? 8 : diagnostics->capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_diagnostic_entry *replacement =
        (atlas_diagnostic_entry *)realloc(diagnostics->entries,
                                          capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    diagnostics->entries = replacement;
    diagnostics->capacity = capacity;
    return ATLAS_OK;
}

atlas_status atlas_diagnostics_add(atlas_diagnostics *diagnostics,
                                   atlas_diagnostic_severity severity,
                                   const char *code,
                                   uint32_t node_id,
                                   size_t edge_index,
                                   const char *format,
                                   ...) {
    if (diagnostics == NULL || code == NULL || format == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_status status = atlas_diagnostics_reserve(diagnostics, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    va_list first;
    va_start(first, format);
    va_list second;
    va_copy(second, first);
    int needed = vsnprintf(NULL, 0, format, first);
    va_end(first);
    if (needed < 0) {
        va_end(second);
        return ATLAS_ERR_FORMAT;
    }
    char *message = (char *)malloc((size_t)needed + 1);
    if (message == NULL) {
        va_end(second);
        return ATLAS_ERR_NOMEM;
    }
    int written = vsnprintf(message, (size_t)needed + 1, format, second);
    va_end(second);
    if (written != needed) {
        free(message);
        return ATLAS_ERR_FORMAT;
    }
    char *code_copy = atlas_strdup_plain(code);
    if (code_copy == NULL) {
        free(message);
        return ATLAS_ERR_NOMEM;
    }
    atlas_diagnostic_entry *entry = &diagnostics->entries[diagnostics->count++];
    entry->severity = severity;
    entry->code = code_copy;
    entry->message = message;
    entry->related_node_id = node_id;
    entry->related_edge_index = edge_index;
    if (severity == ATLAS_DIAGNOSTIC_ERROR) {
        ++diagnostics->error_count;
    } else if (severity == ATLAS_DIAGNOSTIC_WARNING) {
        ++diagnostics->warning_count;
    }
    return ATLAS_OK;
}

static atlas_status atlas_validate_duplicate_edges(const atlas_document *document,
                                                   atlas_diagnostics *diagnostics) {
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        const atlas_string *label = &document->strings[edge->label_index];
        for (size_t previous = 0; previous < edge_index; ++previous) {
            const atlas_edge *candidate = &document->edges[previous];
            const atlas_string *candidate_label =
                &document->strings[candidate->label_index];
            if (edge->source_index == candidate->source_index &&
                edge->target_index == candidate->target_index &&
                label->length == candidate_label->length &&
                memcmp(label->bytes, candidate_label->bytes, label->length) == 0) {
                atlas_status status = atlas_diagnostics_add(
                    diagnostics,
                    ATLAS_DIAGNOSTIC_WARNING,
                    "duplicate-edge",
                    edge->source_id,
                    edge_index,
                    "edge %zu duplicates edge %zu from node %u to node %u",
                    edge_index,
                    previous,
                    edge->source_id,
                    edge->target_id);
                if (status != ATLAS_OK) {
                    return status;
                }
                break;
            }
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_validate_node_shapes(const atlas_document *document,
                                               atlas_diagnostics *diagnostics) {
    for (size_t node_index = 0; node_index < document->node_count; ++node_index) {
        const atlas_node *node = &document->nodes[node_index];
        const atlas_string *name = &document->strings[node->name_index];
        if (name->length == 0) {
            atlas_status status = atlas_diagnostics_add(
                diagnostics,
                ATLAS_DIAGNOSTIC_ERROR,
                "empty-node-name",
                node->id,
                ATLAS_DIAGNOSTIC_EDGE_NONE,
                "node %u has an empty canonical name",
                node->id);
            if (status != ATLAS_OK) {
                return status;
            }
        }
        if (node->incoming_count == 0 && node->outgoing_count == 0) {
            atlas_status status = atlas_diagnostics_add(
                diagnostics,
                ATLAS_DIAGNOSTIC_INFO,
                "isolated-node",
                node->id,
                ATLAS_DIAGNOSTIC_EDGE_NONE,
                "node %u is isolated",
                node->id);
            if (status != ATLAS_OK) {
                return status;
            }
        }
        if (node->attribute_count == 0) {
            atlas_status status = atlas_diagnostics_add(
                diagnostics,
                ATLAS_DIAGNOSTIC_INFO,
                "attribute-free-node",
                node->id,
                ATLAS_DIAGNOSTIC_EDGE_NONE,
                "node %u has no attributes",
                node->id);
            if (status != ATLAS_OK) {
                return status;
            }
        }
    }
    return ATLAS_OK;
}

static atlas_status atlas_validate_self_loops(const atlas_document *document,
                                              atlas_diagnostics *diagnostics) {
    for (size_t edge_index = 0; edge_index < document->edge_count; ++edge_index) {
        const atlas_edge *edge = &document->edges[edge_index];
        if (edge->source_index == edge->target_index) {
            atlas_status status = atlas_diagnostics_add(
                diagnostics,
                ATLAS_DIAGNOSTIC_WARNING,
                "self-loop",
                edge->source_id,
                edge_index,
                "edge %zu creates a self-loop on node %u",
                edge_index,
                edge->source_id);
            if (status != ATLAS_OK) {
                return status;
            }
        }
    }
    return ATLAS_OK;
}

atlas_status atlas_document_validate(atlas_context *context,
                                     const atlas_document *document,
                                     atlas_diagnostics **out_diagnostics) {
    if (context == NULL || document == NULL || out_diagnostics == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_diagnostics = NULL;
    atlas_diagnostics *diagnostics = atlas_calloc(context, 1, sizeof(*diagnostics));
    if (diagnostics == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    atlas_status status = atlas_validate_node_shapes(document, diagnostics);
    if (status == ATLAS_OK) {
        status = atlas_validate_self_loops(document, diagnostics);
    }
    if (status == ATLAS_OK) {
        status = atlas_validate_duplicate_edges(document, diagnostics);
    }
    if (status != ATLAS_OK) {
        atlas_diagnostics_destroy(diagnostics);
        return status;
    }
    *out_diagnostics = diagnostics;
    return ATLAS_OK;
}

void atlas_diagnostics_destroy(atlas_diagnostics *diagnostics) {
    if (diagnostics == NULL) {
        return;
    }
    for (size_t index = 0; index < diagnostics->count; ++index) {
        free(diagnostics->entries[index].code);
        free(diagnostics->entries[index].message);
    }
    free(diagnostics->entries);
    free(diagnostics);
}

size_t atlas_diagnostics_count(const atlas_diagnostics *diagnostics) {
    return diagnostics == NULL ? 0 : diagnostics->count;
}

size_t atlas_diagnostics_error_count(const atlas_diagnostics *diagnostics) {
    return diagnostics == NULL ? 0 : diagnostics->error_count;
}

size_t atlas_diagnostics_warning_count(const atlas_diagnostics *diagnostics) {
    return diagnostics == NULL ? 0 : diagnostics->warning_count;
}

atlas_status atlas_diagnostics_at(const atlas_diagnostics *diagnostics,
                                  size_t index,
                                  atlas_diagnostic_view *out_view) {
    if (diagnostics == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= diagnostics->count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_diagnostic_entry *entry = &diagnostics->entries[index];
    out_view->severity = entry->severity;
    out_view->code = entry->code;
    out_view->message = entry->message;
    out_view->related_node_id = entry->related_node_id;
    out_view->related_edge_index = entry->related_edge_index;
    return ATLAS_OK;
}
