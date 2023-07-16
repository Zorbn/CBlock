#ifndef MESHER_H
#define MESHER_H

#include "mesh.h"
#include "../chunk.h"
#include "../list.h"

struct World;
uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z);

struct Mesher {
    struct List_float vertices;
    struct List_uint32_t indices;
};

struct Mesher mesher_create();
struct Mesh mesher_mesh_chunk(struct Mesher *mesher, struct World *world, struct Chunk *chunk,
    int32_t texture_atlas_width, int32_t texture_atlas_height);
void mesher_destroy(struct Mesher *mesher);

#endif