#include "internal.h"

atlas_status atlas_distance_map_build(atlas_context *context,
                                      const atlas_document *document,
                                      uint32_t start_node_id,
                                      int reverse,
                                      atlas_distance_map **out_map) {
    if (context == NULL || document == NULL || out_map == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_map = NULL;
    size_t start_index = atlas_node_index_by_id(document, start_node_id);
    if (start_index == ATLAS_NO_INDEX) {
        atlas_set_error(context, "distance start node %u does not exist", start_node_id);
        return ATLAS_ERR_NOT_FOUND;
    }
    atlas_distance_map *map = atlas_calloc(context, 1, sizeof(*map));
    if (map == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    map->distances = atlas_calloc(context,
                                  document->node_count,
                                  sizeof(*map->distances));
    size_t *queue = atlas_calloc(context,
                                 document->node_count,
                                 sizeof(*queue));
    if ((map->distances == NULL || queue == NULL) && document->node_count != 0) {
        free(queue);
        atlas_distance_map_destroy(map);
        return ATLAS_ERR_NOMEM;
    }
    for (size_t index = 0; index < document->node_count; ++index) {
        map->distances[index] = ATLAS_NO_INDEX;
    }
    map->document = document;
    map->distances[start_index] = 0;
    map->reachable_count = 1;
    size_t head = 0;
    size_t tail = 0;
    queue[tail++] = start_index;
    while (head < tail) {
        size_t node_index = queue[head++];
        const atlas_node *node = &document->nodes[node_index];
        size_t count = reverse ? node->incoming_count : node->outgoing_count;
        size_t start = reverse ? node->incoming_start : node->outgoing_start;
        const size_t *order = reverse ? document->incoming_order : document->edge_order;
        for (size_t ordinal = 0; ordinal < count; ++ordinal) {
            size_t edge_index = order[start + ordinal];
            const atlas_edge *edge = &document->edges[edge_index];
            size_t neighbor = reverse ? edge->source_index : edge->target_index;
            if (map->distances[neighbor] != ATLAS_NO_INDEX) {
                continue;
            }
            map->distances[neighbor] = map->distances[node_index] + 1;
            if (map->distances[neighbor] > map->maximum) {
                map->maximum = map->distances[neighbor];
            }
            ++map->reachable_count;
            queue[tail++] = neighbor;
        }
    }
    free(queue);
    *out_map = map;
    return ATLAS_OK;
}

void atlas_distance_map_destroy(atlas_distance_map *map) {
    if (map == NULL) {
        return;
    }
    free(map->distances);
    free(map);
}

atlas_status atlas_distance_map_get(const atlas_distance_map *map,
                                    uint32_t node_id,
                                    size_t *out_distance) {
    if (map == NULL || out_distance == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(map->document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    if (map->distances[node_index] == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    *out_distance = map->distances[node_index];
    return ATLAS_OK;
}

size_t atlas_distance_map_reachable_count(const atlas_distance_map *map) {
    return map == NULL ? 0 : map->reachable_count;
}

size_t atlas_distance_map_maximum(const atlas_distance_map *map) {
    return map == NULL ? 0 : map->maximum;
}
