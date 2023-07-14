#include "window.h"
#include "chunk.h"
#include "graphics/mesh.h"
#include "graphics/mesher.h"
#include "graphics/resources.h"

#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

void move_camera(struct Window *window, float delta_time, vec3 eye) {
    float forward_direction = 0.0f;
    float up_direction = 0.0f;
    float right_direction = 0.0f;

    if (glfwGetKey(window->glfw_window, GLFW_KEY_W) == GLFW_PRESS) {
        forward_direction -= 1.0f;
    }

    if (glfwGetKey(window->glfw_window, GLFW_KEY_S) == GLFW_PRESS) {
        forward_direction += 1.0f;
    }

    if (glfwGetKey(window->glfw_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        up_direction -= 1.0f;
    }

    if (glfwGetKey(window->glfw_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        up_direction += 1.0f;
    }

    if (glfwGetKey(window->glfw_window, GLFW_KEY_A) == GLFW_PRESS) {
        right_direction -= 1.0f;
    }

    if (glfwGetKey(window->glfw_window, GLFW_KEY_D) == GLFW_PRESS) {
        right_direction += 1.0f;
    }

    const float camera_move_speed = 10.0f;
    float current_move_speed = delta_time * camera_move_speed;

    eye[0] += right_direction * current_move_speed;
    eye[1] += up_direction * current_move_speed;
    eye[2] += forward_direction * current_move_speed;
}

int main() {
    struct Window window = window_create("CBlock", 640, 480);

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program = program_create("shader.vert", "shader.frag");
    // struct Mesh mesh = mesh_create((float *)vertices, 4, (uint32_t *)indices, 6);
    uint32_t texture = texture_create("texture.png");

    struct Mesher mesher = mesher_create();
    struct Chunk chunk = chunk_create();
    // TODO: Add mesh_destroy().
    struct Mesh mesh = mesher_mesh_chunk(&mesher, &chunk);

    vec3 eye = {0.0f, 0.0f, 10.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4 view_matrix;
    mat4 projection_matrix;

    int32_t view_matrix_location = glGetUniformLocation(program, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program, "projection_matrix");

    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(window.glfw_window)) {
        if (glfwGetKey(window.glfw_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.glfw_window, true);
        }

        double current_time = glfwGetTime();
        float delta_time = (float)(current_time - last_time);
        last_time = current_time;

        move_camera(&window, delta_time, eye);
        glm_vec3_copy(eye, center);
        center[2] -= 1.0f;
        glm_lookat(eye, center, up, view_matrix);

        if (window.was_resized) {
            glViewport(0, 0, window.width, window.height);
            glm_perspective(glm_rad(45.0), window.width / (float)window.height, 0.1f, 100.0f, projection_matrix);
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

    chunk_destroy(&chunk);
    mesher_destroy(&mesher);

    glDeleteProgram(program);

    glfwTerminate();

    return 0;
}
