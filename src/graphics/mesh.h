#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <inttypes.h>

struct Mesh {
    uint32_t vbo;
    uint32_t vao;
    uint32_t ebo;
    uint32_t index_count;
};

struct Mesh mesh_create(float *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count);
void mesh_draw(struct Mesh *mesh);

#endif