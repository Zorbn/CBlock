#ifndef CHUNK_H
#define CHUNK_H

#include <inttypes.h>

extern const size_t chunk_size;
extern const size_t chunk_height;

// TODO: Keep track of heighest and lowest? block at each XZ position,
// use that info to reduce the amount of meshing required. (Helpful in the
// case of sky islands which may fill only a small section of a chunk, or
// chunks that contain nothing and are just empty space in most columns).
struct Chunk {
    uint8_t *blocks;
};

struct Chunk chunk_create();
size_t chunk_get_index(int32_t x, int32_t y, int32_t z);
uint8_t chunk_get_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z);
void chunk_set_block(struct Chunk *chunk, int32_t x, int32_t y, int32_t z, uint8_t block);
void chunk_destroy(struct Chunk *chunk);

#endif