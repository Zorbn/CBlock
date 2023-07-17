#include "directions.h"

const ivec3s directions[6] = {
    {{0, 0, -1}}, // Forward
    {{0, 0, 1}},  // Backward
    {{1, 0, 0}},  // Right
    {{-1, 0, 0}}, // Left
    {{0, 1, 0}},  // Up
    {{0, -1, 0}}, // Down
};

const size_t down = 5;