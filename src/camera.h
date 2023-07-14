#ifndef CAMERA_H
#define CAMERA_H

#include "window.h"

#include <cglm/struct.h>
#include <GLFW/glfw3.h>

struct Camera {
    vec3s position;
    float rotation_x;
    float rotation_y;
};

struct Camera camera_create();
void camera_move(struct Camera *camera, struct Window *window, float delta_time);
void camera_rotate(struct Camera *camera, struct Window *window);
mat4s camera_get_view_matrix(struct Camera *camera);

#endif