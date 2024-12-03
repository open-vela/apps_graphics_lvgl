/**
 * @file lv_vector_stroke.h
 *
 */

#ifndef LV_VECTOR_STROKE_H
#define LV_VECTOR_STROKE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_vector.h"
#include "lv_vector_polygon.h"

#if LV_USE_VECTOR_GRAPHIC

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Convert a path to a stroke with callback.
 * @param path    The input path.
 * @param stroke_dsc The stroke dst pointer.
 * @param cb      The path to stroke callback.
 * @param user_data The user data for path to stroke.
 * @return return true if success.
 */
bool lv_vector_stroke_generate(const lv_vector_path_t * path, const lv_vector_stroke_dsc_t * stroke_dsc,
                               path_generate_cb cb, void * user_data);

/**
 * Convert a path to a stroke path.
 * @param result   The clip result path.
 * @param path     The source path.
 * @param stroke_dsc The stroke dst pointer.
 * @return return true if success.
 */
bool lv_vector_stroke_to_path(lv_vector_path_t * result, const lv_vector_path_t * path,
                              const lv_vector_stroke_dsc_t * stroke_dsc);

#endif /* LV_USE_VECTOR_GRAPHIC */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_VECTOR_STROKE_H */
