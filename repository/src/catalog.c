#include "internal.h"

static atlas_status atlas_catalog_reserve(atlas_catalog *catalog, size_t additional) {
    if (additional > SIZE_MAX - catalog->count) {
        return ATLAS_ERR_RANGE;
    }
    size_t required = catalog->count + additional;
    if (required <= catalog->capacity) {
        return ATLAS_OK;
    }
    size_t capacity = catalog->capacity == 0 ? 8 : catalog->capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            return ATLAS_ERR_RANGE;
        }
        capacity *= 2;
    }
    atlas_catalog_entry *replacement =
        (atlas_catalog_entry *)realloc(catalog->entries,
                                       capacity * sizeof(*replacement));
    if (replacement == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    catalog->entries = replacement;
    catalog->capacity = capacity;
    return ATLAS_OK;
}

static atlas_status atlas_catalog_add(atlas_catalog *catalog,
                                      const char *text,
                                      size_t length) {
    for (size_t index = 0; index < catalog->count; ++index) {
        atlas_catalog_entry *entry = &catalog->entries[index];
        if (entry->length == length && memcmp(entry->text, text, length) == 0) {
            ++entry->usage_count;
            return ATLAS_OK;
        }
    }
    atlas_status status = atlas_catalog_reserve(catalog, 1);
    if (status != ATLAS_OK) {
        return status;
    }
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    memcpy(copy, text, length);
    copy[length] = '\0';
    atlas_catalog_entry *entry = &catalog->entries[catalog->count++];
    entry->text = copy;
    entry->length = length;
    entry->usage_count = 1;
    return ATLAS_OK;
}

static void atlas_catalog_sort(atlas_catalog *catalog) {
    for (size_t index = 1; index < catalog->count; ++index) {
        atlas_catalog_entry value = catalog->entries[index];
        size_t position = index;
        while (position > 0) {
            atlas_catalog_entry *previous = &catalog->entries[position - 1];
            int move = previous->usage_count < value.usage_count;
            if (previous->usage_count == value.usage_count) {
                move = strcmp(previous->text, value.text) > 0;
            }
            if (!move) {
                break;
            }
            catalog->entries[position] = *previous;
            --position;
        }
        catalog->entries[position] = value;
    }
}

static atlas_status atlas_catalog_create(atlas_context *context,
                                         atlas_catalog **out_catalog) {
    atlas_catalog *catalog = atlas_calloc(context, 1, sizeof(*catalog));
    if (catalog == NULL) {
        return ATLAS_ERR_NOMEM;
    }
    *out_catalog = catalog;
    return ATLAS_OK;
}

atlas_status atlas_document_label_catalog(atlas_context *context,
                                          const atlas_document *document,
                                          atlas_catalog **out_catalog) {
    if (context == NULL || document == NULL || out_catalog == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_catalog = NULL;
    atlas_catalog *catalog = NULL;
    atlas_status status = atlas_catalog_create(context, &catalog);
    for (size_t index = 0; index < document->edge_count && status == ATLAS_OK; ++index) {
        const atlas_string *label = &document->strings[document->edges[index].label_index];
        status = atlas_catalog_add(catalog, label->bytes, label->length);
    }
    if (status != ATLAS_OK) {
        atlas_catalog_destroy(catalog);
        return status;
    }
    atlas_catalog_sort(catalog);
    *out_catalog = catalog;
    return ATLAS_OK;
}

atlas_status atlas_document_attribute_key_catalog(atlas_context *context,
                                                  const atlas_document *document,
                                                  atlas_catalog **out_catalog) {
    if (context == NULL || document == NULL || out_catalog == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    *out_catalog = NULL;
    atlas_catalog *catalog = NULL;
    atlas_status status = atlas_catalog_create(context, &catalog);
    for (size_t index = 0; index < document->attribute_count && status == ATLAS_OK; ++index) {
        const atlas_string *key = &document->strings[document->attributes[index].key_index];
        status = atlas_catalog_add(catalog, key->bytes, key->length);
    }
    if (status != ATLAS_OK) {
        atlas_catalog_destroy(catalog);
        return status;
    }
    atlas_catalog_sort(catalog);
    *out_catalog = catalog;
    return ATLAS_OK;
}

void atlas_catalog_destroy(atlas_catalog *catalog) {
    if (catalog == NULL) {
        return;
    }
    for (size_t index = 0; index < catalog->count; ++index) {
        free(catalog->entries[index].text);
    }
    free(catalog->entries);
    free(catalog);
}

size_t atlas_catalog_count(const atlas_catalog *catalog) {
    return catalog == NULL ? 0 : catalog->count;
}

atlas_status atlas_catalog_at(const atlas_catalog *catalog,
                              size_t index,
                              atlas_catalog_entry_view *out_view) {
    if (catalog == NULL || out_view == NULL) {
        return ATLAS_ERR_ARGUMENT;
    }
    if (index >= catalog->count) {
        return ATLAS_ERR_RANGE;
    }
    const atlas_catalog_entry *entry = &catalog->entries[index];
    out_view->text = entry->text;
    out_view->text_length = entry->length;
    out_view->usage_count = entry->usage_count;
    return ATLAS_OK;
}
