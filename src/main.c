#include "window.h"
#include "chunk.h"
#include "camera.h"
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

const size_t world_size = 24;
const size_t world_length = world_size * world_size;

int main() {
    struct Window window = window_create("CBlock", 640, 480);

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program = program_create("shader.vert", "shader.frag");
    uint32_t texture = texture_create("texture.png");

    struct Mesher mesher = mesher_create();
    struct Chunk chunks[world_length];
    struct Mesh meshes[world_length];

    {
        for (size_t i = 0; i < world_length; i++) {
            chunks[i] = chunk_create((i % world_size) * chunk_size, i / world_size * chunk_size);
        }
        
        double start_world_load = glfwGetTime();
        for (size_t i = 0; i < world_length; i++) {
            meshes[i] = mesher_mesh_chunk(&mesher, &chunks[i]);
        }
        double end_world_load = glfwGetTime();
        printf("World loaded in %fs\n", end_world_load - start_world_load);
    }

    struct Camera camera = camera_create();
    camera.position.y = chunk_height / 2;

    mat4s view_matrix;
    mat4s projection_matrix;

    int32_t view_matrix_location = glGetUniformLocation(program, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program, "projection_matrix");

    double last_time = glfwGetTime();
    float fps_print_timer = 0.0f;

    while (!glfwWindowShouldClose(window.glfw_window)) {
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
        view_matrix = camera_get_view_matrix(&camera);

        if (window.was_resized) {
            glViewport(0, 0, window.width, window.height);
            projection_matrix = glms_perspective(glm_rad(90.0), window.width / (float)window.height, 0.1f, 100.0f);
            window.was_resized = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, (const float *)&view_matrix);
        glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, (const float *)&projection_matrix);
        glBindTexture(GL_TEXTURE_2D, texture);
        for (size_t i = 0; i < world_length; i++) {
            mesh_draw(&meshes[i]);
        }

        glfwSwapBuffers(window.glfw_window);
        glfwPollEvents();
    }

    for (size_t i = 0; i < world_length; i++) {
        chunk_destroy(&chunks[i]);
        mesh_destroy(&meshes[i]);
    }
    mesher_destroy(&mesher);

    glDeleteProgram(program);

    glfwTerminate();

    return 0;
}
