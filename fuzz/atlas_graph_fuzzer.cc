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
        atlas_plan *plan = nullptr;
        if (atlas_plan_build(context, document, &plan) == ATLAS_OK) {
            for (size_t i = 0; i < atlas_plan_count(plan); ++i) {
                atlas_node_view view;
                (void)atlas_plan_node_at(plan, i, &view);
            }
            atlas_plan_destroy(plan);
        }
        atlas_document_destroy(document);
    }
    atlas_context_destroy(context);
    return 0;
}
