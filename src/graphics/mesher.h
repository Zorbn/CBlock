#ifndef MESHER_H
#define MESHER_H

#include "mesh.h"
#include "../chunk.h"
#include "../list.h"

struct Mesher {
    struct List_float vertices;
    struct List_uint32_t indices;
};

struct Mesher mesher_create();
struct Mesh mesher_mesh_chunk(struct Mesher *mesher, struct Chunk *chunk);
void mesher_destroy(struct Mesher *mesher);

#endif