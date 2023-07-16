#include "world.h"

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

const size_t world_size = 4;
const size_t world_length = world_size * world_size;

struct World world_create() {
    struct World world = (struct World){
        .chunks = malloc(world_length * sizeof(struct Chunk)),
        .meshes = malloc(world_length * sizeof(struct Mesh)),
        .mesher = mesher_create(),
    };

    assert(world.chunks);
    assert(world.meshes);

    for (size_t i = 0; i < world_length; i++) {
        world.chunks[i] = chunk_create((i % world_size) * chunk_size, i / world_size * chunk_size);
    }

    return world;
}

void world_draw(struct World *world) {
    for (size_t i = 0; i < world_length; i++) {
        mesh_draw(&world->meshes[i]);
    }
}

// Uses DDA Voxel traversal to find the first voxel hit by the ray.
struct RaycastHit world_raycast(struct World *world, vec3s start, vec3s direction, float range) {
    direction = glms_vec3_normalize(direction);

    // Add a small bias to prevent landing perfectly on block boundaries,
    // which would cause rays to be inaccurate with this algorithm.
    start.x += 1e-4;
    start.y += 1e-4;
    start.z += 1e-4;

    vec3s tile_direction = glms_vec3_sign(direction);
    vec3s step = glms_vec3_abs((vec3s){{1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z}});
    vec3s initial_step;

    if (direction.x > 0.0f) {
        initial_step.x = (ceilf(start.x) - start.x) * step.x;
    } else {
        initial_step.x = (start.x - floorf(start.x)) * step.x;
    }

    if (direction.y > 0.0f) {
        initial_step.y = (ceilf(start.y) - start.y) * step.y;
    } else {
        initial_step.y = (start.y - floorf(start.y)) * step.y;
    }

    if (direction.z > 0.0f) {
        initial_step.z = (ceilf(start.z) - start.z) * step.z;
    } else {
        initial_step.z = (start.z - floorf(start.z)) * step.z;
    }

    vec3s distance_to_next = initial_step;
    ivec3s block_position = (ivec3s){{floorf(start.x), floorf(start.y), floorf(start.z)}};
    ivec3s last_block_position = block_position;

    float last_distance_to_next = 0.0f;

    uint8_t hit_block = world_get_block(world, block_position.x, block_position.y, block_position.z);
    while (hit_block == 0 && last_distance_to_next < range) {
        last_block_position = block_position;

        if (distance_to_next.x < distance_to_next.y && distance_to_next.x < distance_to_next.z) {
            last_distance_to_next = distance_to_next.x;
            distance_to_next.x += step.x;
            block_position.x += (int32_t)tile_direction.x;
        } else if (distance_to_next.y < distance_to_next.x && distance_to_next.y < distance_to_next.z) {
            last_distance_to_next = distance_to_next.y;
            distance_to_next.y += step.y;
            block_position.y += (int32_t)tile_direction.y;
        } else {
            last_distance_to_next = distance_to_next.z;
            distance_to_next.z += step.z;
            block_position.z += (int32_t)tile_direction.z;
        }

        hit_block = world_get_block(world, block_position.x, block_position.y, block_position.z);
    }

    return (struct RaycastHit){
        .block = hit_block,
        .distance = last_distance_to_next,
        .position = block_position,
        .last_position = last_block_position,
    };
}

bool world_is_colliding_with_box(struct World *world, vec3s position, vec3s size, vec3s origin) {
    vec3s half_size = glms_vec3_scale(size, 0.5f);
    vec3s min_position = glms_vec3_sub(position, half_size);
    min_position = glms_vec3_sub(min_position, origin);
    vec3s max_position = glms_vec3_add(position, half_size);
    max_position = glms_vec3_sub(max_position, origin);

    ivec3s steps = (ivec3s){
        .x = (int32_t)floorf(size.x) + 1,
        .y = (int32_t)floorf(size.y) + 1,
        .z = (int32_t)floorf(size.z) + 1,
    };
    vec3s interpolated_position;

    // Interpolate between the minimum and maximum positions, checking each block between them.
    for (int32_t z = 0; z <= steps.z; z++) {
        interpolated_position.z = min_position.z + (float)z / steps.z * size.z;
        for (int32_t x = 0; x <= steps.x; x++) {
            interpolated_position.x = min_position.x + (float)x / steps.x * size.x;
            for (int32_t y = 0; y <= steps.y; y++) {
                interpolated_position.y = min_position.y + (float)y / steps.y * size.y;

                ivec3s interpolated_block_position = (ivec3s){
                    .x = (int32_t)floorf(interpolated_position.x),
                    .y = (int32_t)floorf(interpolated_position.y),
                    .z = (int32_t)floorf(interpolated_position.z),
                };

                if (world_get_block(world, interpolated_block_position.x, interpolated_block_position.y,
                        interpolated_block_position.z) != 0) {

                    return true;
                }
            }
        }
    }

    return false;
}

void world_destroy(struct World *world) {
    for (size_t i = 0; i < world_length; i++) {
        chunk_destroy(&world->chunks[i]);
        mesh_destroy(&world->meshes[i]);
    }

    mesher_destroy(&world->mesher);

    free(world->chunks);
    free(world->meshes);
}

extern inline void world_set_block(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t block);
extern inline uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z);