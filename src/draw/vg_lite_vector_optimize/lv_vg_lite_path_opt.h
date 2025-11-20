/**
 * @file lv_vg_lite_path_opt.h
 *
 */

#ifndef LV_VG_LITE_PATH_OPT_H
#define LV_VG_LITE_PATH_OPT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"
#include "../vg_lite/lv_vg_lite_utils.h"

#if LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE

#include "../vector_optimize/lv_draw_vector_private.h"
#include "../vg_lite/lv_vg_lite_path.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_platform_vg_lite_path_t {
    lv_platform_path_base_t base;
    lv_vg_lite_path_t * vg_path;
    lv_vg_lite_path_t * stroke_path_cache;
} lv_platform_vg_lite_path_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_vg_lite_path_add_end(lv_vg_lite_path_t * path);
void lv_vg_lite_path_clear_end(lv_vg_lite_path_t * path);
void lv_vg_lite_path_expand_bounding_box(lv_vg_lite_path_t * path);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DRAW_VG_LITE*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_VG_LITE_PATH_OPT_H*/
