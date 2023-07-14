#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_tex_coord;

out vec3 vertex_color;
out vec2 vertex_tex_coord;

uniform mat4 view_matrix;
uniform mat4 projection_matrix;

void main() {
    gl_Position = projection_matrix * view_matrix * vec4(in_position, 1.0);
    vertex_color = in_color;
    vertex_tex_coord = in_tex_coord;
}