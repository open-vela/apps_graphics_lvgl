/**
 * @file lv_vector_polygon.h
 *
 */

#ifndef LV_VECTOR_POLYGON_H
#define LV_VECTOR_POLYGON_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_vector_private.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE

/*********************
 *      DEFINES
 *********************/
#define LV_VECTOR_POLYGON_STOP 0xF

#define CHECK_AND_RESIZE_PATH_CONTAINER(P, N) \
    do { \
        if ((lv_array_size(&(P)->ops) + (N)) > lv_array_capacity(&(P)->ops)) { \
            lv_array_resize(&(P)->ops, ((P)->ops.capacity << 1)); \
        } \
        if ((lv_array_size(&(P)->points) + (N)) > lv_array_capacity(&(P)->points)) { \
            lv_array_resize(&(P)->points, ((P)->points.capacity << 1)); \
        } \
    } while(0)

#define OP_PUSH_BACK(arr, op) \
    do { \
        uint8_t * co = ((uint8_t *)(arr)->data) + (arr)->size; \
        *co = *(op);\
        (arr)->size++; \
    } while(0)

#define POINT_PUSH_BACK(arr, p) \
    do { \
        lv_fpoint_t * pt = ((lv_fpoint_t *)(arr)->data) + (arr)->size; \
        *pt = *(p);\
        (arr)->size++; \
    } while(0)

#define POINT2_PUSH_BACK(arr, p1, p2) \
    do { \
        lv_fpoint_t * pt = ((lv_fpoint_t *)(arr)->data) + (arr)->size; \
        *pt = *(p1); \
        *(pt + 1) = *(p2); \
        (arr)->size += 2; \
    } while(0)

#define POINT3_PUSH_BACK(arr, p1, p2, p3) \
    do { \
        lv_fpoint_t * pt = ((lv_fpoint_t *)(arr)->data) + (arr)->size; \
        *pt = *(p1); \
        *(pt + 1) = *(p2); \
        *(pt + 2) = *(p3); \
        (arr)->size += 3; \
    } while(0)

/**********************
 *      TYPEDEFS
 **********************/

typedef void (*path_generate_cb)(lv_vector_path_op_t op, const lv_fpoint_t * point, void * data);

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void lv_flatten_quadratic_curve(const lv_fpoint_t * sp, const lv_fpoint_t * cp,
                                const lv_fpoint_t * ep, path_generate_cb cb, void * user_data);

void lv_flatten_cubic_curve(const lv_fpoint_t * sp, const lv_fpoint_t * cp1,
                            const lv_fpoint_t * cp2, const lv_fpoint_t * ep, path_generate_cb cb, void * user_data);

/**
 * Flatten a path with callback.
 * @param path    The input path.
 * @param cb      The path flatten callback.
 * @param user_data The user data for flatten.
 * @return return true if success.
 */
bool lv_vector_path_flatten(const lv_platform_path_base_t * impl, path_generate_cb cb, void * user_data);

/**
 * Flatten a path to a polygon path.
 * @param result  The polygon path.
 * @param path    The input path.
 * @return return true if success.
 */
bool lv_vector_path_to_polygon(lv_vector_path_t * result, const lv_vector_path_t * path);

#endif /* LV_USE_VECTOR_GRAPHIC_OPTIMIZE */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_VECTOR_POLYGON_H */
