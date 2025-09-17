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
#include "lv_draw_vector.h"

#if LV_USE_VECTOR_GRAPHIC

/*********************
 *      DEFINES
 *********************/
#define LV_VECTOR_POLYGON_STOP 0xF

/**********************
 *      TYPEDEFS
 **********************/

typedef void (*path_generate_cb)(lv_vector_path_op_t op, const lv_fpoint_t * point, void * data);

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Flatten a path with callback.
 * @param path    The input path.
 * @param cb      The path flatten callback.
 * @param user_data The user data for flatten.
 * @return return true if success.
 */
bool lv_vector_path_flatten(const lv_vector_path_t * path, path_generate_cb cb, void * user_data);

/**
 * Flatten a path to a polygon path.
 * @param result  The polygon path.
 * @param path    The input path.
 * @return return true if success.
 */
bool lv_vector_path_to_polygon(lv_vector_path_t * result, const lv_vector_path_t * path);

#endif /* LV_USE_VECTOR_GRAPHIC */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_VECTOR_POLYGON_H */
