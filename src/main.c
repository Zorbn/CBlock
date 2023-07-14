#include "window.h"
#include "graphics/mesh.h"
#include "graphics/resources.h"

#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

const float vertices[] = {
    0.5f, 0.5f, 0.0f, /* Color: */ 1.0f, 0.0f, 0.0f, /* Tex coords: */ 1.0f, 1.0f,   // Top right
    0.5f, -0.5f, 0.0f, /* Color: */ 0.0f, 1.0f, 0.0f, /* Tex coords: */ 1.0f, 0.0f,  // Bottom right
    -0.5f, -0.5f, 0.0f, /* Color: */ 0.0f, 0.0f, 1.0f, /* Tex coords: */ 0.0f, 0.0f, // Bottom left
    -0.5f, 0.5f, 0.0f, /* Color: */ 1.0f, 0.0f, 1.0f, /* Tex coords: */ 0.0f, 1.0f,  // Top left
};

const uint32_t indices[] = {
    0, 1, 3, // Triangle 1
    1, 2, 3, // Triangle 2
};

int main() {
    struct Window window = window_create("CBlock", 640, 480);

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program = program_create("shader.vert", "shader.frag");
    struct Mesh mesh = mesh_create((float *)vertices, 4, (uint32_t *)indices, 6);
    uint32_t texture = texture_create("texture.png");

    vec3 eye = {0.0f, 0.0f, -10.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4 view_matrix;
    mat4 projection_matrix;
    glm_lookat(eye, center, up, view_matrix);

    int32_t view_matrix_location = glGetUniformLocation(program, "view_matrix");
    int32_t projection_matrix_location = glGetUniformLocation(program, "projection_matrix");

    while (!glfwWindowShouldClose(window.glfw_window)) {
        if (glfwGetKey(window.glfw_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.glfw_window, true);
        }

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

    glDeleteProgram(program);

    glfwTerminate();

    return 0;
}
