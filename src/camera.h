#ifndef CAMERA_H
#define CAMERA_H

#include "detect_leak.h"

#include "window.h"
#include "world.h"

#include <cglm/struct.h>
#include <GLFW/glfw3.h>

struct Camera {
    vec3s position;
    float rotation_x;
    float rotation_y;
    vec3s look_vector;
};

struct Camera camera_create(void);
void camera_move(struct Camera *camera, struct Window *window, struct World* world, float delta_time);
void camera_rotate(struct Camera *camera, struct Window *window);
void camera_interact(struct Camera *camera, struct Input *input, struct World *world);
mat4s camera_get_view_matrix(struct Camera *camera);

#endif