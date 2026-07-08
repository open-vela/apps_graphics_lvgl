/*
 * Copyright (C) 2025-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lv_draw_ge2d.h"
#if LV_USE_DRAW_GE2D

#include "lv_draw_ge2d_utils.h"
#include <math.h>
#include <stdlib.h>

//#define GE2D_IMAGE_DEBUG

#define PI 3.141592653589f
#define SIN(x) (sinf((x)* PI / 180.0f))
#define COS(x) (cosf((x)* PI / 180.0f))

typedef struct {
    int32_t x_in;
    int32_t y_in;
    int32_t x_out;
    int32_t y_out;
    int32_t sinma;
    int32_t cosma;
    int32_t scale_x;
    int32_t scale_y;
    int32_t angle;
    int32_t pivot_x_256;
    int32_t pivot_y_256;
    lv_point_t pivot;
} point_transform_t;

static void ge2d_img_draw_core(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                               const lv_image_decoder_dsc_t *decoder_dsc, lv_draw_image_sup_t *sup,
                               const lv_area_t *img_coords, const lv_area_t *clipped_img_area);
extern void lv_draw_ge2d_scale(lv_draw_unit_t *draw_unit, const lv_draw_sw_blend_dsc_t *blend_dsc, const lv_draw_image_dsc_t *draw_dsc,
                               const lv_area_t *clipped_area, const lv_draw_buf_t *decoded);

#define _draw_info LV_GLOBAL_DEFAULT()->draw_info

void lv_draw_ge2d_layer(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc, const lv_area_t *coords)
{
    lv_layer_t *layer_to_draw = (lv_layer_t *)draw_dsc->src;

    /*It can happen that nothing was draw on a layer and therefore its buffer is not allocated.
     *In this case just return. */
    if (layer_to_draw->draw_buf == NULL) return;

    lv_draw_image_dsc_t new_draw_dsc = *draw_dsc;
    new_draw_dsc.src = layer_to_draw->draw_buf;
    lv_draw_ge2d_img(draw_unit, &new_draw_dsc, coords);
#if LV_USE_LAYER_DEBUG || LV_USE_PARALLEL_DRAW_DEBUG
    lv_area_t area_rot;
    lv_area_copy(&area_rot, coords);
    if(draw_dsc->rotation || draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE) {
        int32_t w = lv_area_get_width(coords);
        int32_t h = lv_area_get_height(coords);

        _lv_image_buf_get_transformed_area(&area_rot, w, h, draw_dsc->rotation, draw_dsc->scale_x, draw_dsc->scale_y,
                                           &draw_dsc->pivot);

        area_rot.x1 += coords->x1;
        area_rot.y1 += coords->y1;
        area_rot.x2 += coords->x1;
        area_rot.y2 += coords->y1;
    }
    lv_area_t draw_area;
    if(!_lv_area_intersect(&draw_area, &area_rot, draw_unit->clip_area)) return;
#endif

#if LV_USE_LAYER_DEBUG
    lv_draw_fill_dsc_t fill_dsc;
    lv_draw_fill_dsc_init(&fill_dsc);
    fill_dsc.color = lv_color_hex(layer_to_draw->color_format == LV_COLOR_FORMAT_ARGB8888 ? 0xff0000 : 0x00ff00);
    fill_dsc.opa = LV_OPA_20;
    lv_draw_sw_fill(draw_unit, &fill_dsc, &area_rot);

    lv_draw_border_dsc_t border_dsc;
    lv_draw_border_dsc_init(&border_dsc);
    border_dsc.color = fill_dsc.color;
    border_dsc.opa = LV_OPA_60;
    border_dsc.width = 2;
    lv_draw_sw_border(draw_unit, &border_dsc, &area_rot);

#endif

