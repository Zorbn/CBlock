#ifndef RESOURCES_H
#define RESOURCES_H

#include <inttypes.h>
#include <glad/glad.h>

uint32_t shader_create(char *file_path, GLenum shader_type);
uint32_t program_create(char *vertex_path, char *fragment_path);
uint32_t texture_create(char *file_path);

#endif