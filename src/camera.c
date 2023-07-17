#include "camera.h"

struct Camera camera_create() {
    return (struct Camera){
        .position = {{0.0f, 0.0f, 0.0f}},
        .rotation_x = 0.0f,
        .rotation_y = 0.0f,
    };
}

void camera_move(struct Camera *camera, struct Window *window, struct World *world, float delta_time) {
    if (!window->is_mouse_locked) {
        return;
    }

    vec3s direction = {{0.0f, 0.0f, 0.0f}};

    if (input_is_button_held(&window->input, GLFW_KEY_W)) {
        direction.z -= 1.0f;
    }

    if (input_is_button_held(&window->input, GLFW_KEY_S)) {
        direction.z += 1.0f;
    }

    if (input_is_button_held(&window->input, GLFW_KEY_LEFT_SHIFT)) {
        direction.y -= 1.0f;
    }

    if (input_is_button_held(&window->input, GLFW_KEY_SPACE)) {
        direction.y += 1.0f;
    }

    if (input_is_button_held(&window->input, GLFW_KEY_A)) {
        direction.x -= 1.0f;
    }

    if (input_is_button_held(&window->input, GLFW_KEY_D)) {
        direction.x += 1.0f;
    }

    direction = glms_vec3_normalize(direction);

    const float camera_move_speed = 5.0f;
    float current_move_speed = delta_time * camera_move_speed;

    vec3s forward_movement = glms_vec3_rotate(GLMS_ZUP, glm_rad(camera->rotation_y), GLMS_YUP);
    forward_movement = glms_vec3_scale(forward_movement, direction.z * current_move_speed);
    vec3s right_movement = glms_vec3_rotate(GLMS_ZUP, glm_rad(camera->rotation_y + 90.0f), GLMS_YUP);
    right_movement = glms_vec3_scale(right_movement, direction.x * current_move_speed);

    const vec3s size = (vec3s){
        .x = 0.8f,
        .y = 1.8f,
        .z = 0.8f,
    };
    const vec3s origin = (vec3s){
        .y = 0.75f,
    };

    vec3s delta_position = glms_vec3_add(forward_movement, right_movement);
    delta_position.y = direction.y * current_move_speed;

    camera->position.z += delta_position.z;
    if (world_is_colliding_with_box(world, camera->position, size, origin)) {
        camera->position.z -= delta_position.z;
    }

    camera->position.x += delta_position.x;
    if (world_is_colliding_with_box(world, camera->position, size, origin)) {
        camera->position.x -= delta_position.x;
    }

    camera->position.y += delta_position.y;
    if (world_is_colliding_with_box(world, camera->position, size, origin)) {
        camera->position.y -= delta_position.y;
    }
}

// TODO: Mouse delta should be handled by Input not Window.
void camera_rotate(struct Camera *camera, struct Window *window) {
    float delta_x, delta_y;
    window_get_mouse_delta(window, &delta_x, &delta_y);

    const float camera_rotate_speed = 0.1f;
    camera->rotation_x -= delta_y * camera_rotate_speed;
    camera->rotation_y -= delta_x * camera_rotate_speed;
    camera->rotation_x = glm_clamp(camera->rotation_x, -89.0f, 89.0f);

    mat4s rotation_matrix_y = glms_rotate_y(glms_mat4_identity(), glm_rad(camera->rotation_y));
    mat4s rotation_matrix_x = glms_rotate_x(glms_mat4_identity(), glm_rad(camera->rotation_x));

    vec3s look_vector = {{0.0f, 0.0f, -1.0f}};
    look_vector = glms_mat4_mulv3(rotation_matrix_x, look_vector, 1.0f);
    look_vector = glms_mat4_mulv3(rotation_matrix_y, look_vector, 1.0f);
    camera->look_vector = look_vector;
}

// TODO: This should be part of the player when it's created.
void camera_interact(struct Camera *camera, struct Input *input, struct World *world) {
    if (input_is_button_pressed(input, GLFW_MOUSE_BUTTON_LEFT)) {
        struct RaycastHit hit = world_raycast(world, camera->position, camera->look_vector, 10.0f);

        if (hit.block != 0) {
            double start_update_lights = glfwGetTime();
            for (size_t i = 0; i < 100; i++) {
                world_set_block(world, hit.position.x, hit.position.y, hit.position.z, 0);
            }
            double end_update_lights = glfwGetTime();
            printf("Light updated in %fs\n", end_update_lights - start_update_lights);
        }
    } else if (input_is_button_pressed(input, GLFW_MOUSE_BUTTON_RIGHT)) {
        struct RaycastHit hit = world_raycast(world, camera->position, camera->look_vector, 10.0f);

        if (hit.block != 0) {
            world_set_block(world, hit.last_position.x, hit.last_position.y, hit.last_position.z, 1);
        }
    } else if (input_is_button_pressed(input, GLFW_MOUSE_BUTTON_MIDDLE)) {
        struct RaycastHit hit = world_raycast(world, camera->position, camera->look_vector, 10.0f);

        if (hit.block != 0) {
            world_set_block(world, hit.last_position.x, hit.last_position.y, hit.last_position.z, 3);
        }
    }
}

mat4s camera_get_view_matrix(struct Camera *camera) {
    vec3s view_target = glms_vec3_add(camera->look_vector, camera->position);

    return glms_lookat(camera->position, view_target, GLMS_YUP);
}