#if LV_USE_PARALLEL_DRAW_DEBUG
    uint32_t idx = 0;
    lv_draw_unit_t * draw_unit_tmp = _draw_info.unit_head;
    while(draw_unit_tmp != draw_unit) {
        draw_unit_tmp = draw_unit_tmp->next;
        idx++;
    }

    lv_draw_fill_dsc_t fill_dsc;
    lv_draw_fill_dsc_init(&fill_dsc);
    fill_dsc.color = lv_palette_main(idx % _LV_PALETTE_LAST);
    fill_dsc.opa = LV_OPA_10;
    lv_draw_sw_fill(draw_unit, &fill_dsc, &area_rot);

    lv_draw_border_dsc_t border_dsc;
    lv_draw_border_dsc_init(&border_dsc);
    border_dsc.color = lv_palette_main(idx % _LV_PALETTE_LAST);
    border_dsc.opa = LV_OPA_100;
    border_dsc.width = 2;
    lv_draw_sw_border(draw_unit, &border_dsc, &area_rot);

    lv_point_t txt_size;
    lv_text_get_size(&txt_size, "W", LV_FONT_DEFAULT, 0, 0, 100, LV_TEXT_FLAG_NONE);

    lv_area_t txt_area;
    txt_area.x1 = draw_area.x1;
    txt_area.x2 = draw_area.x1 + txt_size.x - 1;
    txt_area.y2 = draw_area.y2;
    txt_area.y1 = draw_area.y2 - txt_size.y + 1;

    lv_draw_fill_dsc_init(&fill_dsc);
    fill_dsc.color = lv_color_black();
    lv_draw_sw_fill(draw_unit, &fill_dsc, &txt_area);

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", idx);
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_color_make(0x00, 0x00, 0xff);
    label_dsc.text = buf;
    lv_draw_sw_label(draw_unit, &label_dsc, &txt_area);
#endif
}

static inline void ge2d_sync(struct mpp_ge *ge2d_dev)
{
    int ret;

    ret = mpp_ge_emit(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("emit fail");
        return;
    }

    ret = mpp_ge_sync(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("sync fail");
        return;
    }

    return;
}

void lv_draw_ge2d_img(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                      const lv_area_t *coords)
{

    if (lv_image_src_get_type(draw_dsc->src) == LV_IMAGE_SRC_FILE) {
        const char *ptr = lv_fs_get_ext(draw_dsc->src);
        if (!strcmp(ptr, "fake")) {
            int alpha_en;
            unsigned int color;

            {
                char *p = (char *)draw_dsc->src + 3;
                char *n;
                strtol(p, &n, 10);           /* skip width */
                strtol(n + 1, &n, 10);       /* skip height */
                alpha_en = (int)strtol(n + 1, &n, 10);
                color = (unsigned int)strtoul(n + 1, NULL, 16);
            }
            lv_draw_fill_dsc_t fill_dsc;
            lv_draw_fill_dsc_init(&fill_dsc);
            fill_dsc.color.blue =  color & 0xff;
            fill_dsc.color.green =  (color >> 8) & 0xff;
            fill_dsc.color.red =  (color >> 16) & 0xff;
            fill_dsc.opa = (color >> 24) & 0xff;

            lv_area_t clipped_img_area;
            if(!_lv_area_intersect(&clipped_img_area, coords, draw_unit->clip_area)) {
                return;
            }

            _lv_image_buf_get_transformed_area(&clipped_img_area, lv_area_get_width(coords), lv_area_get_height(coords),
                                            draw_dsc->rotation, draw_dsc->scale_x,
                                            draw_dsc->scale_y, &draw_dsc->pivot);
            lv_area_move(&clipped_img_area, coords->x1, coords->y1);
            lv_draw_ge2d_fill_with_blend(draw_unit, &fill_dsc, &clipped_img_area, alpha_en);
            return;
        }
    }

    if(!draw_dsc->tile) {
        _lv_draw_image_normal_helper(draw_unit, draw_dsc, coords, ge2d_img_draw_core);
    } else {
        _lv_draw_image_tiled_helper(draw_unit, draw_dsc, coords, ge2d_img_draw_core);
    }
}

