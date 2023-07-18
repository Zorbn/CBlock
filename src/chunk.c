#include "chunk.h"

#include <stdlib.h>
#include <assert.h>

const size_t chunk_height = 256;
const size_t chunk_length = CHUNK_SIZE * CHUNK_SIZE * chunk_height;
const size_t heightmap_length = CHUNK_SIZE * CHUNK_SIZE;
const float inv_light_level_count = 1.0f / MAX_LIGHT_LEVEL;
const uint8_t light_mask = 0x0f;    // Lower 4 bits are for lighting.
const uint8_t sunlight_mask = 0xf0; // Upper 4 bits are for sunlighting.
const uint8_t light_offset = 0;
const uint8_t sunlight_offset = 4;

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
        chunk.heightmap_min[i] = chunk_height - 1;
    }

    size_t ground_height = chunk_height / 2;
    for (size_t z = 0; z < CHUNK_SIZE; z++) {
        for (size_t x = 0; x < CHUNK_SIZE; x++) {
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

#include <stdio.h>

void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block) {
    int32_t heightmap_i = HEIGHTMAP_INDEX(x, z);
    int32_t *heightmap_block_min = chunk->heightmap_min + heightmap_i;
    int32_t *heightmap_block_max = chunk->heightmap_max + heightmap_i;

    // TODO: Only use this logic after the map is generated, when creating a new chunk the heightmap should be created once the chunk has generated.
    if (block == 0) {
        int32_t iy;
        for (iy = chunk_height - 1; iy >= 0; iy--) {
            if (chunk_get_block(chunk, x, iy, z) != 0) {
                break;
            }
        }
        *heightmap_block_max = iy;

        for (iy = 0; iy < chunk_height; iy++) {
            if (chunk_get_block(chunk, x, iy, z) != 0) {
                break;
            }
        }
        *heightmap_block_min = iy;
    } else {
        if (y < *heightmap_block_min) {
            *heightmap_block_min = y;
        } else if (y > *heightmap_block_max) {
            *heightmap_block_max = y;
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
extern inline uint8_t chunk_get_light_level(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset);
extern inline void chunk_set_light_level(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t light_level, uint8_t mask, uint8_t offset);