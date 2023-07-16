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

#define BLOCK_TEXTURE_COUNT 2

struct MeshingThreadInfo {
    struct World *world;
    _Atomic(int32_t) processed_chunk_i;
    _Atomic(bool) is_done;
    int32_t texture_atlas_width;
    int32_t texture_atlas_height;
};

DWORD WINAPI meshing_thread_start(void *start_info) {
    struct MeshingThreadInfo *info = start_info;
    while (!info->is_done) {
        while (info->processed_chunk_i != -1) {
            Sleep(0);
        }

        for (int32_t i = 0; i < world_length; i++) {
            if (!info->world->chunks[i].is_dirty) {
                continue;
            }

            info->world->chunks[i].is_dirty = false;

            // double start_chunk_mesh = glfwGetTime();
            mesher_mesh_chunk(&info->world->mesher, info->world, &info->world->chunks[i], info->texture_atlas_width,
                info->texture_atlas_height);
            // double end_chunk_mesh = glfwGetTime();
            // printf("Chunk meshed in %fs\n", end_chunk_mesh - start_chunk_mesh);
            info->processed_chunk_i = i;
            break;
        }
    }

    return 0;
}

void upload_processed_chunk(struct MeshingThreadInfo *info) {
    int32_t processed_chunk_i = info->processed_chunk_i;
    if (processed_chunk_i != -1) {
        mesh_destroy(&info->world->meshes[processed_chunk_i]);
        info->world->meshes[processed_chunk_i] =
            mesh_create(info->world->mesher.vertices.data, info->world->mesher.vertices.length / vertex_component_count,
                info->world->mesher.indices.data, info->world->mesher.indices.length);
        info->processed_chunk_i = -1;
    }
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
    camera.position.y = chunk_height / 2;

    mat4s view_matrix;
    mat4s projection_matrix_3d;
    mat4s projection_matrix_2d;

    int32_t view_matrix_location = glGetUniformLocation(program_3d, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program_3d, "projection_matrix");

    double last_time = glfwGetTime();
    float fps_print_timer = 0.0f;

    struct MeshingThreadInfo meshing_thread_info = (struct MeshingThreadInfo){
        .world = &world,
        .processed_chunk_i = -1,
        .is_done = false,
        .texture_atlas_width = texture_atlas_3d.width,
        .texture_atlas_height = texture_atlas_3d.height,
    };
    HANDLE meshing_thread = CreateThread(NULL, 0, meshing_thread_start, &meshing_thread_info, 0, NULL);
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

        camera_move(&camera, &window, delta_time);
        camera_rotate(&camera, &window);
        camera_interact(&camera, &window.input, &world);
        view_matrix = camera_get_view_matrix(&camera);

        upload_processed_chunk(&meshing_thread_info);

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
        world_draw(&world);

        glUseProgram(program_2d);
        glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (const float *)&projection_matrix_2d);
        glBindTexture(GL_TEXTURE_2D, texture_atlas_2d.id);
        sprite_batch_draw(&sprite_batch);

        window_update(&window);

        glfwSwapBuffers(window.glfw_window);
        glfwPollEvents();
    }

    meshing_thread_info.is_done = true;
    WaitForSingleObject(meshing_thread, INFINITE);

    sprite_batch_destroy(&sprite_batch);
    world_destroy(&world);

    glDeleteTextures(1, &texture_atlas_3d.id);
    glDeleteTextures(1, &texture_atlas_2d.id);

    glDeleteProgram(program_3d);

    window_destroy(&window);

    printf("Found leaks: %s\n", _CrtDumpMemoryLeaks() ? "true" : "false");

    return 0;
}
