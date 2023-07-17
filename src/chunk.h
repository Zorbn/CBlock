#ifndef CHUNK_H
#define CHUNK_H

#include "detect_leak.h"

#include <inttypes.h>
#include <stdbool.h>

extern const size_t chunk_size;
extern const size_t chunk_height;
extern const size_t chunk_length;
#define MAX_LIGHT_LEVEL 15
extern const float inv_light_level_count;

struct Chunk {
    uint8_t *blocks;
    uint8_t *lightmap;
    uint32_t x;
    uint32_t z;
    int32_t *heightmap_min;
    int32_t *heightmap_max;
    bool is_dirty;
};

#define BLOCK_INDEX(x, y, z) ((y) + (x)*chunk_height + (z)*chunk_height * chunk_size)
#define HEIGHTMAP_INDEX(x, z) ((x) + (z)*chunk_size)

struct Chunk chunk_create(int32_t x, int32_t z);
void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block);
void chunk_destroy(struct Chunk *chunk);

inline uint8_t chunk_get_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z) {
    size_t i = BLOCK_INDEX(x, y, z);
    return chunk->blocks[i];
}

inline uint8_t chunk_get_light_level(struct Chunk *chunk, int32_t x, int32_t y, int32_t z) {
    size_t i = BLOCK_INDEX(x, y, z);
    return chunk->lightmap[i];
}

inline void chunk_set_light_level(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t light_level) {
    size_t i = BLOCK_INDEX(x, y, z);
    chunk->lightmap[i] = light_level;
}

#endif