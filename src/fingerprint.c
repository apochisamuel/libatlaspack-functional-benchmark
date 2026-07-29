#include "internal.h"

static uint64_t atlas_fingerprint_string(uint64_t hash, const atlas_string *string) {
    hash = atlas_hash_combine(hash, string->length);
    hash = atlas_hash_combine(hash, atlas_hash_bytes(string->bytes, string->length));
    return hash;
}

uint64_t atlas_document_fingerprint(const atlas_document *document) {
    if (document == NULL) {
        return 0;
    }
    uint64_t hash = UINT64_C(0x4154504b00000001);
    hash = atlas_hash_combine(hash, document->header.version);
    hash = atlas_hash_combine(hash, document->node_count);
    hash = atlas_hash_combine(hash, document->edge_count);
    hash = atlas_hash_combine(hash, document->attribute_count);
    hash = atlas_hash_combine(hash, document->metadata_count);
    for (size_t index = 0; index < document->node_count; ++index) {
        const atlas_node *node = &document->nodes[index];
        hash = atlas_hash_combine(hash, node->id);
        hash = atlas_fingerprint_string(hash, &document->strings[node->name_index]);
        hash = atlas_hash_combine(hash, node->attribute_count);
    }
    for (size_t index = 0; index < document->edge_count; ++index) {
        const atlas_edge *edge = &document->edges[index];
        hash = atlas_hash_combine(hash, edge->source_id);
        hash = atlas_hash_combine(hash, edge->target_id);
        hash = atlas_fingerprint_string(hash, &document->strings[edge->label_index]);
    }
    for (size_t index = 0; index < document->attribute_count; ++index) {
        const atlas_attribute *attribute = &document->attributes[index];
        hash = atlas_hash_combine(hash, attribute->node_id);
        hash = atlas_fingerprint_string(hash, &document->strings[attribute->key_index]);
        hash = atlas_fingerprint_string(hash, &document->strings[attribute->value_index]);
    }
    for (size_t index = 0; index < document->metadata_count; ++index) {
        const atlas_metadata *metadata = &document->metadata[index];
        hash = atlas_fingerprint_string(hash, &document->strings[metadata->key_index]);
        hash = atlas_fingerprint_string(hash, &document->strings[metadata->value_index]);
    }
    return hash;
}
