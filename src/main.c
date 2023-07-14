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

int main() {
    struct Window window = window_create("CBlock", 640, 480);

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program = program_create("shader.vert", "shader.frag");
    uint32_t texture = texture_create("texture.png");

    struct Mesher mesher = mesher_create();
    struct Chunk chunk = chunk_create();
    struct Mesh mesh = mesher_mesh_chunk(&mesher, &chunk);

    struct Camera camera = camera_create();

    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4s view_matrix;
    mat4s projection_matrix;

    int32_t view_matrix_location = glGetUniformLocation(program, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program, "projection_matrix");

    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(window.glfw_window)) {
        window_update_mouse_lock(&window);

        double current_time = glfwGetTime();
        float delta_time = (float)(current_time - last_time);
        printf("fps: %f\n", 1.0f / delta_time);
        last_time = current_time;

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
        mesh_draw(&mesh);

        glfwSwapBuffers(window.glfw_window);
        glfwPollEvents();
    }

    mesh_destroy(&mesh);
    chunk_destroy(&chunk);
    mesher_destroy(&mesher);

    glDeleteProgram(program);

    glfwTerminate();

    return 0;
}
