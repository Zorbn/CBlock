#include "chunk.h"

#include <stdlib.h>
#include <assert.h>

const size_t chunk_size = 16;
const size_t chunk_height = 256;
const size_t chunk_length = chunk_size * chunk_size * chunk_height;
const size_t heightmap_length = chunk_size * chunk_size;
const float inv_light_level_count = 1.0f / MAX_LIGHT_LEVEL;

struct Chunk chunk_create(int32_t x, int32_t z) {
    struct Chunk chunk = (struct Chunk){
        .blocks = calloc(chunk_length, sizeof(uint8_t)),
        .lightmap = calloc(chunk_length, sizeof(uint8_t)),
        .x = x,
        .z = z,
        .heightmap_min = malloc(heightmap_length * sizeof(int32_t)),
        .heightmap_max = calloc(heightmap_length, sizeof(int32_t)),
        .is_dirty = false,
    };

    assert(chunk.blocks);
    assert(chunk.lightmap);
    assert(chunk.heightmap_min);
    assert(chunk.heightmap_max);

    for (size_t i = 0; i < heightmap_length; i++) {
        chunk.heightmap_min[i] = chunk_height;
    }

    size_t ground_height = chunk_height / 2;
    for (size_t z = 0; z < chunk_size; z++) {
        for (size_t x = 0; x < chunk_size; x++) {
            for (size_t y = 0; y <= ground_height; y++) {
                uint8_t block = 1;

                if (y == ground_height) {
                    block = 2;
                }

                chunk_set_block(&chunk, x, y, z, block);
            }
        }
    }

    return chunk;
}

void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block) {
    if (block != 0) {
        int32_t heightmap_index = HEIGHTMAP_INDEX(x, z);
        int32_t *heightmap_current_min = chunk->heightmap_min + heightmap_index;
        int32_t *heightmap_current_max = chunk->heightmap_max + heightmap_index;

        if (y < *heightmap_current_min) {
            *heightmap_current_min = y;
        } else if (y > *heightmap_current_max) {
            *heightmap_current_max = y;
        }
    }

    size_t i = BLOCK_INDEX(x, y, z);
    chunk->blocks[i] = block;
    chunk->is_dirty = true;
}

void chunk_destroy(struct Chunk *chunk) {
    free(chunk->blocks);
    free(chunk->lightmap);
    free(chunk->heightmap_min);
    free(chunk->heightmap_max);
}

extern inline uint8_t chunk_get_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z);
extern inline uint8_t chunk_get_light_level(struct Chunk *chunk, int32_t x, int32_t y, int32_t z);
extern inline void chunk_set_light_level(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t light_level);