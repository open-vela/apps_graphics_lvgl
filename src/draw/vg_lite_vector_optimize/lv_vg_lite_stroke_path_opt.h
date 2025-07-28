/**
 * @file lv_vg_lite_stroke_path_opt.h
 *
 */

#ifndef LV_VG_LITE_STROKE_PATH_OPT_H
#define LV_VG_LITE_STROKE_PATH_OPT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../vg_lite/lv_vg_lite_utils.h"

#if LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE
#include "lv_vg_lite_path_opt.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_draw_vg_lite_unit_t;
struct _lv_vg_lite_path_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize the stroke path module
 * @param unit pointer to the unit
 */
void lv_vg_lite_stroke_path_init(struct _lv_draw_vg_lite_unit_t * unit);

/**
 * @brief Deinitialize the stroke path module
 * @param unit pointer to the unit
 */
void lv_vg_lite_stroke_path_deinit(struct _lv_draw_vg_lite_unit_t * unit);

/**
 * @brief Get the stroke path
 * @param impl pointer to the vector path
 * @param dsc pointer to the vector stroke descriptor
 * @return pointer to the path of the stroke
 */
struct _lv_vg_lite_path_t * lv_vg_lite_stroke_path_get(lv_platform_vg_lite_path_t * impl,
                                                       const lv_vector_stroke_dsc_t * dsc);

/**
 * @brief Drop the stroke path
 * @param unit pointer to the unit
 * @param path pointer to the stroke path
 */
void lv_vg_lite_stroke_path_drop(struct _lv_draw_vg_lite_unit_t * unit, struct _lv_vg_lite_path_t * path);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_VG_LITE_STROKE_PATH_OPT_H*/
