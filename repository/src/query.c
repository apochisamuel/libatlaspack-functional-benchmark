#include "internal.h"

size_t atlas_document_node_count(const atlas_document *document) {
    return document == NULL ? 0 : document->node_count;
}

size_t atlas_document_edge_count(const atlas_document *document) {
    return document == NULL ? 0 : document->edge_count;
}

size_t atlas_document_string_count(const atlas_document *document) {
    return document == NULL ? 0 : document->string_count;
}

size_t atlas_document_attribute_count(const atlas_document *document) {
    return document == NULL ? 0 : document->attribute_count;
}

uint16_t atlas_document_version(const atlas_document *document) {
    return document == NULL ? 0 : document->header.version;
}

uint16_t atlas_document_flags(const atlas_document *document) {
    return document == NULL ? 0 : document->header.flags;
}

int atlas_document_is_normalized(const atlas_document *document) {
    return document != NULL && document->normalized != 0;
}

atlas_status atlas_document_node_at(const atlas_document *document,
                                    size_t index,
                                    atlas_node_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= document->node_count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_node *node = &document->nodes[index];
    const atlas_string *name = &document->strings[node->name_index];
    out_view->id = node->id;
    out_view->name = name->bytes;
    out_view->name_length = name->length;
    out_view->outgoing_edges = node->outgoing_count;
    out_view->incoming_edges = node->incoming_count;
    out_view->attribute_count = node->attribute_count;
    return ATLAS_OK;
}

atlas_status atlas_document_edge_at(const atlas_document *document,
                                    size_t index,
                                    atlas_edge_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= document->edge_count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_edge *edge = &document->edges[index];
    const atlas_string *label = &document->strings[edge->label_index];
    out_view->source_id = edge->source_id;
    out_view->target_id = edge->target_id;
    out_view->label = label->bytes;
    out_view->label_length = label->length;
    return ATLAS_OK;
}

atlas_status atlas_document_attribute_at(const atlas_document *document,
                                         size_t index,
                                         atlas_attribute_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= document->attribute_count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_attribute *attribute = &document->attributes[index];
    const atlas_string *key = &document->strings[attribute->key_index];
    const atlas_string *value = &document->strings[attribute->value_index];
    out_view->node_id = attribute->node_id;
    out_view->key = key->bytes;
    out_view->key_length = key->length;
    out_view->value = value->bytes;
    out_view->value_length = value->length;
    return ATLAS_OK;
}

atlas_status atlas_document_find_node(const atlas_document *document,
                                      const char *name,
                                      atlas_node_view *out_view) {
    if (document == NULL || name == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t length = strlen(name);
    uint64_t hash = atlas_hash_bytes(name, length);
    if (document->name_index_capacity == 0) {
        return ATLAS_ERR_NOT_FOUND;
    }
    size_t mask = document->name_index_capacity - 1;
    size_t position = (size_t)hash & mask;
    for (size_t probes = 0; probes < document->name_index_capacity; ++probes) {
        const atlas_name_slot *slot = &document->name_index[position];
        if (!slot->used) {
            return ATLAS_ERR_NOT_FOUND;
        }
        const atlas_string *candidate = &document->strings[slot->string_index];
        if (candidate->hash == hash && candidate->length == length &&
            memcmp(candidate->bytes, name, length) == 0) {
            return atlas_document_node_at(document, slot->node_index, out_view);
        }
        position = (position + 1) & mask;
    }
    return ATLAS_ERR_NOT_FOUND;
}

atlas_status atlas_document_find_node_by_id(const atlas_document *document,
                                            uint32_t id,
                                            atlas_node_view *out_view) {
    if (document == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    int found = 0;
    size_t index = atlas_id_lookup(document, id, &found);
    if (!found) {
        return ATLAS_ERR_NOT_FOUND;
    }
    return atlas_document_node_at(document, index, out_view);
}

size_t atlas_node_index_by_id(const atlas_document *document, uint32_t id) {
    if (document == NULL) {
        return ATLAS_NO_INDEX;
    }
    int found = 0;
    size_t index = atlas_id_lookup(document, id, &found);
    return found ? index : ATLAS_NO_INDEX;
}
