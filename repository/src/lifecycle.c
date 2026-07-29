#include "internal.h"

void atlas_document_destroy(atlas_document *document) {
    if (document == NULL) {
        return;
    }
    for (size_t index = 0; index < document->string_count; ++index) {
        free(document->strings[index].bytes);
    }
    free(document->strings);
    free(document->sections);
    free(document->nodes);
    free(document->edges);
    free(document->attributes);
    free(document->metadata);
    free(document->id_index);
    free(document->name_index);
    free(document->edge_order);
    free(document->incoming_order);
    free(document);
}
