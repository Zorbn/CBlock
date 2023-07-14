#version 330 core

in vec3 vertex_color;
in vec2 vertex_tex_coord;

out vec4 out_frag_color;

uniform sampler2D texture_sampler;

void main() {
    out_frag_color = vec4(vertex_color, 1.0f) * texture(texture_sampler, vertex_tex_coord);
}