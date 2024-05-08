/**
 * @file lv_api_map_v9_0.h
 *
 */

#ifndef LV_API_MAP_V9_0_H
#define LV_API_MAP_V9_0_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/
#define lv_image_set_align               lv_image_set_inner_align
#define lv_image_get_align               lv_image_get_inner_align
#define lv_snapshot_take_to_buf          lv_snapshot_take_to_draw_buf
#define lv_snapshot_draw_buf_create      lv_snapshot_create_draw_buf

#ifndef LV_DRAW_LAYER_SIMPLE_BUF_SIZE
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE    LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE
#endif

#define lv_draw_buf_align_user           lv_draw_buf_align_ex
#define lv_draw_buf_create_user          lv_draw_buf_create_ex
#define lv_draw_buf_width_to_stride_user lv_draw_buf_width_to_stride_ex
#define lv_draw_buf_dup_user             lv_draw_buf_dup_ex

#define lv_draw_buf_invalidate_cache_user(handlers, drawbuf, area)  lv_draw_buf_invalidate_cache(drawbuf, area)
#define lv_draw_buf_flush_cache_user(handlers, drawbuf, area)       lv_draw_buf_flush_cache(drawbuf, area)
#define lv_draw_buf_destroy_user(handlers, drawbuf)                 lv_draw_buf_destroy(drawbuf)

#define LV_DRAW_BUF_DEFINE               LV_DRAW_BUF_DEFINE_STATIC

/**********************
 *      MACROS
 **********************/

/**********************
 * DEPRECATED FUNCTIONS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_API_MAP_V9_0_H*/
