#ifndef MESHER_H
#define MESHER_H

#include "../detect_leak.h"

#include "mesh.h"
#include "../chunk.h"
#include "../list.h"

// TODO: Remove mesher from world, replace with mesher_mesh_world, making these foward declarations unnecessary.
struct World;
uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z);

struct Mesher {
    struct List_float vertices;
    struct List_uint32_t indices;
};

struct Mesher mesher_create();
void mesher_mesh_chunk(struct Mesher *mesher, struct World *world, struct Chunk *chunk,
    int32_t texture_atlas_width, int32_t texture_atlas_height);
void mesher_destroy(struct Mesher *mesher);

#endif