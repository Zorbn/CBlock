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
        .light_event_queue = list_create_struct_LightEventNode(16),
        .light_add_queue = list_create_struct_LightAddNode(64),
        .light_remove_queue = list_create_struct_LightRemoveNode(64),
        .mutex = CreateMutex(NULL, FALSE, NULL),
    };

    assert(world.chunks);
    assert(world.mutex);

    for (size_t i = 0; i < world_length; i++) {
        int32_t chunk_x = (i % world_size) * CHUNK_SIZE;
        int32_t chunk_z = i / world_size * CHUNK_SIZE;
        world.chunks[i] = chunk_create(chunk_x, chunk_z);
    }

    // TODO: This shouldnt be called manually on the main thread for every chunk
    for (size_t i = 0; i < world_length; i++) {
        int32_t chunk_x = (i % world_size) * CHUNK_SIZE;
        int32_t chunk_z = i / world_size * CHUNK_SIZE;

        for (int32_t z = 0; z < CHUNK_SIZE; z++) {
            int32_t world_z = z + chunk_z;
            for (int32_t x = 0; x < CHUNK_SIZE; x++) {
                int32_t world_x = x + chunk_x;

                size_t heightmap_i = HEIGHTMAP_INDEX(x, z);
                int32_t heightmap_max = world.chunks[i].heightmap_max[heightmap_i];
                int32_t sky_y = heightmap_max + 1;
                world_light_add(&world, world_x, sky_y, world_z, sunlight_mask, sunlight_offset, LIGHT_TYPE_SUNLIGHT);
            }
        }
    }

    return world;
}

// TODO: Sunlight currently always propogates down without reducing it's light_level even after hitting a block.

// Based on the Seed Of Andromeda lighting algorithm.
void world_light_add(
    struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset, enum LightType light_type) {
    world_set_light_level(world, x, y, z, MAX_LIGHT_LEVEL, mask, offset);

    list_push_struct_LightAddNode(&world->light_add_queue, (struct LightAddNode){
                                                               .x = x,
                                                               .y = y,
                                                               .z = z,
                                                           });
    while (world->light_add_queue.length > 0) {
        struct LightAddNode light_add_node = list_dequeue_struct_LightAddNode(&world->light_add_queue);

        uint8_t light_level =
            world_get_light_level(world, light_add_node.x, light_add_node.y, light_add_node.z, mask, offset);

        // Sunlight needs to propogate down until it hits the ground, so iterate sides so that
        // down is first, which will speed up sunlight lighting updates.
        for (int32_t side_i = down; side_i >= 0; side_i--) {
            int32_t neighbor_x = light_add_node.x + directions[side_i].x;
            int32_t neighbor_y = light_add_node.y + directions[side_i].y;
            int32_t neighbor_z = light_add_node.z + directions[side_i].z;
            uint8_t neighbor_block = world_get_block(world, neighbor_x, neighbor_y, neighbor_z);
            uint8_t neighbor_light_level =
                world_get_light_level(world, neighbor_x, neighbor_y, neighbor_z, mask, offset);

            bool sunlight_down = light_type == LIGHT_TYPE_SUNLIGHT && side_i == down;
            bool should_continue =
                (sunlight_down && neighbor_light_level < light_level) || (neighbor_light_level + 1 < light_level);
            if ((neighbor_block == 0 || neighbor_block == LIGHT_BLOCK) && should_continue) {
                uint8_t new_light_level = sunlight_down ? light_level : light_level - 1;
                world_set_light_level(world, neighbor_x, neighbor_y, neighbor_z, new_light_level, mask, offset);
                list_push_struct_LightAddNode(&world->light_add_queue, (struct LightAddNode){
                                                                           .x = neighbor_x,
                                                                           .y = neighbor_y,
                                                                           .z = neighbor_z,
                                                                       });
            }
        }
    }
}

void world_light_remove(
    struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset, enum LightType light_type) {
    uint8_t previous_light_level = world_get_light_level(world, x, y, z, mask, offset);
    list_push_struct_LightRemoveNode(&world->light_remove_queue, (struct LightRemoveNode){
                                                                     .x = x,
                                                                     .y = y,
                                                                     .z = z,
                                                                     .light_level = previous_light_level,
                                                                 });

    world_set_light_level(world, x, y, z, 0, mask, offset);

    while (world->light_remove_queue.length > 0) {
        struct LightRemoveNode light_remove_node = list_dequeue_struct_LightRemoveNode(&world->light_remove_queue);

        for (int32_t side_i = down; side_i >= 0; side_i--) {
            int32_t neighbor_x = light_remove_node.x + directions[side_i].x;
            int32_t neighbor_y = light_remove_node.y + directions[side_i].y;
            int32_t neighbor_z = light_remove_node.z + directions[side_i].z;
            uint8_t neighbor_light_level =
                world_get_light_level(world, neighbor_x, neighbor_y, neighbor_z, mask, offset);

            bool sunlight_down = light_type == LIGHT_TYPE_SUNLIGHT && side_i == down;
            bool should_continue = neighbor_light_level < light_remove_node.light_level || sunlight_down;
            if (neighbor_light_level != 0 && should_continue) {
                // Get rid of outdated light values.
                world_set_light_level(world, neighbor_x, neighbor_y, neighbor_z, 0, mask, offset);
                list_push_struct_LightRemoveNode(&world->light_remove_queue, (struct LightRemoveNode){
                                                                                 .x = neighbor_x,
                                                                                 .y = neighbor_y,
                                                                                 .z = neighbor_z,
                                                                                 .light_level = neighbor_light_level,
                                                                             });
            } else if (neighbor_light_level >= light_remove_node.light_level) {
                // Repopulate the light map based on valid neighboring light values.
                list_push_struct_LightAddNode(&world->light_add_queue, (struct LightAddNode){
                                                                           .x = neighbor_x,
                                                                           .y = neighbor_y,
                                                                           .z = neighbor_z,
                                                                       });
            }
        }
    }
}

