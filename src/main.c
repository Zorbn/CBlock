#include "window.h"
#include "camera.h"
#include "world.h"
#include "graphics/mesh.h"
#include "graphics/mesher.h"
#include "graphics/resources.h"

#include <cglm/struct.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define SPRITE_SIZE 16.0f
#define SPRITE_X (640.0f / 2.0f - SPRITE_SIZE / 2.0f)
#define SPRITE_Y (480.0f / 2.0f - SPRITE_SIZE / 2.0f)

const float sprite_vertices[] = {
    SPRITE_X, SPRITE_Y, 0, 1, 1, 1, 0, 1,                             // 1
    SPRITE_X, SPRITE_Y + SPRITE_SIZE, 0, 1, 1, 1, 0, 0,               // 2
    SPRITE_X + SPRITE_SIZE, SPRITE_Y + SPRITE_SIZE, 0, 1, 1, 1, 1, 0, // 3
    SPRITE_X + SPRITE_SIZE, SPRITE_Y, 0, 1, 1, 1, 1, 1,               // 4
};

const uint32_t sprite_indices[] = {0, 2, 1, 0, 3, 2};

int main() {
    struct Window window = window_create("CBlock", 640, 480);

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program_3d = program_create("shader_3d.vert", "shader_3d.frag");
    uint32_t program_2d = program_create("shader_2d.vert", "shader_2d.frag");
    uint32_t texture = texture_create("texture.png");

    // TODO: Accept a const ptr in mesh_create.
    struct Mesh cursor_mesh = mesh_create(sprite_vertices, 4, sprite_indices, 6);
    float cursor_x = 0.0f;
    float cursor_y = 0.0f;

    struct Mesher mesher = mesher_create();
    struct Chunk chunks[world_length];
    struct Mesh meshes[world_length];
    struct World world = world_create();

    {
        double start_world_mesh = glfwGetTime();
        world_mesh_chunks(&world);
        double end_world_mesh = glfwGetTime();
        printf("World meshed in %fs\n", end_world_mesh - start_world_mesh);
    }

    struct Camera camera = camera_create();
    camera.position.y = chunk_height / 2;

    mat4s view_matrix;
    mat4s projection_matrix_3d;
    mat4s projection_matrix_2d;

    int32_t view_matrix_location = glGetUniformLocation(program_3d, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program_3d, "projection_matrix");

    double last_time = glfwGetTime();
    float fps_print_timer = 0.0f;

    while (!glfwWindowShouldClose(window.glfw_window)) {
        // Update:
        window_update_mouse_lock(&window);

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
        camera_interact(&camera, &window, &world);
        view_matrix = camera_get_view_matrix(&camera);

        world_mesh_chunks(&world);

        // Draw:
        if (window.was_resized) {
            glViewport(0, 0, window.width, window.height);
            projection_matrix_3d = glms_perspective(glm_rad(90.0), window.width / (float)window.height, 0.1f, 100.0f);
            projection_matrix_2d = glms_ortho(0.0f, (float)window.width, 0.0f, (float)window.height, -100.0, 100.0);

            cursor_x = window.width / 2.0f - SPRITE_SIZE / 2.0f;
            cursor_y = window.height / 2.0f - SPRITE_SIZE / 2.0f;

            window.was_resized = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_3d);
        glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, (const float *)&view_matrix);
        glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (const float *)&projection_matrix_3d);
        glBindTexture(GL_TEXTURE_2D, texture);
        world_draw(&world);

        glUseProgram(program_2d);
        glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (const float *)&projection_matrix_2d);
        mesh_draw(&cursor_mesh);

        glfwSwapBuffers(window.glfw_window);
        glfwPollEvents();
    }

    mesh_destroy(&cursor_mesh);
    world_destroy(&world);

    glDeleteProgram(program_3d);

    glfwTerminate();

    return 0;
}
