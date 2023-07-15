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
void world_mesh_chunks(struct World *world);
void world_draw(struct World *world);
struct RaycastHit world_raycast(struct World *world, vec3s start, vec3s direction, float range);

// TODO: Can these be inlined somehow?
void world_set_block(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t block);
uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z);

void world_destroy(struct World *world);

#endif