static inline bool yuv_size_is_invalid(int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h)
{
    if (src_w < 8 || src_h < 8 || dst_w < 8 || dst_h < 8) {
        return true;
    }

    return false;
}

static inline bool rgb_size_is_invalid(int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h)
{
    if (src_w < 4 || src_h < 4 || dst_w < 4 || dst_h < 4) {
        return true;
    }

    return false;
}

void lv_draw_ge2d_blit(lv_draw_unit_t *draw_unit, const lv_draw_sw_blend_dsc_t *blend_dsc)
{
    if (blend_dsc->opa <= LV_OPA_MIN)
        return;

    lv_area_t blend_area;

    int32_t src_w = lv_area_get_width(blend_dsc->blend_area);
    int32_t src_h = lv_area_get_height(blend_dsc->blend_area);

    if (!_lv_area_intersect(&blend_area, blend_dsc->blend_area, draw_unit->clip_area)) {
        LV_LOG_ERROR("_lv_area_intersect");
        return;
    }

    lv_layer_t *layer = draw_unit->target_layer;
    lv_color_format_t dst_color_format = layer->color_format;
    uint32_t layer_stride_byte = lv_draw_buf_width_to_stride(lv_area_get_width(&layer->buf_area), dst_color_format);

    int32_t dest_w = lv_area_get_width(&blend_area);
    int32_t dest_h = lv_area_get_height(&blend_area);
    int32_t dest_stride = layer_stride_byte;
    lv_opa_t opa = blend_dsc->opa;
    int32_t src_stride = blend_dsc->src_stride;
    lv_color_format_t src_color_format = blend_dsc->src_color_format;
    const uint8_t *src_buf = blend_dsc->src_buf;
    const uint8_t *dst_buf = layer->draw_buf->data;

    int32_t src_x = blend_area.x1 - blend_dsc->src_area->x1;
    int32_t src_y = blend_area.y1 - blend_dsc->src_area->y1;
    int32_t dst_x = blend_area.x1 - layer->buf_area.x1;
    int32_t dst_y = blend_area.y1 - layer->buf_area.y1;

    struct mpp_ge *ge2d_dev = get_ge2d_device();
    struct ge_bitblt blt = { 0 };

    // set src buf
    if (lv_fmt_is_yuv(src_color_format)) {
         if (yuv_size_is_invalid(dest_w, dest_h, dest_w, dest_h)) {
            LV_LOG_TRACE("invalid w:%d, h:%d", dest_w, dest_h);
            return;
         }

        struct mpp_buf *buf = (struct mpp_buf *)src_buf;
        blt.src_buf.phy_addr[0] = buf->phy_addr[0];
        blt.src_buf.phy_addr[1] = buf->phy_addr[1];
        blt.src_buf.phy_addr[2] = buf->phy_addr[2];
        blt.src_buf.stride[0] = buf->stride[0];
        blt.src_buf.stride[1] = buf->stride[1];
    } else {
        blt.src_buf.phy_addr[0] = (unsigned int)(ulong)src_buf;
        blt.src_buf.stride[0] = src_stride;
    }

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.size.width = src_w;
    blt.src_buf.size.height = src_h;
    blt.src_buf.format = lv_fmt_to_mpp_fmt(src_color_format);;
    blt.src_buf.crop_en = 1;
    blt.src_buf.crop.x = src_x;
    blt.src_buf.crop.y = src_y;
    blt.src_buf.crop.width = dest_w;
    blt.src_buf.crop.height = dest_h;

#ifdef GE2D_IMAGE_DEBUG
    LV_LOG_INFO("blend_area.x1:%d", blend_area.x1);
    LV_LOG_INFO("blend_area.y1:%d", blend_area.y1);
    LV_LOG_INFO("blend_area.x2:%d", blend_area.x2);
    LV_LOG_INFO("blend_area.y2:%d", blend_area.y2);
    LV_LOG_INFO("src_area.x1:%d", blend_dsc->src_area->x1);
    LV_LOG_INFO("src_area.y1:%d", blend_dsc->src_area->y1);
    LV_LOG_INFO("src_area.x2:%d", blend_dsc->src_area->x2);
    LV_LOG_INFO("src_area.y2:%d", blend_dsc->src_area->y2);
    LV_LOG_INFO("src_x:%d", src_x);
    LV_LOG_INFO("src_y:%d", src_x);
    LV_LOG_INFO("src_w:%d", dest_w);
    LV_LOG_INFO("src_h:%d", dest_h);
    LV_LOG_INFO("dst_x:%d", dst_x);
    LV_LOG_INFO("dst_y:%d", dst_y);
    LV_LOG_INFO("dst_w:%d", dest_w);
    LV_LOG_INFO("dst_h:%d", dest_h);
    LV_LOG_INFO("dst_stride:%d", dest_stride);
#endif

    // set dst buf
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (unsigned int)(ulong)dst_buf;
    blt.dst_buf.stride[0] = dest_stride;
    blt.dst_buf.size.width = layer->draw_buf->header.w;
    blt.dst_buf.size.height = layer->draw_buf->header.h;
    blt.dst_buf.format = lv_fmt_to_mpp_fmt(dst_color_format);
    blt.dst_buf.crop_en = 1;
    blt.dst_buf.crop.x = dst_x;;
    blt.dst_buf.crop.y = dst_y;
    blt.dst_buf.crop.width = dest_w;
    blt.dst_buf.crop.height = dest_h;

    if(opa >= LV_OPA_MAX && src_color_format != LV_COLOR_FORMAT_ARGB8888)
        blt.ctrl.alpha_en = 0;
    else
        blt.ctrl.alpha_en = 1;

    blt.ctrl.src_alpha_mode = 2;
    blt.ctrl.src_global_alpha = opa;

#ifdef AIC_GE_DITHER
    if (dst_color_format == LV_COLOR_FORMAT_RGB565) {
        blt.ctrl.dither_en = 1;
        if (blt.ctrl.alpha_en || blt.src_buf.format == MPP_FMT_RGB_565)
            blt.ctrl.dither_en = 0;
    }
#endif

    int ret = mpp_ge_bitblt(ge2d_dev, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail");
        return;
    }

    ge2d_sync(ge2d_dev);
}

