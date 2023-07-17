#include "detect_leak.h"

#include "window.h"
#include "camera.h"
#include "world.h"
#include "graphics/mesh.h"
#include "graphics/mesher.h"
#include "graphics/resources.h"
#include "graphics/sprite_batch.h"

#include <cglm/struct.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdatomic.h>

#define BLOCK_TEXTURE_COUNT 3

const size_t mesher_count = 4;
const size_t light_update_size = MAX_LIGHT_LEVEL * 2 + 1;

#define LIGHT_LEVEL_CACHE_INDEX(x, y, z) ((y) + (x)*light_update_size + (z)*light_update_size * light_update_size)

struct MeshingInfo {
    struct World *world;
    struct Mesh *meshes;
    struct Mesher *meshers;
    uint8_t *light_level_cache;
    _Atomic(bool) is_done;
    int32_t texture_atlas_width;
    int32_t texture_atlas_height;
};

DWORD WINAPI meshing_thread_start(void *start_info) {
    struct MeshingInfo *info = start_info;
    while (!info->is_done) {
        WaitForSingleObject(info->world->mutex, INFINITE);

        // Process lighting updates:
        while (info->world->light_event_queue.length > 0) {
            struct LightEventNode light_event_node =
                list_dequeue_struct_LightEventNode(&info->world->light_event_queue);

            for (int32_t z = -MAX_LIGHT_LEVEL; z <= MAX_LIGHT_LEVEL; z++) {
                int32_t world_z = z + light_event_node.z;
                int32_t cache_z = z + MAX_LIGHT_LEVEL;
                for (int32_t x = -MAX_LIGHT_LEVEL; x <= MAX_LIGHT_LEVEL; x++) {
                    int32_t world_x = x + light_event_node.x;
                    int32_t cache_x = x + MAX_LIGHT_LEVEL;
                    for (int32_t y = -MAX_LIGHT_LEVEL; y <= MAX_LIGHT_LEVEL; y++) {
                        int32_t world_y = y + light_event_node.y;
                        int32_t cache_y = y + MAX_LIGHT_LEVEL;

                        uint8_t light_level = world_get_light_level(info->world, world_x, world_y, world_z);
                        size_t cache_i = LIGHT_LEVEL_CACHE_INDEX(cache_x, cache_y, cache_z);
                        info->light_level_cache[cache_i] = light_level;
                    }
                }
            }

            switch (light_event_node.event_type) {
                case LIGHT_EVENT_TYPE_ADD:
                    world_light_add(info->world, light_event_node.x, light_event_node.y, light_event_node.z);
                    break;
                case LIGHT_EVENT_TYPE_REMOVE:
                    world_light_remove(info->world, light_event_node.x, light_event_node.y, light_event_node.z);
                    break;
                case LIGHT_EVENT_TYPE_UPDATE:
                    world_light_update(info->world, light_event_node.x, light_event_node.y, light_event_node.z);
                    break;
            }

            for (int32_t z = -MAX_LIGHT_LEVEL; z <= MAX_LIGHT_LEVEL; z++) {
                int32_t world_z = z + light_event_node.z;
                int32_t cache_z = z + MAX_LIGHT_LEVEL;
                for (int32_t x = -MAX_LIGHT_LEVEL; x <= MAX_LIGHT_LEVEL; x++) {
                    int32_t world_x = x + light_event_node.x;
                    int32_t cache_x = x + MAX_LIGHT_LEVEL;
                    for (int32_t y = -MAX_LIGHT_LEVEL; y <= MAX_LIGHT_LEVEL; y++) {
                        int32_t world_y = y + light_event_node.y;
                        int32_t cache_y = y + MAX_LIGHT_LEVEL;

                        uint8_t light_level = world_get_light_level(info->world, world_x, world_y, world_z);
                        size_t cache_i = LIGHT_LEVEL_CACHE_INDEX(cache_x, cache_y, cache_z);
                        uint8_t cached_light_level = info->light_level_cache[cache_i];

                        // Blocks outside the map will never differ from their cached light level.
                        // Getting a light level outside the map returns a constant placeholder.
                        // All blocks that make it past this point are inside real chunks.
                        if (light_level == cached_light_level) {
                            continue;
                        }

                        int32_t chunk_x = world_x / chunk_size;
                        int32_t chunk_z = world_z / chunk_size;
                        size_t chunk_i = CHUNK_INDEX(chunk_x, chunk_z);
                        info->world->chunks[chunk_i].is_dirty = true;
                    }
                }
            }
        }

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
    const size_t light_update_length = light_update_size * light_update_size * light_update_size;

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

int main() {
    struct Window window = window_create("CBlock", 640, 480);

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program_3d = program_create("shader_3d.vert", "shader_3d.frag");
    uint32_t program_2d = program_create("shader_2d.vert", "shader_2d.frag");

    char *block_texture_paths[BLOCK_TEXTURE_COUNT] = {
        "assets/block_textures/dirt.png",
        "assets/block_textures/grass.png",
        "assets/block_textures/light.png",
    };
    // TODO: Add texture_bind and texture_destroy
    struct TextureArray texture_atlas_3d = texture_array_create(block_texture_paths, BLOCK_TEXTURE_COUNT, 16, 16);
    struct Texture texture_atlas_2d = texture_create("assets/texture_atlas.png");

    const float cursor_size = 16.0f;
    float cursor_x = 0.0f;
    float cursor_y = 0.0f;
    struct SpriteBatch sprite_batch = sprite_batch_create(16);

    struct World world = world_create();

    struct Camera camera = camera_create();
    camera.position.y = chunk_height / 2 + 3;
    camera.position.x += 2;
    camera.position.z += 2;

    mat4s view_matrix;
    mat4s projection_matrix_3d;
    mat4s projection_matrix_2d;

    int32_t view_matrix_location = glGetUniformLocation(program_3d, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program_3d, "projection_matrix");

    double last_time = glfwGetTime();
    float fps_print_timer = 0.0f;

    struct MeshingInfo meshing_info = meshing_info_create(&world, texture_atlas_3d.width, texture_atlas_3d.height);
    HANDLE meshing_thread = CreateThread(NULL, 0, meshing_thread_start, &meshing_info, 0, NULL);
    assert(meshing_thread);

    while (!glfwWindowShouldClose(window.glfw_window)) {
        // Update:
        if (window.was_resized) {
            glViewport(0, 0, window.width, window.height);
            projection_matrix_3d = glms_perspective(glm_rad(90.0), window.width / (float)window.height, 0.1f, 100.0f);
            projection_matrix_2d = glms_ortho(0.0f, (float)window.width, 0.0f, (float)window.height, -100.0, 100.0);

            cursor_x = window.width / 2.0f - cursor_size / 2.0f;
            cursor_y = window.height / 2.0f - cursor_size / 2.0f;

            window.was_resized = false;
        }

        double current_time = glfwGetTime();
        float delta_time = (float)(current_time - last_time);
        last_time = current_time;

        fps_print_timer += delta_time;

        if (fps_print_timer > 1.0f) {
            fps_print_timer = 0.0f;
            printf("fps: %f\n", 1.0f / delta_time);
        }

        camera_move(&camera, &window, &world, delta_time);
        camera_rotate(&camera, &window);
        camera_interact(&camera, &window.input, &world);
        view_matrix = camera_get_view_matrix(&camera);

        meshing_info_upload(&meshing_info);

        sprite_batch_begin(&sprite_batch);
        sprite_batch_add(&sprite_batch, (struct Sprite){
                                            .x = cursor_x,
                                            .y = cursor_y,
                                            .width = cursor_size,
                                            .height = cursor_size,
                                            .texture_y = cursor_size,
                                            .texture_width = cursor_size,
                                            .texture_height = cursor_size,
                                        });
        sprite_batch_end(&sprite_batch, texture_atlas_2d.width, texture_atlas_2d.height);

        // Draw:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_3d);
        glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, (const float *)&view_matrix);
        glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (const float *)&projection_matrix_3d);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture_atlas_3d.id);
        meshing_info_draw(&meshing_info);

        glUseProgram(program_2d);
        glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (const float *)&projection_matrix_2d);
        glBindTexture(GL_TEXTURE_2D, texture_atlas_2d.id);
        sprite_batch_draw(&sprite_batch);

        window_update(&window);

        glfwSwapBuffers(window.glfw_window);
        glfwPollEvents();
    }

    meshing_info.is_done = true;
    WaitForSingleObject(meshing_thread, INFINITE);
    CloseHandle(meshing_thread);
    meshing_info_destroy(&meshing_info);

    sprite_batch_destroy(&sprite_batch);
    world_destroy(&world);

    glDeleteTextures(1, &texture_atlas_3d.id);
    glDeleteTextures(1, &texture_atlas_2d.id);

    glDeleteProgram(program_3d);

    window_destroy(&window);

    printf("Found leaks: %s\n", _CrtDumpMemoryLeaks() ? "true" : "false");

    return 0;
}
