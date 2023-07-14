#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

struct Window {
    GLFWwindow *glfw_window;
    int32_t width;
    int32_t height;
    bool was_resized;
};

struct Mesh {
    uint32_t vbo;
    uint32_t vao;
    uint32_t ebo;
    uint32_t index_count;
};

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

char *get_file_string(char *file_path) {
    FILE *file;
    fopen_s(&file, file_path, "rb");

    if (!file) {
        printf("Failed to open file: %s\n", file_path);
        exit(-1);
    }

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    rewind(file);

    if (file_length == -1) {
        printf("Couldn't get file length: %s\n", file_path);
    }

    size_t string_length = (size_t)file_length;
    char *buffer = malloc(string_length + 1);
    if (!buffer) {
        printf("Failed to allocate space for file: %s\n", file_path);
    }

    fread(buffer, 1, string_length, file);
    buffer[string_length] = '\0';

    return buffer;
}

uint32_t shader_create(char *file_path, GLenum shader_type) {
    uint32_t shader = glCreateShader(shader_type);

    char *shader_source = get_file_string(file_path);
    glShaderSource(shader, 1, (const char *const *)&shader_source, NULL);
    glCompileShader(shader);

    int32_t success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        printf("Failed to compile shader (%s):\n%s\n", file_path, info_log);
    }

    free(shader_source);

    return shader;
}

uint32_t program_create(char *vertex_path, char *fragment_path) {
    uint32_t vertex_shader = shader_create(vertex_path, GL_VERTEX_SHADER);
    uint32_t fragment_shader = shader_create(fragment_path, GL_FRAGMENT_SHADER);

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int32_t success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("Failed to link program of (%s) and (%s):\n%s\n", vertex_path, fragment_path, info_log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

struct Mesh mesh_create(float *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count) {
    uint32_t vbo;
    glGenBuffers(1, &vbo);

    uint32_t vao;
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    const uint64_t sizeof_vec2 = sizeof(float) * 2;
    const uint64_t sizeof_vec3 = sizeof(float) * 3;
    const uint64_t sizeof_vertex = sizeof_vec3 * 2 + sizeof_vec2;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_vertex * vertex_count, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof_vertex, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof_vertex, (void *)sizeof_vec3);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof_vertex, (void *)(sizeof_vec3 * 2));
    glEnableVertexAttribArray(2);

    uint32_t ebo;
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * index_count, indices, GL_STATIC_DRAW);

    return (struct Mesh){
        .vao = vao,
        .vbo = vbo,
        .ebo = ebo,
        .index_count = index_count,
    };
}

void mesh_draw(struct Mesh *mesh) {
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
}

uint32_t texture_create(char *file_path) {
    uint32_t texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, channel_count;
    uint8_t *data = stbi_load(file_path, &width, &height, &channel_count, 0);
    if (!data) {
        printf("Failed to load texture: %s\n", file_path);
        exit(-1);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    return texture;
}

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