static void transform_point_inverse(point_transform_t *t, int32_t xin, int32_t yin, int32_t *xout,
                                    int32_t *yout, int32_t src_w, int32_t src_h)
{
    if(t->angle == 0 && t->scale_x == LV_SCALE_NONE && t->scale_y == LV_SCALE_NONE) {
        *xout = xin * 256;
        *yout = yin * 256;
        return;
    }

    xin -= t->pivot.x;
    yin -= t->pivot.y;

    if (t->angle == 0) {
        *xout = ((int32_t)(xin * 256 * 256 / t->scale_x)) + (t->pivot_x_256);
        *yout = ((int32_t)(yin * 256 * 256 / t->scale_y)) + (t->pivot_y_256);
    } else if (t->scale_x == LV_SCALE_NONE && t->scale_y == LV_SCALE_NONE) {
        *xout = ((t->cosma * xin - t->sinma * yin) >> 2) + (t->pivot_x_256);
        *yout = ((t->sinma * xin + t->cosma * yin) >> 2) + (t->pivot_y_256);
    } else {
        *xout = (((t->cosma * xin - t->sinma * yin) * 256 / t->scale_x) >> 2) + (t->pivot_x_256);
        *yout = (((t->sinma * xin + t->cosma * yin) * 256 / t->scale_y) >> 2) + (t->pivot_y_256);
    }

    *xout = LV_CLAMP(0, (*xout + 128) >> 8,  src_w);
    *yout = LV_CLAMP(0, (*yout + 128) >> 8,  src_h);
}

