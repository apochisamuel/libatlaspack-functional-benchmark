#include "internal.h"

uint32_t atlas_checksum32(const uint8_t *data, size_t size) {
    uint32_t hash = 2166136261u;
    for (size_t index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= 16777619u;
    }
    return hash;
}

uint64_t atlas_hash_bytes(const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

uint64_t atlas_hash_combine(uint64_t left, uint64_t right) {
    right += UINT64_C(0x9e3779b97f4a7c15);
    right += left << 6;
    right += left >> 2;
    return left ^ right;
}
