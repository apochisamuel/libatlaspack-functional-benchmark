#include "internal.h"

static void atlas_rank_sort(atlas_rank *rank) {
    size_t count = rank->document->node_count;
    for (size_t index = 0; index < count; ++index) {
        rank->order[index] = index;
    }
    for (size_t index = 1; index < count; ++index) {
        size_t node_index = rank->order[index];
        size_t position = index;
        while (position > 0) {
            size_t previous_index = rank->order[position - 1];
            double previous_score = rank->scores[previous_index];
            double current_score = rank->scores[node_index];
            if (previous_score > current_score) {
                break;
            }
            if (previous_score == current_score &&
                rank->document->nodes[previous_index].id <
                    rank->document->nodes[node_index].id) {
                break;
            }
            rank->order[position] = previous_index;
            --position;
        }
        rank->order[position] = node_index;
    }
}

atlas_status atlas_rank_compute(atlas_context *context,
                                const atlas_document *document,
                                double damping,
                                size_t iterations,
                                atlas_rank **out_rank) {
    if (context == NULL || document == NULL || out_rank == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_rank = NULL;
    if (!(damping >= 0.0 && damping <= 1.0) || iterations == 0) {
        atlas_set_error(context, "rank parameters are outside their valid bounds");
        return ATLAS_ERR_ARGUMENT;
    }
    atlas_rank *rank = atlas_calloc(context, 1, sizeof(*rank));
    if (rank == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    size_t count = document->node_count;
    rank->scores = atlas_calloc(context, count, sizeof(*rank->scores));
    rank->order = atlas_calloc(context, count, sizeof(*rank->order));
    double *next = atlas_calloc(context, count, sizeof(*next));
    if ((rank->scores == NULL || rank->order == NULL || next == NULL) && count != 0) {
        free(next);
        atlas_rank_destroy(rank);
        return ATLAS_ERR_NOMEM;
    }
    rank->document = document;
    if (count == 0) {
        free(next);
        *out_rank = rank;
        return ATLAS_OK;
    }
    double initial = 1.0 / (double)count;
    for (size_t index = 0; index < count; ++index) {
        rank->scores[index] = initial;
    }
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        double dangling = 0.0;
        for (size_t index = 0; index < count; ++index) {
            if (document->nodes[index].outgoing_count == 0) {
                dangling += rank->scores[index];
            }
            next[index] = (1.0 - damping) / (double)count;
        }
        double dangling_share = damping * dangling / (double)count;
        for (size_t index = 0; index < count; ++index) {
            next[index] += dangling_share;
        }
        for (size_t node_index = 0; node_index < count; ++node_index) {
            const atlas_node *node = &document->nodes[node_index];
            if (node->outgoing_count == 0) {
                continue;
            }
            double contribution = damping * rank->scores[node_index] /
                                  (double)node->outgoing_count;
            for (size_t ordinal = 0; ordinal < node->outgoing_count; ++ordinal) {
                size_t edge_index =
                    document->edge_order[node->outgoing_start + ordinal];
                size_t target_index = document->edges[edge_index].target_index;
                next[target_index] += contribution;
            }
        }
        double *swap = rank->scores;
        rank->scores = next;
        next = swap;
    }
    free(next);
    atlas_rank_sort(rank);
    *out_rank = rank;
    return ATLAS_OK;
}

void atlas_rank_destroy(atlas_rank *rank) {
    if (rank == NULL) {
        return;
    }
    free(rank->scores);
    free(rank->order);
    free(rank);
}

atlas_status atlas_rank_score(const atlas_rank *rank,
                              uint32_t node_id,
                              double *out_score) {
    if (rank == NULL || out_score == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    size_t node_index = atlas_node_index_by_id(rank->document, node_id);
    if (node_index == ATLAS_NO_INDEX) {
        return ATLAS_ERR_NOT_FOUND;
    }
    *out_score = rank->scores[node_index];
    return ATLAS_OK;
}

atlas_status atlas_rank_node_at(const atlas_rank *rank,
                                size_t ordinal,
                                atlas_node_view *out_node,
                                double *out_score) {
    if (rank == NULL || out_node == NULL || out_score == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (ordinal >= rank->document->node_count) {
        return ATLAS_ERR_RANGE;
    }
    size_t node_index = rank->order[ordinal];
    atlas_status status = atlas_document_node_at(rank->document,
                                                 node_index,
                                                 out_node);
    if (status == ATLAS_OK) {
        *out_score = rank->scores[node_index];
    }
    return status;
}