static void image_buf_get_inverse_transform(lv_area_t *in_area, lv_area_t *out_area, int32_t rotation,
                                            uint16_t scale_x, uint16_t scale_y, const lv_point_t *pivot,
                                            int32_t src_w, int32_t src_h)
{
    int32_t angle_low = 0;
    int32_t angle_high = 0;
    int32_t angle_rem = 0;
    int32_t s1 = 0;
    int32_t s2 = 0;
    int32_t c1 = 0;
    int32_t c2 = 0;
    point_transform_t tr_dsc = { 0 };
    lv_point_t out_p[4] = { 0 };

    tr_dsc.angle = -rotation;
    tr_dsc.scale_x = scale_x;
    tr_dsc.scale_y = scale_y;
    tr_dsc.pivot.x = pivot->x;
    tr_dsc.pivot.y = pivot->y;

    angle_low = tr_dsc.angle / 10;
    angle_high = angle_low + 1;
    angle_rem = tr_dsc.angle  - (angle_low * 10);

    s1 = lv_trigo_sin(angle_low);
    s2 = lv_trigo_sin(angle_high);

    c1 = lv_trigo_sin(angle_low + 90);
    c2 = lv_trigo_sin(angle_high + 90);

    tr_dsc.sinma = (s1 * (10 - angle_rem) + s2 * angle_rem) / 10;
    tr_dsc.cosma = (c1 * (10 - angle_rem) + c2 * angle_rem) / 10;
    tr_dsc.sinma = tr_dsc.sinma >> (LV_TRIGO_SHIFT - 10);
    tr_dsc.cosma = tr_dsc.cosma >> (LV_TRIGO_SHIFT - 10);
    tr_dsc.pivot_x_256 = tr_dsc.pivot.x * 256;
    tr_dsc.pivot_y_256 = tr_dsc.pivot.y * 256;

    transform_point_inverse(&tr_dsc, in_area->x1, in_area->y1,
                            &out_p[0].x, &out_p[0].y, src_w, src_h);
    transform_point_inverse(&tr_dsc, in_area->x2 + 1, in_area->y1,
                            &out_p[1].x, &out_p[1].y, src_w, src_h);
    transform_point_inverse(&tr_dsc, in_area->x1, in_area->y2 + 1,
                            &out_p[2].x, &out_p[2].y, src_w, src_h);
    transform_point_inverse(&tr_dsc, in_area->x2 + 1, in_area->y2 + 1,
                            &out_p[3].x, &out_p[3].y, src_w, src_h);

    out_area->x1 = LV_MIN4(out_p[0].x, out_p[1].x, out_p[2].x, out_p[3].x);
    out_area->x2 = LV_MAX4(out_p[0].x, out_p[1].x, out_p[2].x, out_p[3].x);
    out_area->y1 = LV_MIN4(out_p[0].y, out_p[1].y, out_p[2].y, out_p[3].y);
    out_area->y2 = LV_MAX4(out_p[0].y, out_p[1].y, out_p[2].y, out_p[3].y);
}

