#ifndef LIST_H
#define LIST_H

#include "detect_leak.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#define LIST_DEFINE(type)                                                                                              \
    struct List_##type {                                                                                               \
        type *data;                                                                                                    \
        size_t capacity;                                                                                               \
        size_t length;                                                                                                 \
    };                                                                                                                 \
                                                                                                                       \
    inline struct List_##type list_create_##type(size_t capacity) {                                                    \
        struct List_##type list = (struct List_##type){                                                                \
            .data = malloc(capacity * sizeof(type)),                                                                   \
            .capacity = capacity,                                                                                      \
            .length = 0,                                                                                               \
        };                                                                                                             \
                                                                                                                       \
        assert(list.data);                                                                                             \
                                                                                                                       \
        return list;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    inline void list_reset_##type(struct List_##type *list) {                                                          \
        list->length = 0;                                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    inline void list_push_##type(struct List_##type *list, type value) {                                               \
        if (list->length >= list->capacity) {                                                                          \
            list->capacity *= 2;                                                                                       \
            list->data = realloc(list->data, list->capacity * sizeof(type));                                           \
            assert(list->data);                                                                                        \
        }                                                                                                              \
                                                                                                                       \
        list->data[list->length] = value;                                                                              \
        ++list->length;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    inline type list_dequeue_##type(struct List_##type *list) {                                                        \
        assert(list->length > 0);                                                                                      \
                                                                                                                       \
        type value = list->data[0];                                                                                    \
                                                                                                                       \
        for (size_t i = 0; i < list->length - 1; i++) {                                                                \
            list->data[i] = list->data[i + 1];                                                                         \
        }                                                                                                              \
                                                                                                                       \
        --list->length;                                                                                                \
        return value;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    inline void list_destroy_##type(struct List_##type *list) {                                                        \
        free(list->data);                                                                                              \
    }

// TODO: Should memmove be used for dequeue?

LIST_DEFINE(float)
LIST_DEFINE(uint32_t)
LIST_DEFINE(int32_t)

// TODO:
// inline type list_pop_##type(struct List_##type *list) {
//     assert(list->length > 0);
//
//     --list->length;
//     return list->data[list->length];
// }

#endif