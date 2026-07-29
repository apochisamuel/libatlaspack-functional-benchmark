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
        for (size_t i = 0; i < atlas_document_node_count(document); ++i) {
            atlas_node_view view;
            if (atlas_document_node_at(document, i, &view) == ATLAS_OK) {
                atlas_node_view found;
                (void)atlas_document_find_node(document, view.name, &found);
            }
        }
        char *report = nullptr;
        size_t report_size = 0;
        if (atlas_document_report(context, document, &report, &report_size) == ATLAS_OK)
            atlas_free(report);
        atlas_document_destroy(document);
    }
    atlas_context_destroy(context);
    return 0;
}
