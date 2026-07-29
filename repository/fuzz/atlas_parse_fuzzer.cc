#include "atlas.h"
#include <cstddef>
#include <cstdint>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 64 || size > 16 * 1024 * 1024) return 0;
    atlas_context *context = atlas_context_create();
    if (!context) return 0;
    atlas_parse_options options = atlas_parse_options_default();
    options.validate_checksum = 0;
    atlas_document *document = nullptr;
    if (atlas_parse(context, data, size, &options, &document) == ATLAS_OK) {
        (void)atlas_document_node_count(document);
        (void)atlas_document_edge_count(document);
        atlas_document_destroy(document);
    }
    atlas_context_destroy(context);
    return 0;
}