void lv_draw_ge2d_transform(lv_draw_unit_t *draw_unit, const lv_draw_sw_blend_dsc_t *blend_dsc,
                            const lv_draw_image_dsc_t *draw_dsc, const lv_area_t *clipped_area, const lv_draw_buf_t *decoded)
{
    int32_t src_w = 0;
    int32_t src_h = 0;
    lv_area_t in_area = { 0 };
    lv_area_t out_area = { 0 };

    lv_layer_t *layer = draw_unit->target_layer;
    lv_color_format_t dst_color_format = layer->color_format;
    uint32_t layer_stride_byte = lv_draw_buf_width_to_stride(lv_area_get_width(&layer->buf_area), dst_color_format);

    lv_area_copy(&in_area, clipped_area);
    lv_area_move(&in_area, -blend_dsc->src_area->x1, -blend_dsc->src_area->y1);

    src_w = lv_area_get_width(blend_dsc->src_area);
    src_h = lv_area_get_height(blend_dsc->src_area);

    image_buf_get_inverse_transform(&in_area, &out_area, draw_dsc->rotation,
                                    draw_dsc->scale_x, draw_dsc->scale_y, &draw_dsc->pivot,
                                    src_w, src_h);

    src_w = lv_area_get_width(&out_area);
    src_h = lv_area_get_height(&out_area);


    int32_t dest_w = lv_area_get_width(clipped_area);
    int32_t dest_h = lv_area_get_height(clipped_area);
    int32_t dest_stride = layer_stride_byte;
    lv_opa_t opa = blend_dsc->opa;
    int32_t src_stride = blend_dsc->src_stride;
    lv_color_format_t src_color_format = blend_dsc->src_color_format;
    const uint8_t *src_buf = blend_dsc->src_buf;
    const uint8_t *dst_buf = layer->draw_buf->data;

    int32_t src_x = out_area.x1;
    int32_t src_y = out_area.y1;
    int32_t dst_x = clipped_area->x1 - layer->buf_area.x1;
    int32_t dst_y = clipped_area->y1 - layer->buf_area.y1;

    if (src_x + src_w > decoded->header.w) {
        src_w = decoded->header.w - src_x;
    }

    if (src_y + src_h > decoded->header.h) {
        src_h = decoded->header.h - src_y;
    }

    struct mpp_ge *ge2d_dev = get_ge2d_device();
    struct ge_bitblt blt = { 0 };
    int32_t angle = draw_dsc->rotation;

    // set src buf
    if (lv_fmt_is_yuv(src_color_format)) {
        if (yuv_size_is_invalid(src_w, src_h, dest_w, dest_h)) {
            LV_LOG_TRACE("invalid src_w:%d, src_h:%d, dst_w:%d, dst_h:%d", src_w, src_h, dest_w, dest_h);
            return;
        }

        struct mpp_buf *buf = (struct mpp_buf  *)src_buf;
        blt.src_buf.phy_addr[0] = buf->phy_addr[0];
        blt.src_buf.phy_addr[1] = buf->phy_addr[1];
        blt.src_buf.phy_addr[2] = buf->phy_addr[2];
        blt.src_buf.stride[0] = buf->stride[0];
        blt.src_buf.stride[1] = buf->stride[1];
    } else {
        if (rgb_size_is_invalid(src_w, src_h, dest_w, dest_h)) {
            LV_LOG_TRACE("invalid src_w:%d, src_h:%d, dst_w:%d, dst_h:%d", src_w, src_h, dest_w, dest_h);
            return;
        }
        blt.src_buf.phy_addr[0] = (unsigned int)(ulong)src_buf;
        blt.src_buf.stride[0] = src_stride;
    }

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.size.width = src_w + src_x;
    blt.src_buf.size.height = src_h + src_y;
    blt.src_buf.format = lv_fmt_to_mpp_fmt(src_color_format);
    blt.src_buf.crop_en = 1;
    blt.src_buf.crop.x = src_x;
    blt.src_buf.crop.y = src_y;
    blt.src_buf.crop.width = src_w;
    blt.src_buf.crop.height = src_h;

    // set dst buf
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (unsigned int)(ulong)dst_buf;
    blt.dst_buf.stride[0] = dest_stride;
    blt.dst_buf.size.width = layer->draw_buf->header.w;
    blt.dst_buf.size.height = layer->draw_buf->header.h;
    blt.dst_buf.format = lv_fmt_to_mpp_fmt(dst_color_format);
    blt.dst_buf.crop_en = 1;
    blt.dst_buf.crop.x = dst_x;;
    blt.dst_buf.crop.y = dst_y;
    blt.dst_buf.crop.width = dest_w;
    blt.dst_buf.crop.height = dest_h;

    if(opa >= LV_OPA_MAX && src_color_format != LV_COLOR_FORMAT_ARGB8888)
        blt.ctrl.alpha_en = 0;
    else
        blt.ctrl.alpha_en = 1;

#ifdef GE2D_IMAGE_DEBUG
    if (angle == 2700 || angle == 1800) {
        LV_LOG_INFO("scale_x:%d, scale_y:%d", draw_dsc->scale_x, draw_dsc->scale_y);
        LV_LOG_INFO("src_x:%d", src_x);
        LV_LOG_INFO("src_y:%d", src_y);
        LV_LOG_INFO("src_w:%d", src_w);
        LV_LOG_INFO("src_h:%d", src_h);
        LV_LOG_INFO("dst_x:%d", dst_x);
        LV_LOG_INFO("dst_y:%d", dst_y);
        LV_LOG_INFO("dst_w:%d", dest_w);
        LV_LOG_INFO("dst_h:%d", dest_h);
    }
#endif

    blt.ctrl.src_alpha_mode = 2;
    blt.ctrl.src_global_alpha = opa;

    while(angle < 0) angle += 3600;
    while(angle >= 3600) angle -= 3600;
    blt.ctrl.flags = angle / 900;

    int ret = mpp_ge_bitblt(ge2d_dev, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail");
        return;
    }

    ge2d_sync(ge2d_dev);
}

