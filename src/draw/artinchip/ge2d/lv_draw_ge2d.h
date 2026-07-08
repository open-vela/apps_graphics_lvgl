/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_DRAW_GE2D_H
#define LV_DRAW_GE2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "mpp_ge.h"

#ifndef LV_GE2D_FILL_SIZE_LIMIT
#define LV_GE2D_FILL_SIZE_LIMIT 100 * 100
#endif

#ifndef LV_GE2D_FILL_OPA_SIZE_LIMIT
#define LV_GE2D_FILL_OPA_SIZE_LIMIT 100 * 100
#endif

#ifndef LV_GE2D_BLIT_SIZE_LIMIT
#define LV_GE2D_BLIT_SIZE_LIMIT 100 * 100
#endif

#ifndef LV_GE2D_BLIT_OPA_SIZE_LIMIT
#define LV_GE2D_BLIT_OPA_SIZE_LIMIT 40 * 40
#endif

typedef lv_draw_sw_unit_t lv_draw_ge2d_unit_t;

struct mpp_ge *get_ge2d_device(void);

void lv_draw_ge2d_init(void);

void lv_draw_ge2d_deinit(void);

void lv_draw_ge2d_rotate(const void *src_buf, void *dest_buf, int32_t src_width, int32_t src_height,
                         int32_t src_stride, int32_t dest_stride, lv_display_rotation_t rotation,
                         lv_color_format_t cf);

void lv_draw_ge2d_fill_with_blend(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
                                  const lv_area_t *coords, int32_t alpha_en);

void lv_draw_ge2d_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
                       const lv_area_t *coords);

void lv_draw_ge2d_img(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *dsc,
                      const lv_area_t *coords);

void lv_draw_ge2d_layer(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                        const lv_area_t *coords);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DRAW_GE2D_H*/
