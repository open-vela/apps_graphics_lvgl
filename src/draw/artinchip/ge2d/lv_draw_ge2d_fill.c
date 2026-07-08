/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lv_draw_ge2d.h"

#if LV_USE_DRAW_GE2D
#include "lv_draw_ge2d_utils.h"

void lv_draw_ge2d_fill_with_blend(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
                                  const lv_area_t *coords, int32_t alpha_en)
{
    lv_area_t blend_area;
    if (!_lv_area_intersect(&blend_area, coords, draw_unit->clip_area))
        return;

    lv_layer_t *layer = draw_unit->target_layer;
    lv_draw_buf_t *draw_buf = layer->draw_buf;
    lv_area_move(&blend_area, -layer->buf_area.x1, -layer->buf_area.y1);

    int32_t blend_width = lv_area_get_width(&blend_area);
    int32_t blend_height = lv_area_get_height(&blend_area);
    lv_coord_t dest_width = lv_area_get_width(&layer->buf_area);
    lv_coord_t dest_height = lv_area_get_height(&layer->buf_area);

    struct ge_fillrect fill = { 0 };
    struct mpp_ge *ge2d_device = get_ge2d_device();

    /* fill info */
    fill.type = GE_NO_GRADIENT;

    if (dsc->opa >= LV_OPA_MAX) {
        fill.start_color = lv_color_to_u32(dsc->color);
    } else {
        fill.start_color = dsc->color.blue | (dsc->color.green << 8)
                          | (dsc->color.red << 16) | (dsc->opa << 24);
    }

    fill.end_color = 0;

    /* dst buf */
    fill.dst_buf.buf_type = MPP_PHY_ADDR;
    fill.dst_buf.phy_addr[0] = (int32_t)(long)draw_buf->data;
    fill.dst_buf.stride[0] = draw_buf->header.stride;
    fill.dst_buf.size.width = dest_width;
    fill.dst_buf.size.height = dest_height;
    fill.dst_buf.format = lv_fmt_to_mpp_fmt(draw_buf->header.cf);
    fill.dst_buf.crop_en = 1;
    fill.dst_buf.crop.x = blend_area.x1;
    fill.dst_buf.crop.y = blend_area.y1;
    fill.dst_buf.crop.width = blend_width;
    fill.dst_buf.crop.height = blend_height;

    /* ctrl */
    if (alpha_en)
        fill.ctrl.alpha_en = 1;

    fill.ctrl.src_alpha_mode = 0;

    int ret = mpp_ge_fillrect(ge2d_device, &fill);
    if (ret < 0) {
        LV_LOG_WARN("fillrect1 fail");
        return;
    }
    ret = mpp_ge_emit(ge2d_device);
    if (ret < 0) {
        LV_LOG_WARN("emit fail");
        return;
    }
    ret = mpp_ge_sync(ge2d_device);
    if (ret < 0) {
        LV_LOG_WARN("sync fail");
        return;
    }
}

void lv_draw_ge2d_fill(lv_draw_unit_t *draw_unit, const lv_draw_fill_dsc_t *dsc,
                       const lv_area_t *coords)
{
    int32_t alpha_en;
    if (dsc->opa <= (lv_opa_t)LV_OPA_MIN)
        return;

    if (dsc->opa < LV_OPA_MAX)
        alpha_en = 1;
    else
        alpha_en = 0;

    lv_draw_ge2d_fill_with_blend(draw_unit, dsc, coords, alpha_en);
}

#endif /*LV_USE_DRAW_GE2D*/
