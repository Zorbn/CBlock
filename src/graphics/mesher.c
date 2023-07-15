#include "mesher.h"

#include <cglm/struct.h>

const vec3 cube_vertices[6][4] = {
    // Forward
    {
        {0, 0, 0},
        {0, 1, 0},
        {1, 1, 0},
        {1, 0, 0},
    },
    // Backward
    {
        {0, 0, 1},
        {0, 1, 1},
        {1, 1, 1},
        {1, 0, 1},
    },
    // Right
    {
        {1, 0, 0},
        {1, 0, 1},
        {1, 1, 1},
        {1, 1, 0},
    },
    // Left
    {
        {0, 0, 0},
        {0, 0, 1},
        {0, 1, 1},
        {0, 1, 0},
    },
    // Up
    {
        {0, 1, 0},
        {0, 1, 1},
        {1, 1, 1},
        {1, 1, 0},
    },
    // Down
    {
        {0, 0, 0},
        {0, 0, 1},
        {1, 0, 1},
        {1, 0, 0},
    },
};

const vec2 cube_uvs[6][4] = {
    // Forward
    {
        {1, 1},
        {1, 0},
        {0, 0},
        {0, 1},
    },
    // Backward
    {
        {0, 1},
        {0, 0},
        {1, 0},
        {1, 1},
    },
    // Right
    {
        {1, 1},
        {0, 1},
        {0, 0},
        {1, 0},
    },
    // Left
    {
        {0, 1},
        {1, 1},
        {1, 0},
        {0, 0},
    },
    // Up
    {
        {0, 1},
        {0, 0},
        {1, 0},
        {1, 1},
    },
    // Down
    {
        {0, 1},
        {0, 0},
        {1, 0},
        {1, 1},
    },
};

const uint16_t cube_indices[6][6] = {
    {0, 1, 2, 0, 2, 3}, // Forward
    {0, 2, 1, 0, 3, 2}, // Backward
    {0, 2, 1, 0, 3, 2}, // Right
    {0, 1, 2, 0, 2, 3}, // Left
    {0, 1, 2, 0, 2, 3}, // Up
    {0, 2, 1, 0, 3, 2}, // Down
};

const int32_t directions[6][3] = {
    {0, 0, -1}, // Forward
    {0, 0, 1},  // Backward
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Up
    {0, -1, 0}, // Down
};

struct Mesher mesher_create() {
    return (struct Mesher){
        .vertices = list_create_float(4096),
        .indices = list_create_uint32_t(4096),
    };
}

struct Mesh mesher_mesh_chunk(struct Mesher *mesher, struct Chunk *chunk) {
    const size_t vertex_component_count = 8;

    list_reset_float(&mesher->vertices);
    list_reset_uint32_t(&mesher->indices);

    for (int32_t z = 0; z < chunk_size; z++) {
        for (int32_t x = 0; x < chunk_size; x++) {
            int32_t y_min = chunk->heightmap_min[x + z * chunk_size];
            int32_t y_max = chunk->heightmap_max[x + z * chunk_size];
            for (int32_t y = y_min; y <= y_max; y++) {
                uint8_t block = chunk_get_block(chunk, x, y, z);
                // Don't include empty blocks in the mesh.
                if (block == 0) {
                    continue;
                }

                // Finding the neighbors first improves the cache efficiency slightly.
                uint8_t neighbors[6];
                for (size_t side_i = 0; side_i < 6; side_i++) {
                    int32_t neighbor_x = x + directions[side_i][0];
                    int32_t neighbor_y = y + directions[side_i][1];
                    int32_t neighbor_z = z + directions[side_i][2];
                    neighbors[side_i] = chunk_get_block(chunk, neighbor_x, neighbor_y, neighbor_z);
                }

                for (size_t side_i = 0; side_i < 6; side_i++) {
                    // Skip faces that are covered by a neighboring block.
                    if (neighbors[side_i] != 0) {
                        continue;
                    }

                    uint32_t vertex_count = mesher->vertices.length / vertex_component_count;

                    for (size_t index_i = 0; index_i < 6; index_i++) {
                        uint32_t index = vertex_count + cube_indices[side_i][index_i];
                        list_push_uint32_t(&mesher->indices, index);
                    }

                    for (size_t vertex_i = 0; vertex_i < 4; vertex_i++) {
                        // Position:
                        float vertex_x = chunk->x + x + cube_vertices[side_i][vertex_i][0];
                        float vertex_y = y + cube_vertices[side_i][vertex_i][1];
                        float vertex_z = chunk->z + z + cube_vertices[side_i][vertex_i][2];
                        list_push_float(&mesher->vertices, vertex_x);
                        list_push_float(&mesher->vertices, vertex_y);
                        list_push_float(&mesher->vertices, vertex_z);

                        // Color:
                        list_push_float(&mesher->vertices, 1.0f);
                        list_push_float(&mesher->vertices, 1.0f);
                        list_push_float(&mesher->vertices, 1.0f);

                        // UV:
                        list_push_float(&mesher->vertices, cube_uvs[side_i][vertex_i][0]);
                        list_push_float(&mesher->vertices, cube_uvs[side_i][vertex_i][1]);
                    }
                }
            }
        }
    }

    return mesh_create(mesher->vertices.data, mesher->vertices.length / vertex_component_count, mesher->indices.data,
        mesher->indices.length);
}

void mesher_destroy(struct Mesher *mesher) {
    list_destroy_float(&mesher->vertices);
    list_destroy_uint32_t(&mesher->indices);
}