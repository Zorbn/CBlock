#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>

struct Window {
    GLFWwindow *glfw_window;
    int32_t width;
    int32_t height;
    bool was_resized;
};

struct Window window_create(char *title, int32_t width, int32_t height);

#endif