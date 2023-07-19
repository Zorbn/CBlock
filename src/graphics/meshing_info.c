#include "meshing_info.h"

// 6 seems like the maximum reasonable number of chunks updates, ie: from placing a light that then lights several
// neighboring chunks. With 6 meshers that entire update could be processed in a single batch.
const size_t mesher_count = 6;
const size_t light_update_size = MAX_LIGHT_LEVEL * 2 + 1;

#define LIGHT_LEVEL_CACHE_INDEX(x, y, z) ((y) + (x)*chunk_height + (z)*chunk_height * light_update_size)

DWORD WINAPI meshing_thread_start(void *start_info) {
    struct MeshingInfo *info = start_info;
    while (!info->is_done) {
        WaitForSingleObject(info->world->mutex, INFINITE);

        world_update_lighting(info->world);

        // Process meshing updates:
        for (int32_t i = 0; i < world_length; i++) {
            if (!info->world->chunks[i].is_dirty) {
                continue;
            }

            size_t available_mesher_i = -1;

            for (size_t mesher_i = 0; mesher_i < mesher_count; mesher_i++) {
                if (info->meshers[mesher_i].processed_chunk_i == -1) {
                    available_mesher_i = mesher_i;
                    break;
                }
            }

            // No meshers are available right now.
            if (available_mesher_i == -1) {
                break;
            }

            info->world->chunks[i].is_dirty = false;

            mesher_mesh_chunk(&info->meshers[available_mesher_i], info->world, &info->world->chunks[i],
                info->texture_atlas_width, info->texture_atlas_height);
            info->meshers[available_mesher_i].processed_chunk_i = i;
        }

        ReleaseMutex(info->world->mutex);

        Sleep(0);
    }

    return 0;
}

struct MeshingInfo meshing_info_create(struct World *world, int32_t texture_atlas_width, int32_t texture_atlas_height) {
    const size_t light_update_length = light_update_size * light_update_size * chunk_height;

    struct MeshingInfo info = (struct MeshingInfo){
        .world = world,
        .meshes = calloc(world_length, sizeof(struct Mesh)),
        .meshers = malloc(mesher_count * sizeof(struct Mesher)),
        .light_level_cache = malloc(light_update_length * sizeof(uint8_t)),
        .is_done = false,
        .texture_atlas_width = texture_atlas_width,
        .texture_atlas_height = texture_atlas_height,
    };

    assert(info.meshes);
    assert(info.meshers);
    assert(info.light_level_cache);

    for (size_t i = 0; i < mesher_count; i++) {
        info.meshers[i] = mesher_create();
    }

    return info;
}

void meshing_info_upload(struct MeshingInfo *info) {
    // Try to lock the world mutex.
    DWORD wait_result = WaitForSingleObject(info->world->mutex, 0);
    if (wait_result != WAIT_OBJECT_0) {
        return;
    }

    size_t upload_count = 0;
    for (size_t i = 0; i < mesher_count; i++) {
        int32_t processed_chunk_i = info->meshers[i].processed_chunk_i;
        if (processed_chunk_i != -1) {
            ++upload_count;
            mesh_destroy(&info->meshes[processed_chunk_i]);
            info->meshes[processed_chunk_i] =
                mesh_create(info->meshers[i].vertices.data, info->meshers[i].vertices.length / vertex_component_count,
                    info->meshers[i].indices.data, info->meshers[i].indices.length);
            info->meshers[i].processed_chunk_i = -1;
        }
    }

    if (upload_count != 0) {
        printf("Uploaded %zu meshes\n", upload_count);
    }

    ReleaseMutex(info->world->mutex);
}

void meshing_info_draw(struct MeshingInfo *info) {
    for (size_t i = 0; i < world_length; i++) {
        mesh_draw(&info->meshes[i]);
    }
}

void meshing_info_destroy(struct MeshingInfo *info) {
    for (size_t i = 0; i < world_length; i++) {
        mesh_destroy(&info->meshes[i]);
    }

    for (size_t i = 0; i < mesher_count; i++) {
        mesher_destroy(&info->meshers[i]);
    }

    free(info->meshes);
    free(info->meshers);
    free(info->light_level_cache);
}