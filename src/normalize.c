#include "internal.h"

atlas_status atlas_normalize(atlas_context *context, atlas_document *document) {
    if (document == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (!document->options.normalize_names) {
        return ATLAS_OK;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        atlas_node *node = &document->nodes[index];
        if (node->name_index >= document->string_count) {
            atlas_set_error(context, "node %u cannot be normalized before references resolve",
                            node->id);
            return ATLAS_ERR_REFERENCE;
        }
        atlas_string *name = &document->strings[node->name_index];
        atlas_ascii_normalize(name->bytes);
        name->length = strlen(name->bytes);
        name->hash = atlas_hash_bytes(name->bytes, name->length);
    }
    document->normalized = 1;
    return ATLAS_OK;
}
