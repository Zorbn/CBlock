#ifndef MESHING_INFO_H
#define MESHING_INFO_H

#include "../chunk.h"
#include "../world.h"
#include "../list.h"
#include "mesher.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdatomic.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct MeshingInfo {
    struct World *world;
    struct Mesh *meshes;
    struct Mesher *meshers;
    uint8_t *light_level_cache;
    _Atomic(bool) is_done;
    int32_t texture_atlas_width;
    int32_t texture_atlas_height;
};

DWORD WINAPI meshing_thread_start(void *start_info);
struct MeshingInfo meshing_info_create(struct World *world, int32_t texture_atlas_width, int32_t texture_atlas_height);
void meshing_info_upload(struct MeshingInfo *info);
void meshing_info_draw(struct MeshingInfo *info);
void meshing_info_cache_light_levels(struct MeshingInfo *info, struct LightEventNode *light_event_node);
void meshing_info_mark_dirty_chunks(struct MeshingInfo *info, struct LightEventNode *light_event_node);
void meshing_info_destroy(struct MeshingInfo *info);

#endif