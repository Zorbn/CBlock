#include "window.h"

void framebuffer_size_callback(GLFWwindow *glfw_window, int32_t width, int32_t height) {
    struct Window *window = glfwGetWindowUserPointer(glfw_window);
    window->width = width;
    window->height = height;
    window->was_resized = true;
}

struct Window window_create(char *title, int32_t width, int32_t height) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *glfw_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!glfw_window) {
        puts("Failed to create GLFW window");
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(glfw_window);

    struct Window window = {
        .glfw_window = glfw_window,
    };
    glfwSetWindowUserPointer(glfw_window, (void *)&window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        puts("Failed to initialize GLAD");
        exit(-1);
    }

    glfwSetFramebufferSizeCallback(glfw_window, framebuffer_size_callback);
    framebuffer_size_callback(glfw_window, width, height);

    return window;
}