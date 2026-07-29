#include "internal.h"

atlas_context *atlas_context_create(void) {
    atlas_context *context = (atlas_context *)calloc(1, sizeof(*context));
    if (context != NULL) {
        context->allocation_limit = 64u * 1024u * 1024u;
    }
    return context;
}

void atlas_context_destroy(atlas_context *context) {
    free(context);
}

void atlas_context_reset(atlas_context *context) {
    if (context == NULL) {
        return;
    }
    context->error[0] = '\0';
    context->allocated = 0;
}

const char *atlas_context_error(const atlas_context *context) {
    return context == NULL ? "invalid context" : context->error;
}

size_t atlas_context_allocation_limit(const atlas_context *context) {
    return context == NULL ? 0 : context->allocation_limit;
}

void atlas_context_set_allocation_limit(atlas_context *context, size_t limit) {
    if (context == NULL) {
        return;
    }
    context->allocation_limit = limit == 0 ? 64u * 1024u * 1024u : limit;
}

void atlas_set_error(atlas_context *context, const char *format, ...) {
    if (context == NULL || format == NULL) {
        return;
    }
    va_list args;
    va_start(args, format);
    vsnprintf(context->error, sizeof(context->error), format, args);
    va_end(args);
}

void *atlas_alloc(atlas_context *context, size_t size) {
    if (size == 0) {
        size = 1;
    }
    if (context != NULL) {
        if (context->allocated > context->allocation_limit ||
            size > context->allocation_limit - context->allocated) {
            atlas_set_error(context, "allocation limit exceeded");
            return NULL;
        }
    }
    void *pointer = malloc(size);
    if (pointer == NULL) {
        atlas_set_error(context, "allocation of %zu bytes failed", size);
        return NULL;
    }
    if (context != NULL) {
        context->allocated += size;
    }
    return pointer;
}

void *atlas_calloc(atlas_context *context, size_t count, size_t size) {
    if (count != 0 && size > SIZE_MAX / count) {
        atlas_set_error(context, "allocation size overflow");
        return NULL;
    }
    size_t total = count * size;
    void *pointer = atlas_alloc(context, total);
    if (pointer != NULL) {
        memset(pointer, 0, total);
    }
    return pointer;
}

char *atlas_strdup_n(atlas_context *context, const char *data, size_t size) {
    if (data == NULL && size != 0) {
        atlas_set_error(context, "cannot copy a null string");
        return NULL;
    }
    if (size == SIZE_MAX) {
        atlas_set_error(context, "string size overflow");
        return NULL;
    }
    char *copy = (char *)atlas_alloc(context, size + 1);
    if (copy == NULL) {
        return NULL;
    }
    if (size != 0) {
        memcpy(copy, data, size);
    }
    copy[size] = '\0';
    return copy;
}

char *atlas_strdup_plain(const char *text) {
    if (text == NULL) {
        return NULL;
    }
    size_t length = strlen(text);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, length + 1);
    return copy;
}

atlas_parse_options atlas_parse_options_default(void) {
    atlas_parse_options options;
    options.validate_checksum = 1;
    options.allow_unknown_sections = 1;
    options.normalize_names = 1;
    options.require_metadata_keys_unique = 1;
    options.allocation_limit = 64u * 1024u * 1024u;
    return options;
}

void atlas_free(void *pointer) {
    free(pointer);
}

const char *atlas_status_name(atlas_status status) {
    switch (status) {
        case ATLAS_OK: return "ok";
        case ATLAS_ERR_ARGUMENT: return "argument";
        case ATLAS_ERR_NOMEM: return "nomem";
        case ATLAS_ERR_TRUNCATED: return "truncated";
        case ATLAS_ERR_MAGIC: return "magic";
        case ATLAS_ERR_VERSION: return "version";
        case ATLAS_ERR_FORMAT: return "format";
        case ATLAS_ERR_RANGE: return "range";
        case ATLAS_ERR_DUPLICATE: return "duplicate";
        case ATLAS_ERR_REFERENCE: return "reference";
        case ATLAS_ERR_CYCLE: return "cycle";
        case ATLAS_ERR_NOT_FOUND: return "not-found";
        case ATLAS_ERR_CONFLICT: return "conflict";
        case ATLAS_ERR_POLICY: return "policy";
        case ATLAS_ERR_IO: return "io";
        case ATLAS_ERR_UNSUPPORTED: return "unsupported";
        default: return "unknown";
    }
}
