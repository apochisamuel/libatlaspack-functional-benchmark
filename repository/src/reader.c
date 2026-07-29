#include "internal.h"

void atlas_reader_init(atlas_reader *reader, const uint8_t *data, size_t size) {
    if (reader == NULL) {
        return;
    }
    reader->data = data;
    reader->size = size;
    reader->position = 0;
}

bool atlas_reader_seek(atlas_reader *reader, size_t position) {
    if (reader == NULL || position > reader->size) {
        return false;
    }
    reader->position = position;
    return true;
}

bool atlas_reader_skip(atlas_reader *reader, size_t amount) {
    if (reader == NULL || reader->position > reader->size ||
        amount > reader->size - reader->position) {
        return false;
    }
    reader->position += amount;
    return true;
}

bool atlas_reader_bytes(atlas_reader *reader, const uint8_t **bytes, size_t amount) {
    if (reader == NULL || bytes == NULL || reader->position > reader->size ||
        amount > reader->size - reader->position) {
        return false;
    }
    *bytes = reader->data + reader->position;
    reader->position += amount;
    return true;
}

bool atlas_reader_u16(atlas_reader *reader, uint16_t *value) {
    const uint8_t *bytes = NULL;
    if (value == NULL || !atlas_reader_bytes(reader, &bytes, 2)) {
        return false;
    }
    *value = (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8);
    return true;
}

bool atlas_reader_u32(atlas_reader *reader, uint32_t *value) {
    const uint8_t *bytes = NULL;
    if (value == NULL || !atlas_reader_bytes(reader, &bytes, 4)) {
        return false;
    }
    *value = (uint32_t)bytes[0] |
             ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) |
             ((uint32_t)bytes[3] << 24);
    return true;
}

bool atlas_reader_u64(atlas_reader *reader, uint64_t *value) {
    uint32_t low = 0;
    uint32_t high = 0;
    if (value == NULL || !atlas_reader_u32(reader, &low) ||
        !atlas_reader_u32(reader, &high)) {
        return false;
    }
    *value = (uint64_t)low | ((uint64_t)high << 32);
    return true;
}

size_t atlas_reader_remaining(const atlas_reader *reader) {
    if (reader == NULL || reader->position > reader->size) {
        return 0;
    }
    return reader->size - reader->position;
}
