#include "world.h"
#include "directions.h"

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

// TODO: block "3" is currently a stand-in for all lights.
#define LIGHT_BLOCK 3

const size_t world_size = 4;
const size_t world_length = world_size * world_size;
const size_t world_size_in_blocks = world_size * CHUNK_SIZE;

struct World world_create() {
    struct World world = (struct World){
        .chunks = malloc(world_length * sizeof(struct Chunk)),
        .lighting_updates = list_create_struct_LightingUpdate(128),
        .mutex = CreateMutex(NULL, FALSE, NULL),
    };

    assert(world.chunks);
    assert(world.mutex);

    for (size_t i = 0; i < world_length; i++) {
        int32_t chunk_x = (i % world_size) * CHUNK_SIZE;
        int32_t chunk_z = i / world_size * CHUNK_SIZE;
        world.chunks[i] = chunk_create(chunk_x, chunk_z);
    }

    return world;
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

// TODO: Add to header, call from other thread instead of main, etc.
// Based on xtreme8000's CavEX lighting algorithm.
void world_update_lighting(struct World *world, int32_t source_x, int32_t source_y, int32_t source_z) {
    list_push_struct_LightingUpdate(&world->lighting_updates, (struct LightingUpdate){
                                                                  .x = source_x,
                                                                  .y = source_y,
                                                                  .z = source_z,
                                                              });

    while (world->lighting_updates.length > 0) {
        struct LightingUpdate current = list_pop_struct_LightingUpdate(&world->lighting_updates);

        size_t chunk_i = CHUNK_INDEX(current.x / CHUNK_SIZE, current.z / CHUNK_SIZE);
        size_t heightmap_i = HEIGHTMAP_INDEX(current.x % CHUNK_SIZE, current.z % CHUNK_SIZE);
        int32_t heightmap_max = world->chunks[chunk_i].heightmap_max[heightmap_i];

        uint8_t block = world_get_block(world, current.x, current.y, current.z);

        uint8_t old_sunlight =
            world_get_light_level(world, current.x, current.y, current.z, sunlight_mask, sunlight_offset);
        uint8_t old_light = world_get_light_level(world, current.x, current.y, current.z, light_mask, light_offset);

        uint8_t new_sunlight = 0;
        uint8_t new_light = 0;

        if (current.y >= heightmap_max + 1) {
            new_sunlight = MAX_LIGHT_LEVEL;
        }

        if (block == LIGHT_BLOCK) {
            new_light = MAX_LIGHT_LEVEL;
        }

        // Handle transparent blocks.
        if (block == 0) {
            for (size_t side_i = 0; side_i < 6; side_i++) {
                int32_t neighbor_x = current.x + directions[side_i].x;
                int32_t neighbor_y = current.y + directions[side_i].y;
                int32_t neighbor_z = current.z + directions[side_i].z;

                if (neighbor_x >= 0 && neighbor_x < world_size_in_blocks && neighbor_y >= 0 &&
                    neighbor_y < chunk_height && neighbor_z >= 0 && neighbor_z < world_size_in_blocks) {
                    uint8_t neighbor_sunlight_level = world_get_light_level(
                        world, neighbor_x, neighbor_y, neighbor_z, sunlight_mask, sunlight_offset);
                    new_sunlight = GLM_MAX(GLM_MAX(neighbor_sunlight_level - 1, 0), new_sunlight);
                    uint8_t neighbor_light_level =
                        world_get_light_level(world, neighbor_x, neighbor_y, neighbor_z, light_mask, light_offset);
                    new_light = GLM_MAX(GLM_MAX(neighbor_light_level - 1, 0), new_light);
                }
            }
        }

        bool is_light_different = (old_light != new_light) || (old_sunlight != new_sunlight);
        if (is_light_different) {
            world_set_light_level(world, current.x, current.y, current.z, new_sunlight, sunlight_mask, sunlight_offset);
            world_set_light_level(world, current.x, current.y, current.z, new_light, light_mask, light_offset);

            // TODO: If smooth lighting gets added, neighboring chunks need to be updated if this lighting update is on
            // a chunk boundary. (ie: the same as when setting a block).
            world->chunks[chunk_i].is_dirty = true;

            for (size_t side_i = 0; side_i < 6; side_i++) {
                int32_t neighbor_x = current.x + directions[side_i].x;
                int32_t neighbor_y = current.y + directions[side_i].y;
                int32_t neighbor_z = current.z + directions[side_i].z;

                if (neighbor_x >= 0 && neighbor_x < world_size_in_blocks && neighbor_y >= 0 &&
                    neighbor_y < chunk_height && neighbor_z >= 0 && neighbor_z < world_size_in_blocks) {

                    list_push_struct_LightingUpdate(&world->lighting_updates, (struct LightingUpdate){
                                                                                  .x = neighbor_x,
                                                                                  .y = neighbor_y,
                                                                                  .z = neighbor_z,
                                                                              });
                }
            }
        }
    }
}

void world_set_block(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t block) {
    if (x < 0 || x >= world_size_in_blocks || z < 0 || z >= world_size_in_blocks || y < 0 || y >= chunk_height) {
        return;
    }

    WaitForSingleObject(world->mutex, INFINITE);

    int32_t chunk_x = x / CHUNK_SIZE;
    int32_t chunk_z = z / CHUNK_SIZE;

    int32_t chunk_i = CHUNK_INDEX(chunk_x, chunk_z);

    int32_t block_x = x % CHUNK_SIZE;
    int32_t block_y = y % chunk_height;
    int32_t block_z = z % CHUNK_SIZE;

    chunk_set_block(&world->chunks[chunk_i], block_x, block_y, block_z, block);
    world_update_lighting(world, x, y, z);

    if (block_x == 0 && chunk_x > 0) {
        world->chunks[CHUNK_INDEX(chunk_x - 1, chunk_z)].is_dirty = true;
    }

    if (block_x == CHUNK_SIZE - 1 && chunk_x < world_size - 1) {
        world->chunks[CHUNK_INDEX(chunk_x + 1, chunk_z)].is_dirty = true;
    }

    if (block_z == 0 && chunk_z > 0) {
        world->chunks[CHUNK_INDEX(chunk_x, chunk_z - 1)].is_dirty = true;
    }

    if (block_z == CHUNK_SIZE - 1 && chunk_z < world_size - 1) {
        world->chunks[CHUNK_INDEX(chunk_x, chunk_z + 1)].is_dirty = true;
    }

    ReleaseMutex(world->mutex);
}

void world_destroy(struct World *world) {
    CloseHandle(world->mutex);

    for (size_t i = 0; i < world_length; i++) {
        chunk_destroy(&world->chunks[i]);
    }

    list_destroy_struct_LightingUpdate(&world->lighting_updates);

    free(world->chunks);
}

extern inline uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z);
extern inline void world_set_light_level(
    struct World *world, int32_t x, int32_t y, int32_t z, uint8_t light_level, uint8_t mask, uint8_t offset);
extern inline uint8_t world_get_light_level(
    struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset);