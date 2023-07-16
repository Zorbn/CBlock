#include "mesher.h"

#include <cglm/struct.h>

const float cube_texture_size = 16.0f;

const vec3s cube_vertices[6][4] = {
    // Forward
    {
        {{0, 0, 0}},
        {{0, 1, 0}},
        {{1, 1, 0}},
        {{1, 0, 0}},
    },
    // Backward
    {
        {{0, 0, 1}},
        {{0, 1, 1}},
        {{1, 1, 1}},
        {{1, 0, 1}},
    },
    // Right
    {
        {{1, 0, 0}},
        {{1, 0, 1}},
        {{1, 1, 1}},
        {{1, 1, 0}},
    },
    // Left
    {
        {{0, 0, 0}},
        {{0, 0, 1}},
        {{0, 1, 1}},
        {{0, 1, 0}},
    },
    // Up
    {
        {{0, 1, 0}},
        {{0, 1, 1}},
        {{1, 1, 1}},
        {{1, 1, 0}},
    },
    // Down
    {
        {{0, 0, 0}},
        {{0, 0, 1}},
        {{1, 0, 1}},
        {{1, 0, 0}},
    },
};

const vec2s cube_uvs[6][4] = {
    // Forward
    {
        {{1, 1}},
        {{1, 0}},
        {{0, 0}},
        {{0, 1}},
    },
    // Backward
    {
        {{0, 1}},
        {{0, 0}},
        {{1, 0}},
        {{1, 1}},
    },
    // Right
    {
        {{1, 1}},
        {{0, 1}},
        {{0, 0}},
        {{1, 0}},
    },
    // Left
    {
        {{0, 1}},
        {{1, 1}},
        {{1, 0}},
        {{0, 0}},
    },
    // Up
    {
        {{0, 1}},
        {{0, 0}},
        {{1, 0}},
        {{1, 1}},
    },
    // Down
    {
        {{0, 1}},
        {{0, 0}},
        {{1, 0}},
        {{1, 1}},
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

const float cube_shades[6] = {
    0.950f, // Forward
    0.925f, // Backward
    0.975f, // Right
    0.900f, // Left
    1.000f, // Up
    0.875f, // Down
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

void mesher_mesh_chunk(struct Mesher *mesher, struct World *world, struct Chunk *chunk,
    int32_t texture_atlas_width, int32_t texture_atlas_height) {

    list_reset_float(&mesher->vertices);
    list_reset_uint32_t(&mesher->indices);

    for (int32_t z = 0; z < chunk_size; z++) {
        int32_t world_z = z + chunk->z;
        for (int32_t x = 0; x < chunk_size; x++) {
            int32_t world_x = x + chunk->x;
            int32_t heightmap_index = HEIGHTMAP_INDEX(x, z);
            int32_t y_min = chunk->heightmap_min[heightmap_index];
            int32_t y_max = chunk->heightmap_max[heightmap_index];
            for (int32_t y = y_min; y <= y_max; y++) {
                uint8_t block = world_get_block(world, world_x, y, world_z);
                // Don't include empty blocks in the mesh.
                if (block == 0) {
                    continue;
                }

                float block_texture_index = block - 1;

                // Finding the neighbors first is more cache efficient.
                uint8_t neighbors[6];
                for (size_t side_i = 0; side_i < 6; side_i++) {
                    int32_t neighbor_x = chunk->x + x + directions[side_i][0];
                    int32_t neighbor_y = y + directions[side_i][1];
                    int32_t neighbor_z = chunk->z + z + directions[side_i][2];
                    neighbors[side_i] = world_get_block(world, neighbor_x, neighbor_y, neighbor_z);
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

                    float side_shade = cube_shades[side_i];

                    for (size_t vertex_i = 0; vertex_i < 4; vertex_i++) {
                        // Position:
                        float vertex_x = chunk->x + x + cube_vertices[side_i][vertex_i].x;
                        float vertex_y = y + cube_vertices[side_i][vertex_i].y;
                        float vertex_z = chunk->z + z + cube_vertices[side_i][vertex_i].z;
                        list_push_float(&mesher->vertices, vertex_x);
                        list_push_float(&mesher->vertices, vertex_y);
                        list_push_float(&mesher->vertices, vertex_z);

                        // Color:
                        list_push_float(&mesher->vertices, side_shade);
                        list_push_float(&mesher->vertices, side_shade);
                        list_push_float(&mesher->vertices, side_shade);

                        // UV:
                        float u = cube_uvs[side_i][vertex_i].u;
                        float v = cube_uvs[side_i][vertex_i].v;
                        list_push_float(&mesher->vertices, u);
                        list_push_float(&mesher->vertices, v);
                        list_push_float(&mesher->vertices, block_texture_index);
                    }
                }
            }
        }
    }
}

void mesher_destroy(struct Mesher *mesher) {
    list_destroy_float(&mesher->vertices);
    list_destroy_uint32_t(&mesher->indices);
}