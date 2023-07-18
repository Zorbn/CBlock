#ifndef WORLD_H
#define WORLD_H

#include "detect_leak.h"

#include "chunk.h"
#include "list.h"

#include <cglm/struct.h>

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern const size_t world_size;
extern const size_t world_length;
extern const size_t world_size_in_blocks;

struct LightAddNode {
    int32_t x;
    int32_t y;
    int32_t z;
};

typedef struct LightAddNode struct_LightAddNode;
LIST_DEFINE(struct_LightAddNode);

struct LightRemoveNode {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t light_level;
};

typedef struct LightRemoveNode struct_LightRemoveNode;
LIST_DEFINE(struct_LightRemoveNode);

enum LightEventType {
    LIGHT_EVENT_TYPE_ADD,
    LIGHT_EVENT_TYPE_REMOVE,
    LIGHT_EVENT_TYPE_UPDATE,
};

struct LightEventNode {
    enum LightEventType event_type;
    int32_t x;
    int32_t y;
    int32_t z;
};

typedef struct LightEventNode struct_LightEventNode;
LIST_DEFINE(struct_LightEventNode);

enum LightType {
    LIGHT_TYPE_LIGHT,
    LIGHT_TYPE_SUNLIGHT,
};

struct World {
    struct Chunk *chunks;
    struct List_struct_LightEventNode light_event_queue;
    struct List_struct_LightAddNode light_add_queue;
    struct List_struct_LightRemoveNode light_remove_queue;
    HANDLE mutex;
};

struct RaycastHit {
    uint8_t block;
    float distance;
    ivec3s position;
    ivec3s last_position;
};

#define CHUNK_INDEX(chunk_x, chunk_z) ((chunk_x) + (chunk_z)*world_size)

struct World world_create();
void world_light_add(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset, enum LightType light_type);
void world_light_remove(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset, enum LightType light_type);
void world_light_update(struct World *world, int32_t x, int32_t y, int32_t z);
struct RaycastHit world_raycast(struct World *world, vec3s start, vec3s direction, float range);
bool world_is_colliding_with_box(struct World *world, vec3s position, vec3s size, vec3s origin);
void world_set_block(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t block);
void world_destroy(struct World *world);

inline uint8_t world_get_block(struct World *world, int32_t x, int32_t y, int32_t z) {
    if (x < 0 || x >= world_size_in_blocks || z < 0 || z >= world_size_in_blocks || y < 0 || y >= chunk_height) {
        return 1;
    }

    int32_t chunk_x = x / CHUNK_SIZE;
    int32_t chunk_z = z / CHUNK_SIZE;

    int32_t chunk_i = CHUNK_INDEX(chunk_x, chunk_z);

    int32_t block_x = x % CHUNK_SIZE;
    int32_t block_y = y % chunk_height;
    int32_t block_z = z % CHUNK_SIZE;

    return chunk_get_block(&world->chunks[chunk_i], block_x, block_y, block_z);
}

inline void world_set_light_level(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t light_level, uint8_t mask, uint8_t offset) {
    if (x < 0 || x >= world_size_in_blocks || z < 0 || z >= world_size_in_blocks || y < 0 || y >= chunk_height) {
        return;
    }

    int32_t chunk_x = x / CHUNK_SIZE;
    int32_t chunk_z = z / CHUNK_SIZE;

    int32_t chunk_i = CHUNK_INDEX(chunk_x, chunk_z);

    int32_t block_x = x % CHUNK_SIZE;
    int32_t block_y = y % chunk_height;
    int32_t block_z = z % CHUNK_SIZE;

    chunk_set_light_level(&world->chunks[chunk_i], block_x, block_y, block_z, light_level, mask, offset);
}

inline uint8_t world_get_light_level(struct World *world, int32_t x, int32_t y, int32_t z, uint8_t mask, uint8_t offset) {
    if (y >= chunk_height) {
        // Pretend that above the map their is maximum sunlight and no other light.
        // sunlight_mask >> sunlight_offset = MAX_LIGHT_LEVEL, light_mask >> sunlight_offset = 0
        return mask >> sunlight_offset;
    }

    if (x < 0 || x >= world_size_in_blocks || z < 0 || z >= world_size_in_blocks || y < 0) {
        return 0;
    }

    int32_t chunk_x = x / CHUNK_SIZE;
    int32_t chunk_z = z / CHUNK_SIZE;

    int32_t chunk_i = CHUNK_INDEX(chunk_x, chunk_z);

    int32_t block_x = x % CHUNK_SIZE;
    int32_t block_y = y % chunk_height;
    int32_t block_z = z % CHUNK_SIZE;

    return chunk_get_light_level(&world->chunks[chunk_i], block_x, block_y, block_z, mask, offset);
}

#endif