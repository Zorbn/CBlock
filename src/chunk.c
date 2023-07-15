#include "chunk.h"

#include <stdlib.h>
#include <assert.h>

const size_t chunk_size = 16;
const size_t chunk_height = 256;
const size_t chunk_length = chunk_size * chunk_size * chunk_height;
const size_t heightmap_length = chunk_size * chunk_size;

struct Chunk chunk_create(int32_t x, int32_t z) {
    struct Chunk chunk = (struct Chunk){
        .blocks = calloc(chunk_length, sizeof(uint8_t)),
        .x = x,
        .z = z,
        .heightmap_min = malloc(heightmap_length * sizeof(int32_t)),
        .heightmap_max = calloc(heightmap_length, sizeof(int32_t)),
        .is_dirty = false,
    };

    assert(chunk.blocks);
    assert(chunk.heightmap_min);
    assert(chunk.heightmap_max);

    for (size_t i = 0; i < heightmap_length; i++) {
        chunk.heightmap_min[i] = chunk_height;
    }

    for (size_t z = 0; z < chunk_size; z++) {
        for (size_t x = 0; x < chunk_size; x++) {
            for (size_t y = 0; y < chunk_height / 2; y++) {
                chunk_set_block(&chunk, x, y, z, 1);
            }
        }
    }

    return chunk;
}

void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block) {
    // TODO: Remove this bounds check if the same check is removed from chunk_get_block.
    if (x < 0 || x >= chunk_size || z < 0 || z >= chunk_size || y < 0 || y >= chunk_height) {
        return;
    }

    if (block != 0) {
        // TODO: Maybe make a heightmap_get_index function?
        int32_t *heightmap_current_min = chunk->heightmap_min + x + z * chunk_size;
        int32_t *heightmap_current_max = chunk->heightmap_max + x + z * chunk_size;

        if (y < *heightmap_current_min) {
            *heightmap_current_min = y;
        } else if (y > *heightmap_current_max) {
            *heightmap_current_max = y;
        }
    }

    size_t i = chunk_get_index(x, y, z);
    chunk->blocks[i] = block;
    chunk->is_dirty = true;
}

void chunk_destroy(struct Chunk *chunk) {
    free(chunk->blocks);
    free(chunk->heightmap_min);
    free(chunk->heightmap_max);
}

extern inline size_t chunk_get_index(int32_t x, int32_t y, int32_t z);
extern inline uint8_t chunk_get_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z);