void world_light_update(struct World *world, int32_t x, int32_t y, int32_t z) {
    // TODO: Building towards chunk borders above ground sometimes creates shadow-line artifacts along the border. It's
    // fixed by initially lighting chunks from the of the chunk, rather than the top of their heightmap, but I think
    // there's a better fix.

    for (int32_t iz = z - MAX_LIGHT_LEVEL; iz <= z + MAX_LIGHT_LEVEL; iz++) {
        for (int32_t ix = x - MAX_LIGHT_LEVEL; ix <= x + MAX_LIGHT_LEVEL; ix++) {
            if (ix < 0 || ix >= world_size_in_blocks || iz < 0 || iz >= world_size_in_blocks) {
                continue;
            }

            // TODO: Make a chunk inline function for getting heightmap values.
            size_t chunk_i = CHUNK_INDEX(ix / CHUNK_SIZE, iz / CHUNK_SIZE);
            size_t heightmap_i = HEIGHTMAP_INDEX(ix % CHUNK_SIZE, iz % CHUNK_SIZE);
            int32_t heightmap_max = world->chunks[chunk_i].heightmap_max[heightmap_i];

            // Sunlight conceptually emits from the sky, which is at the top of the map, but starting up there
            // is pointless if there are no blocks around to light. Instead start at the top of the heightmap
            // for this xz position, where the sky and the ground meet. However, this will cause problems when
            // the heightmap for a neighboring xz position is below the y level of this lighting update, since
            // starting at the heightmap height will only cast light below the updated location, missing it.
            // In that case, start casting light from the current y position, which is in the sky if it is
            // greater than the heightmap max, while also being low enough to not waste resources on lighting
            // empty space. Move up by one block to make sure we start in the sky, since heightmap_max is the location
            // of the highest block, and if this update is due to placing a block then y will also be inside the ground.
            // TODO: Placing a block at the maximum y level casts shadows strangely.
            int32_t sun_y = GLM_MAX(y, heightmap_max) + 1;

            world_light_remove(world, ix, sun_y, iz, sunlight_mask, sunlight_offset, LIGHT_TYPE_SUNLIGHT);
            world_light_add(world, ix, sun_y, iz, sunlight_mask, sunlight_offset, LIGHT_TYPE_SUNLIGHT);

            for (int32_t iy = y - MAX_LIGHT_LEVEL; iy <= y + MAX_LIGHT_LEVEL; iy++) {
                if (world_get_block(world, ix, iy, iz) == LIGHT_BLOCK) {
                    world_light_remove(world, ix, iy, iz, light_mask, light_offset, LIGHT_TYPE_LIGHT);
                    world_light_add(world, ix, iy, iz, light_mask, light_offset, LIGHT_TYPE_LIGHT);
                }
            }
        }
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

    // If the old block was a light block then it's light needs to be removed.
    if (world_get_block(world, x, y, z) == LIGHT_BLOCK) {
        list_push_struct_LightEventNode(&world->light_event_queue, (struct LightEventNode){
                                                                       .x = x,
                                                                       .y = y,
                                                                       .z = z,
                                                                       .event_type = LIGHT_EVENT_TYPE_REMOVE,
                                                                   });
    }

    chunk_set_block(&world->chunks[chunk_i], block_x, block_y, block_z, block);
    list_push_struct_LightEventNode(&world->light_event_queue, (struct LightEventNode){
                                                                   .x = x,
                                                                   .y = y,
                                                                   .z = z,
                                                                   .event_type = LIGHT_EVENT_TYPE_UPDATE,
                                                               });

    // If the old block is a light block then it's light needs to be added.
    if (block == LIGHT_BLOCK) {
        list_push_struct_LightEventNode(&world->light_event_queue, (struct LightEventNode){
                                                                       .x = x,
                                                                       .y = y,
                                                                       .z = z,
                                                                       .event_type = LIGHT_EVENT_TYPE_ADD,
                                                                   });
    }

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

    list_destroy_struct_LightEventNode(&world->light_event_queue);
    list_destroy_struct_LightAddNode(&world->light_add_queue);
    list_destroy_struct_LightRemoveNode(&world->light_remove_queue);

    free(world->chunks);
}

extern inline uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z);
extern inline void world_set_light_level(
    struct World *world, int32_t x, int32_t y, int32_t z, uint8_t light_level, uint8_t mask, uint8_t offset);
extern inline uint8_t world_get_light_level(
    struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset);