#ifndef WORLD_H
#define WORLD_H

#include "chunk.h"
#include "graphics/mesh.h"
#include "graphics/mesher.h"

#include <cglm/struct.h>

extern const size_t world_size;
extern const size_t world_size_in_blocks;
extern const size_t world_length;

struct World {
    struct Chunk *chunks;
    struct Mesh *meshes;
    struct Mesher mesher;
};

struct RaycastHit {
    uint8_t block;
    float distance;
    ivec3s position;
    ivec3s last_position;
};

struct World world_create();
void world_mesh_chunks(struct World *world, int32_t texture_atlas_width, int32_t texture_atlas_height);
void world_draw(struct World *world);
struct RaycastHit world_raycast(struct World *world, vec3s start, vec3s direction, float range);
void world_destroy(struct World *world);

inline void world_set_block(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t block) {
    const size_t world_size_in_blocks = world_size * chunk_size;
    if (x < 0 || x >= world_size_in_blocks || z < 0 || z >= world_size_in_blocks || y < 0 || y >= chunk_height) {
        return;
    }

    int32_t chunk_x = x / chunk_size;
    int32_t chunk_z = z / chunk_size;

    int32_t chunk_i = chunk_x + chunk_z * world_size;

    int32_t block_x = x % chunk_size;
    int32_t block_y = y % chunk_height;
    int32_t block_z = z % chunk_size;

    chunk_set_block(&world->chunks[chunk_i], block_x, block_y, block_z, block);
}

inline uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z) {
    const size_t world_size_in_blocks = world_size * chunk_size;
    if (x < 0 || x >= world_size_in_blocks || z < 0 || z >= world_size_in_blocks || y < 0 || y >= chunk_height) {
        return 0;
    }

    int32_t chunk_x = x / chunk_size;
    int32_t chunk_z = z / chunk_size;

    int32_t chunk_i = chunk_x + chunk_z * world_size;

    int32_t block_x = x % chunk_size;
    int32_t block_y = y % chunk_height;
    int32_t block_z = z % chunk_size;

    return chunk_get_block(&world->chunks[chunk_i], block_x, block_y, block_z);
}

#endif