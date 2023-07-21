#ifndef MESHER_H
#define MESHER_H

#include "../detect_leak.h"

#include "mesh.h"
#include "../chunk.h"
#include "../world.h"
#include "../list.h"

struct Mesher {
    struct List_float vertices;
    struct List_uint32_t indices;
    int32_t processed_chunk_i;
};

struct Mesher mesher_create(void);
void mesher_mesh_chunk(struct Mesher *mesher, struct World *world, struct Chunk *chunk,
    int32_t texture_atlas_width, int32_t texture_atlas_height);
void mesher_destroy(struct Mesher *mesher);

#endif