void lv_draw_ge2d_rotate_any_degree(lv_draw_unit_t *draw_unit, const lv_draw_sw_blend_dsc_t *blend_dsc,
                                    const lv_draw_image_dsc_t *draw_dsc, const lv_area_t *clipped_area)
{
    int32_t src_w;
    int32_t src_h;
    lv_area_t in_area;

    lv_layer_t *layer = draw_unit->target_layer;
    lv_color_format_t dst_color_format = layer->color_format;
    uint32_t layer_stride_byte = lv_draw_buf_width_to_stride(lv_area_get_width(&layer->buf_area), dst_color_format);

    lv_area_copy(&in_area, clipped_area);
    lv_area_move(&in_area, -blend_dsc->src_area->x1, -blend_dsc->src_area->y1);

    src_w = lv_area_get_width(blend_dsc->src_area);
    src_h = lv_area_get_height(blend_dsc->src_area);

    int32_t dest_w = lv_area_get_width(clipped_area);
    int32_t dest_h = lv_area_get_height(clipped_area);
    int32_t dest_stride = layer_stride_byte;
    lv_opa_t opa = blend_dsc->opa;
    int32_t src_stride = blend_dsc->src_stride;
    lv_color_format_t src_color_format = blend_dsc->src_color_format;
    const uint8_t *src_buf = blend_dsc->src_buf;
    const uint8_t *dst_buf = layer->draw_buf->data;

    int32_t dst_x = clipped_area->x1 - layer->buf_area.x1;
    int32_t dst_y = clipped_area->y1 - layer->buf_area.y1;

    struct mpp_ge *ge2d_dev = get_ge2d_device();
    struct ge_rotation rot = { 0 };
    int32_t angle = draw_dsc->rotation;

    if (rgb_size_is_invalid(src_w, src_h, dest_w, dest_h)) {
        LV_LOG_TRACE("invalid src_w:%d, src_h:%d, dst_w:%d, dst_h:%d", src_w, src_h, dest_w, dest_h);
        return;
    }

    // set src buf
    rot.src_buf.buf_type = MPP_PHY_ADDR;
    rot.src_buf.phy_addr[0] = (unsigned int)(ulong)src_buf;
    rot.src_buf.stride[0] = src_stride;
    rot.src_buf.size.width = src_w;
    rot.src_buf.size.height = src_h;
    rot.src_buf.format = lv_fmt_to_mpp_fmt(src_color_format);

    // src center
    rot.src_rot_center.x = draw_dsc->pivot.x;
    rot.src_rot_center.y = draw_dsc->pivot.y;

    // set dst buf
    rot.dst_buf.buf_type = MPP_PHY_ADDR;
    rot.dst_buf.phy_addr[0] = (unsigned int)(ulong)dst_buf;
    rot.dst_buf.stride[0] = dest_stride;
    rot.dst_buf.size.width = layer->draw_buf->header.w;
    rot.dst_buf.size.height = layer->draw_buf->header.h;
    rot.dst_buf.format = lv_fmt_to_mpp_fmt(dst_color_format);
    rot.dst_buf.crop_en = 1;
    rot.dst_buf.crop.x = dst_x;;
    rot.dst_buf.crop.y = dst_y;
    rot.dst_buf.crop.width = dest_w;
    rot.dst_buf.crop.height = dest_h;

    // dst center
    rot.dst_rot_center.x = blend_dsc->src_area->x1 + draw_dsc->pivot.x - dst_x;
    rot.dst_rot_center.y = blend_dsc->src_area->y1 + draw_dsc->pivot.y - dst_y;

    while(angle < 0) angle += 3600;
    while(angle >= 3600) angle -= 3600;

    // angle
    rot.angle_sin = SIN((float)angle / 10.0f) * 4096;
    rot.angle_cos = COS((float)angle / 10.0f) * 4096;

    if(opa >= LV_OPA_MAX && src_color_format != LV_COLOR_FORMAT_ARGB8888)
        rot.ctrl.alpha_en = 0;
    else
        rot.ctrl.alpha_en = 1;

    rot.ctrl.src_alpha_mode = 2;
    rot.ctrl.src_global_alpha = opa;

    int ret = mpp_ge_rotate(ge2d_dev, &rot);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail");
        return;
    }

    ge2d_sync(ge2d_dev);
}

