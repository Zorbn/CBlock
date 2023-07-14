#include "chunk.h"

#include <stdlib.h>
#include <assert.h>

const size_t chunk_size = 16;
const size_t chunk_height = 32;
const size_t chunk_length = chunk_size * chunk_size * chunk_height;

struct Chunk chunk_create() {
    struct Chunk chunk = (struct Chunk){
        .blocks = malloc(chunk_length * sizeof(uint8_t)),
    };

    assert(chunk.blocks);

    for (size_t i = 0; i < chunk_length; i++) {
        chunk.blocks[i] = (rand() % 100) > 80 ? 1 : 0;
    }

    return chunk;
}

// TODO: Look into inlining these functions.
size_t chunk_get_index(int32_t x, int32_t y, int32_t z) {
    return x + z * chunk_size + y * chunk_size * chunk_size;
}

uint8_t chunk_get_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z) {
    // TODO: If a world struct is created to abstract over the chunks, bounds checking could
    // be done once there rather than in every chunk.
    if (x < 0 || x >= chunk_size || z < 0 || z >= chunk_size || y < 0 || y >= chunk_height) {
        return 0;
    }

    size_t i = chunk_get_index(x, y, z);
    return chunk->blocks[i];
}

void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block) {
    // TODO: Remove this bounds check if the same check is removed from chunk_get_block.
    if (x < 0 || x >= chunk_size || z < 0 || z >= chunk_size || y < 0 || y >= chunk_height) {
        return;
    }
    
    size_t i = chunk_get_index(x, y, z);
    chunk->blocks[i] = block;
}

void chunk_destroy(struct Chunk *chunk) {
    free(chunk->blocks);
}