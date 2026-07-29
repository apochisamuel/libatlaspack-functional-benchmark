#include "internal.h"

int atlas_utf8_valid(const unsigned char *text, size_t length) {
    if (text == NULL && length != 0) {
        return 0;
    }
    size_t index = 0;
    while (index < length) {
        unsigned char first = text[index++];
        if (first < 0x80) {
            continue;
        }
        unsigned continuation_count = 0;
        uint32_t codepoint = 0;
        uint32_t minimum = 0;
        if ((first & 0xe0) == 0xc0) {
            continuation_count = 1;
            codepoint = first & 0x1f;
            minimum = 0x80;
        } else if ((first & 0xf0) == 0xe0) {
            continuation_count = 2;
            codepoint = first & 0x0f;
            minimum = 0x800;
        } else if ((first & 0xf8) == 0xf0) {
            continuation_count = 3;
            codepoint = first & 0x07;
            minimum = 0x10000;
        } else {
            return 0;
        }
        if (continuation_count > length - index) {
            return 0;
        }
        for (unsigned part = 0; part < continuation_count; ++part) {
            unsigned char continuation = text[index++];
            if ((continuation & 0xc0) != 0x80) {
                return 0;
            }
            codepoint = (codepoint << 6) | (continuation & 0x3f);
        }
        if (codepoint < minimum || codepoint > 0x10ffff) {
            return 0;
        }
        if (codepoint >= 0xd800 && codepoint <= 0xdfff) {
            return 0;
        }
    }
    return 1;
}

size_t atlas_ascii_normalized_length(const char *text, size_t length) {
    if (text == NULL) {
        return 0;
    }
    size_t output = 0;
    int have_content = 0;
    int pending_space = 0;
    for (size_t index = 0; index < length; ++index) {
        unsigned char value = (unsigned char)text[index];
        int whitespace = value == ' ' || value == '\t' ||
                         value == '\r' || value == '\n';
        if (whitespace) {
            if (have_content) {
                pending_space = 1;
            }
            continue;
        }
        if (pending_space) {
            ++output;
            pending_space = 0;
        }
        ++output;
        have_content = 1;
    }
    return output;
}

void atlas_ascii_normalize(char *text) {
    if (text == NULL) {
        return;
    }
    char *write = text;
    int have_content = 0;
    int pending_space = 0;
    for (char *read = text; *read != '\0'; ++read) {
        unsigned char value = (unsigned char)*read;
        int whitespace = value == ' ' || value == '\t' ||
                         value == '\r' || value == '\n';
        if (whitespace) {
            if (have_content) {
                pending_space = 1;
            }
            continue;
        }
        if (pending_space) {
            *write++ = ' ';
            pending_space = 0;
        }
        if (value >= 'A' && value <= 'Z') {
            value = (unsigned char)(value - 'A' + 'a');
        }
        *write++ = (char)value;
        have_content = 1;
    }
    *write = '\0';
}

int atlas_string_equal(const atlas_string *string, const char *text) {
    if (string == NULL || text == NULL) {
        return 0;
    }
    size_t length = strlen(text);
    return string->length == length &&
           memcmp(string->bytes, text, length) == 0;
}

int atlas_string_has_prefix(const atlas_string *string, const char *prefix) {
    if (string == NULL || prefix == NULL) {
        return 0;
    }
    size_t length = strlen(prefix);
    return length <= string->length &&
           memcmp(string->bytes, prefix, length) == 0;
}
