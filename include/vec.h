#ifndef VEC_H
#define VEC_H

#include <stdio.h>
#include <limits.h>

#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t capacity;
    size_t n;
    void *data;
} vec_info;

#if defined(_MSC_VER)
    #define VEC_ATTR __pragma(warning(suppress:4505)) static
#else
    #define VEC_ATTR __attribute__((unused)) static
#endif

VEC_ATTR void *vec_new() {
    vec_info *info = malloc(sizeof(vec_info));
    if (info) {
        info->data = NULL;
        info->n = 0;
        info->capacity = 0;
    }
    return info;
}

VEC_ATTR void vec_clear(void *vec) {
    if (vec) {
        vec_info *info = vec;
        if (info->data) {
            info->data = NULL;
        }
        info->n = 0;
        info->capacity = 0;
    }
}

VEC_ATTR void vec_free(void *vec) {
    if (vec) {
        vec_info *info = vec;
        if (info->data) {
            free(info->data);
        }
        vec_clear(vec);
        free(vec);
    }
}

VEC_ATTR void *_vec_clone(void *vec, size_t elem_size) {
    if (!vec) {
        return NULL;
    }
    vec_info *src = vec;
    vec_info *dst = vec_new();
    if (!dst) {
        return NULL;
    }
    dst->n = src->n;
    dst->capacity = src->n;
    if (src->data && src->n > 0) {
        dst->data = malloc(dst->n * elem_size);
        if (!dst->data) {
            free(dst);
            return NULL;
        }
        memcpy(dst->data, src->data, dst->n * elem_size);
    }

    return dst;
}

VEC_ATTR int _vec_resize(void *vec, size_t elem_size, size_t capacity) {
    vec_info *info = vec;
    if (capacity != info->capacity) {
        if (capacity == 0) {
            free(info->data);
            info->data = NULL;
        }
        else {
            void *buffer = realloc(info->data, elem_size * capacity);
            if (buffer == NULL) {
                return 1;
            }
            info->data = buffer;
        }
        info->n = capacity;
        info->capacity = capacity;
    }
    return 0;
}

VEC_ATTR int _vec_reserve(void *vec, size_t elem_size, size_t size) {
    vec_info *info = vec;
    if (size > info->n) {
        if (_vec_resize(info, elem_size, size)) {
            return 1;
        }
    }
    return 0;
}

VEC_ATTR int _vec_push(void *vec, size_t elem_size) {
    vec_info *info = vec;
    size_t new_capacity = info->capacity <= 0 ? 4 : info->capacity;
    size_t new_n = info->n + 1;
    while (new_capacity < new_n) {
        if (new_capacity > SIZE_MAX / 2) {
            return 1;
        }
        new_capacity *= 2;
    }
    if (_vec_resize(info, elem_size, new_capacity)) {
        return 1;
    }
    info->n = new_n;
    return 0;
}

VEC_ATTR void _vec_remove_at(void *vec, size_t elem_size, size_t idx) {
    vec_info *info = vec;
    info->n--;
    if (idx == info->n) {
        return;
    }
    memmove(
        (char *)info->data + idx * elem_size,
        (char *)info->data + (idx + 1) * elem_size,
        (info->n - idx) * elem_size
    );
}

#define vec(type) type *

#define vec_info(vec) ((vec_info*)((void*)vec))

#define vec_data(vec) (vec_info(vec)->data)

#define vec_len(vec) (vec_info(vec)->n)

#define vec_clone(vec) (typeof(vec))(_vec_clone(vec, sizeof(*vec)))

#define vec_resize(vec, n) _vec_resize(vec, sizeof(*vec), n)

#define vec_shrink(vec) _vec_resize(vec, sizeof(*vec), vec_info(vec)->n)

#define vec_reserve(vec, n) _vec_reserve(vec, sizeof(*vec), n)

#define vec_clear(vec) _vec_resize(vec, sizeof(*vec), 0)

#define vec_push(vec, value) (_vec_push(vec, sizeof(*vec)) ? 1 : \
    (((typeof(vec))(vec_data(vec)))[vec_len(vec) - 1] = value, 0))

#define vec_at(vec, idx) (((typeof(vec))(vec_data(vec)))[idx])

#define vec_remove_at(vec, idx) _vec_remove_at(vec, sizeof(*vec), idx)

#define vec_map(vec, idx, name, code) { \
    vec_each(vec, idx, name, { \
        vec_at(vec, idx) = (code); \
    }) \
}

#define vec_map_new(vec, idx, name, code) ({ \
    typeof(*vec) name; \
    size_t idx; \
    typeof(code) *vec2 = vec_new(); \
    if (vec2) { \
        for (idx = 0; idx < vec_len(vec); idx++) { \
            name = vec_at(vec, idx); \
            if (vec_push(vec2, (code))) { \
                vec_free(vec2); \
                vec2 = NULL; \
                break; \
            } \
        } \
    } \
    vec2; \
})

#define vec_reduce(vec, idx, name, acc, init, code) ({ \
    typeof(init) acc = init; \
    vec_each(vec, idx, name, { \
        acc = (code); \
    }) \
    acc; \
})

#define vec_filter_new(vec, idx, name, code) ({ \
    typeof(vec) vec2 = vec_new(); \
    if (vec2) { \
        vec_each(vec, idx, name, { \
            if (code) { \
                if (vec_push(vec2, name)) { \
                    vec_free(vec2); \
                    vec2 = NULL; \
                    break; \
                } \
            } \
        }) \
    } \
    vec2; \
})

#define vec_filter(vec, idx, name, code) { \
    size_t _vec_filter_idx = 0; \
    vec_each(vec, idx, name, { \
        if (code) { \
            vec_at(vec, _vec_filter_idx++) = name; \
        } \
    }) \
    vec_resize(vec, _vec_filter_idx); \
}

#define vec_find_index(vec, idx, name, code) ({ \
    ssize_t found = -1; \
    vec_each(vec, idx, name, { \
        if (code) { \
            found = idx; \
            break; \
        } \
    }) \
    found; \
})

#define vec_find(vec, idx, name, code) ({ \
    typeof(*vec) *found = NULL; \
    vec_each(vec, idx, name, { \
        if (code) { \
            found = &vec_at(vec, idx); \
            break; \
        } \
    }) \
    found; \
})

#define vec_each(vec, idx, name, code) \
    for (size_t idx = 0; idx < vec_len(vec); idx++) { \
        typeof(*vec) name = vec_at(vec, idx); \
        code \
    }

#define vec_each_ptr(vec, idx, name, code) \
    for (size_t idx = 0; idx < vec_len(vec); idx++) { \
        typeof(vec) name = &vec_at(vec, idx); \
        code \
    }

#define arr_each(arr, idx, name, code) \
    for (size_t idx = 0; idx < sizeof(arr) / sizeof(arr[0]); idx++) { \
        typeof(*arr) name = arr[idx]; \
        code \
    }

#define arr_each_ptr(arr, idx, name, code) \
    for (size_t idx = 0; idx < sizeof(arr) / sizeof(arr[0]); idx++) { \
        typeof(arr) name = &arr[idx]; \
        code \
    }

#endif
