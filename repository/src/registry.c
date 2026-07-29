#include "internal.h"

static size_t next_power_of_two(size_t value) {
    size_t result = 8;
    while (result < value) {
        if (result > SIZE_MAX / 2) return 0;
        result *= 2;
    }
    return result;
}

atlas_status atlas_build_registry(atlas_context *context, atlas_document *document) {
    size_t capacity = next_power_of_two(document->node_count * 2 + 1);
    if (capacity == 0) return ATLAS_ERR_RANGE;
    atlas_id_slot *slots = atlas_calloc(context, capacity, sizeof(*slots));
    if (slots == NULL) return ATLAS_ERR_NOMEM;
    for (size_t i = 0; i < document->node_count; ++i) {
        uint32_t key = document->nodes[i].id;
        size_t mask = capacity - 1;
        size_t position = ((uint64_t)key * UINT64_C(2654435761)) & mask;
        while (slots[position].used) {
            if (slots[position].key == key) {
                free(slots);
                atlas_set_error(context, "duplicate node id %u", key);
                return ATLAS_ERR_DUPLICATE;
            }
            position = (position + 1) & mask;
        }
        slots[position].used = 1;
        slots[position].key = key;
        slots[position].value = i;
    }
    document->id_index = slots;
    document->id_index_capacity = capacity;
    return ATLAS_OK;
}

size_t atlas_id_lookup(const atlas_document *document, uint32_t id, int *found) {
    if (document->id_index_capacity == 0) {
        *found = 0;
        return 0;
    }
    size_t mask = document->id_index_capacity - 1;
    size_t position = ((uint64_t)id * UINT64_C(2654435761)) & mask;
    for (size_t probes = 0; probes < document->id_index_capacity; ++probes) {
        const atlas_id_slot *slot = &document->id_index[position];
        if (!slot->used) break;
        if (slot->key == id) {
            *found = 1;
            return slot->value;
        }
        position = (position + 1) & mask;
    }
    *found = 0;
    return 0;
}
