/**
 * @file lv_vector_clipper.h
 *
 */

#ifndef LV_VECTOR_CLIPPER_H
#define LV_VECTOR_CLIPPER_H

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

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    LV_VECTOR_CLIPPER_INTERSECT,
    LV_VECTOR_CLIPPER_UNION,
    LV_VECTOR_CLIPPER_DIFF,
    LV_VECTOR_CLIPPER_XOR,
} lv_vector_clipper_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Flatten a path to a polygon path.
 * @param result  The polygon path.
 * @param path    The input path.
 * @return return true if success.
 */
bool lv_vector_path_to_polygon(lv_vector_path_t * result, const lv_vector_path_t * path);

/**
 * Clip two polygon path with gpc.
 * @param type     The clip type.
 * @param result   The clip result path.
 * @param subject  The subject path.
 * @param clip     The clip path.
 * @return return true if success.
 */
bool lv_vector_path_polygon_clipper(lv_vector_clipper_t type, lv_vector_path_t * result,
                                    const lv_vector_path_t * subject, const lv_vector_path_t * clip);

#endif /* LV_USE_VECTOR_GRAPHIC */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_VECTOR_CLIPPER_H */
