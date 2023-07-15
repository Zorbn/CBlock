#ifndef CHUNK_H
#define CHUNK_H

#include <inttypes.h>
#include <stdbool.h>

extern const size_t chunk_size;
extern const size_t chunk_height;
extern const size_t chunk_length;

// TODO: Keep track of heighest and lowest? block at each XZ position,
// use that info to reduce the amount of meshing required. (Helpful in the
// case of sky islands which may fill only a small section of a chunk, or
// chunks that contain nothing and are just empty space in most columns).
struct Chunk {
    uint8_t *blocks;
    uint32_t x;
    uint32_t z;
    int32_t *heightmap_min;
    int32_t *heightmap_max;
    bool is_dirty;
};

struct Chunk chunk_create(int32_t x, int32_t z);
void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block);
void chunk_destroy(struct Chunk *chunk);

inline size_t chunk_get_index(int32_t x, int32_t y, int32_t z) {
    return y + x * chunk_height + z * chunk_height * chunk_size;
}

inline uint8_t chunk_get_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z) {
    // TODO: If a world struct is created to abstract over the chunks, bounds checking could
    // be done once there rather than in every chunk.
    if (x < 0 || x >= chunk_size || z < 0 || z >= chunk_size || y < 0 || y >= chunk_height) {
        return 1;
    }

    size_t i = chunk_get_index(x, y, z);
    return chunk->blocks[i];
}

#endif