static void ge2d_img_draw_core(lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                               const lv_image_decoder_dsc_t *decoder_dsc, lv_draw_image_sup_t *sup,
                               const lv_area_t *img_coords, const lv_area_t *clipped_img_area)
{
    lv_draw_sw_blend_dsc_t blend_dsc;
    const lv_draw_buf_t *decoded = decoder_dsc->decoded;
    const uint8_t * src_buf = decoded->data;
    uint32_t img_stride = decoded->header.stride;

    if (draw_dsc->opa <= LV_OPA_MIN)
        return;

    lv_memzero(&blend_dsc, sizeof(lv_draw_sw_blend_dsc_t));
    blend_dsc.opa = draw_dsc->opa;
    blend_dsc.blend_mode = draw_dsc->blend_mode;
    blend_dsc.src_stride = img_stride;
    blend_dsc.src_buf = src_buf;
    blend_dsc.color = draw_dsc->recolor;
    blend_dsc.src_color_format = decoded->header.cf;
    blend_dsc.src_area = img_coords;
    blend_dsc.blend_area = img_coords;

    bool scaled = draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE ? true : false;
    bool transformed = draw_dsc->rotation != 0 || scaled ? true : false;
    bool fix_rotated = draw_dsc->rotation % 900 == 0;

    // mpp buffer already flushed cached, so only flush other type buffer
    if (decoded->header.flags & LV_IMAGE_FLAGS_MODIFIABLE) {
        lv_draw_buf_flush_cache(decoded, NULL);
    }

    if (!transformed) {
        lv_area_t clipped_coords;
        if (!_lv_area_intersect(&clipped_coords, img_coords, draw_unit->clip_area))
            return;

        lv_draw_ge2d_blit(draw_unit, &blend_dsc);
    } else if (fix_rotated || scaled) {
        if ((!lv_fmt_is_yuv(decoded->header.cf)) && scaled && draw_dsc->rotation == 0) {
            lv_draw_ge2d_scale(draw_unit, &blend_dsc, draw_dsc, clipped_img_area, decoded);
        } else {
            lv_draw_ge2d_transform(draw_unit, &blend_dsc, draw_dsc, clipped_img_area, decoded);
        }
    } else {
        lv_draw_ge2d_rotate_any_degree(draw_unit, &blend_dsc, draw_dsc, clipped_img_area);
    }
}

#endif /*LV_USE_DRAW_GE2D*/
