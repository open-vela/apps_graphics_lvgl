/**
 * @file lv_draw_g2d_img.c
 *
 */

/**
 * Copyright 2020-2023，2024 G2D
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_g2d.h"

#if LV_USE_DRAW_G2D

#include <math.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/* Blit w/ transformation for images w/o opa and alpha channel */
static void _g2d_blit_transform(lv_draw_buf_t * dest_buf, const lv_area_t * dest_area,
                                const lv_draw_buf_t * src_buf, const lv_area_t * src_area,
                                const lv_draw_image_dsc_t * dsc, const lv_area_t * clip_area,
                                const lv_area_t * buf_area);

/* Blit simple w/ opa and alpha channel */
static void _g2d_blit(lv_draw_buf_t * dest_buf, const lv_area_t * dest_area,
                      const lv_draw_buf_t * src_buf, const lv_area_t * src_area,
                      lv_opa_t opa);

/* Blit w/ without alpha channel */
static void _g2d_blit_no_alpha(lv_draw_buf_t * dest_buf, const lv_area_t * dest_area,
                               const lv_draw_buf_t * src_buf, const lv_area_t * src_area,
                               const lv_draw_image_dsc_t * dsc, const lv_area_t * clip_area,
                               const lv_area_t * buf_area);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_g2d_img(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * dsc,
                     const lv_area_t * coords)
{
    if(dsc->opa <= (lv_opa_t)LV_OPA_MIN)
        return;

    lv_layer_t * layer = draw_unit->target_layer;

    lv_area_t blend_area;
    lv_area_t transformed_area;
    lv_area_copy(&transformed_area, coords);
    bool has_transform = dsc->rotation != 0 || dsc->scale_x != LV_SCALE_NONE || dsc->scale_y != LV_SCALE_NONE;
    if(has_transform) {
        int32_t w = lv_area_get_width(coords);
        int32_t h = lv_area_get_height(coords);

        _lv_image_buf_get_transformed_area(&transformed_area, w, h, dsc->rotation, dsc->scale_x, dsc->scale_y, &dsc->pivot);
        transformed_area.x1 += coords->x1;
        transformed_area.y1 += coords->y1;
        transformed_area.x2 += coords->x1;
        transformed_area.y2 += coords->y1;

    }
    if(has_transform)
        lv_area_copy(&blend_area, &transformed_area);
    else if(!_lv_area_intersect(&blend_area, draw_unit->clip_area, &transformed_area))
        return; /*Fully clipped, nothing to do*/

    lv_area_t src_area;
    src_area.x1 = blend_area.x1 - coords->x1;
    src_area.y1 = blend_area.y1 - coords->y1;
    if(!has_transform) {
        src_area.x2 = src_area.x1 + lv_area_get_width(&blend_area) - 1;
        src_area.y2 = src_area.y1 + lv_area_get_height(&blend_area) - 1;
    }
    else {
        src_area.x2 = src_area.x1 + lv_area_get_width(coords) - 1;
        src_area.y2 = src_area.y1 + lv_area_get_height(coords) - 1;

    }

    lv_image_decoder_dsc_t decoder_dsc;
    lv_result_t res = lv_image_decoder_open(&decoder_dsc, dsc->src, NULL);
    if(res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        return;
    }

    lv_color_format_t cf = decoder_dsc.decoded->header.cf;
    bool src_has_alpha = (cf == LV_COLOR_FORMAT_ARGB8888);
    if(dsc->opa < (lv_opa_t)LV_OPA_MAX && LV_COLOR_FORMAT_IS_NV(cf)) {
        LV_LOG_WARN("yuv image can't be blended with alpha");
    }
    if((dsc->opa >= (lv_opa_t)LV_OPA_MAX && !src_has_alpha) || LV_COLOR_FORMAT_IS_NV(cf))
        _g2d_blit_no_alpha(layer->draw_buf, &blend_area,
                           decoder_dsc.decoded, &src_area,
                           dsc, draw_unit->clip_area, &layer->buf_area);
    else {
        if(has_transform) {
            _g2d_blit_transform(layer->draw_buf, &blend_area,
                                decoder_dsc.decoded, &src_area,
                                dsc, draw_unit->clip_area, &layer->buf_area);
        }
        else {
            lv_area_move(&blend_area, -layer->buf_area.x1, -layer->buf_area.y1);
            _g2d_blit(layer->draw_buf, &blend_area,
                      decoder_dsc.decoded, &src_area, dsc->opa);
        }
    }
    lv_image_decoder_close(&decoder_dsc);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline void _get_scale_clip_area(const lv_area_t * src_area, const lv_area_t * dest_area,
                                        const lv_draw_image_dsc_t * dsc, lv_area_t * src_area_clip,
                                        lv_area_t * dest_area_clip)
{
    /* dest_area is the area calculated after scaling the src image
     * dest_area_clip is the area to be drawn after clipping */
    if(_lv_area_is_equal(dest_area_clip, dest_area)) {
        /* No need to clip, just scale the image from beginning */
        lv_area_move(src_area_clip, -src_area->x1, -src_area->y1);
    }
    else {
        /* The src image will be clipped after scaling
         * so, we need to calculate the src_clip area which is the area to be drawn after scaling and clipping */
        if(dsc->scale_y == 0 || dsc->scale_x == 0) return;
        int32_t dest_clip_w = lv_area_get_width(dest_area_clip);
        int32_t dest_clip_h = lv_area_get_height(dest_area_clip);
        src_area_clip->x1 = (dest_area_clip->x1 - dest_area->x1) * LV_SCALE_NONE / dsc->scale_x;
        src_area_clip->y1 = (dest_area_clip->y1 - dest_area->y1) * LV_SCALE_NONE / dsc->scale_y;
        src_area_clip->x2 = src_area_clip->x1 + dest_clip_w * LV_SCALE_NONE / dsc->scale_x - 1;
        src_area_clip->y2 = src_area_clip->y1 + dest_clip_h * LV_SCALE_NONE / dsc->scale_x - 1;
    }
}

static void _g2d_blit_no_alpha(lv_draw_buf_t * dest_buf, const lv_area_t * dest_area,
                               const lv_draw_buf_t * src_buf, const lv_area_t * src_area,
                               const lv_draw_image_dsc_t * dsc, const lv_area_t * clip_area,
                               const lv_area_t * buf_area)
{
    lv_color_format_t src_cf = src_buf->header.cf;
    lv_color_format_t dest_cf = dest_buf->header.cf;
    uint32_t src_stride = src_buf->header.stride;
    uint32_t dest_stride = dest_buf->header.stride;
    uint8_t dest_px_size = lv_color_format_get_size(dest_cf);
    uint8_t src_px_size = lv_color_format_get_size(src_cf);

    lv_area_t src_area_clip, dest_area_clip;
    lv_area_copy(&src_area_clip, src_area);
    lv_area_copy(&dest_area_clip, dest_area);
    int32_t src_h = lv_area_get_height(&src_area_clip);

    bool has_scale = (dsc->scale_x != LV_SCALE_NONE) || (dsc->scale_y != LV_SCALE_NONE);
    if(has_scale) {
        if(!_lv_area_intersect(&dest_area_clip, &dest_area_clip, clip_area)) {
            return;
        }
        _get_scale_clip_area(src_area, dest_area, dsc, &src_area_clip, &dest_area_clip);
    }

    int32_t src_clip_w = lv_area_get_width(&src_area_clip);
    int32_t src_clip_h = lv_area_get_height(&src_area_clip);
    int32_t dest_w = lv_area_get_width(&dest_area_clip);
    int32_t dest_h = lv_area_get_height(&dest_area_clip);

    lv_yuv_buf_t * yuv_buf = LV_COLOR_FORMAT_IS_NV(src_cf) ? (lv_yuv_buf_t *)src_buf->data : NULL;

    g2d_blt_h info;
    memset(&info, 0, sizeof(g2d_blt_h));
    info.flag_h = G2D_BLT_NONE_H;
    info.src_image_h.format = g2d_get_px_format(src_cf);
    info.src_image_h.clip_rect.x = src_area_clip.x1;
    info.src_image_h.clip_rect.y = src_area_clip.y1;
    info.src_image_h.clip_rect.w = src_clip_w;
    info.src_image_h.clip_rect.h = src_clip_h;
    info.src_image_h.width = (yuv_buf &&
                              yuv_buf->semi_planar.y.stride != 0) ? yuv_buf->semi_planar.y.stride : src_buf->header.w;
    info.src_image_h.height = src_buf->header.h;
    info.src_image_h.mode = G2D_PIXEL_ALPHA;
    info.src_image_h.alpha = 255;
    info.src_image_h.color = 0xee8899;
    info.src_image_h.align[0] = 0;
    info.src_image_h.align[1] = 0;
    info.src_image_h.align[2] = 0;
    info.src_image_h.laddr[0] = yuv_buf ? (uintptr_t)(yuv_buf->semi_planar.y.buf) : (uintptr_t)(src_buf->data);
    info.src_image_h.laddr[1] = yuv_buf ? (uintptr_t)(yuv_buf->semi_planar.uv.buf) : (uintptr_t) 0;
    info.src_image_h.laddr[2] = (uintptr_t) 0;
    info.src_image_h.use_phy_addr = 1;

    info.dst_image_h.format = g2d_get_px_format(dest_cf);
    info.dst_image_h.clip_rect.x = dest_area_clip.x1;
    info.dst_image_h.clip_rect.y = dest_area_clip.y1;
    info.dst_image_h.clip_rect.w = dest_w;
    info.dst_image_h.clip_rect.h = dest_h;
    info.dst_image_h.width = dest_buf->header.w;
    info.dst_image_h.height = dest_buf->header.h;
    info.dst_image_h.mode = G2D_PIXEL_ALPHA;
    info.dst_image_h.alpha = 255;
    info.dst_image_h.color = 0xee8899;
    info.dst_image_h.align[0] = 0;
    info.dst_image_h.align[1] = info.dst_image_h.align[0];
    info.dst_image_h.align[2] = info.dst_image_h.align[0];
    info.dst_image_h.laddr[0] = (uintptr_t)(dest_buf->data);
    info.dst_image_h.laddr[1] = (uintptr_t) 0;
    info.dst_image_h.laddr[2] = (uintptr_t) 0;
    info.dst_image_h.use_phy_addr = 1;

    unsigned long dest_vaddr_start = (unsigned long)dest_buf->data + dest_stride * dest_area_clip.y1 + dest_px_size *
                                     dest_area_clip.x1;
    hal_dcache_clean_invalidate(dest_vaddr_start, dest_stride * dest_h);

    if(LV_COLOR_FORMAT_IS_NV(src_cf)) {
        unsigned long src_y_vaddr_start = (unsigned long)yuv_buf->semi_planar.y.buf;
        unsigned long src_uv_vaddr_start = (unsigned long)yuv_buf->semi_planar.uv.buf;
        hal_dcache_clean_invalidate(src_y_vaddr_start, yuv_buf->semi_planar.y.stride * src_h);
        hal_dcache_clean_invalidate(src_uv_vaddr_start, yuv_buf->semi_planar.uv.stride * src_h);
    }
    else {
        unsigned long src_vaddr_start = (unsigned long)src_buf->data + src_stride * src_area_clip.y1 + src_px_size *
                                        src_area_clip.x1;
        hal_dcache_clean_invalidate(src_vaddr_start, src_stride * src_h);
    }

    if(sunxi_g2d_control(G2D_CMD_BITBLT_H, &info) < 0) {
        LV_LOG_WARN("Error: sunxifb_g2d_blit G2D_CMD_BITBLT_H failed/n");
        LV_LOG_WARN(
            "sunxifb_g2d_blit_to_fb src[phy=%p format=%d alpha=%d wh=[%ld %ld] clip=[%ld %ld %ld %ld]] "
            "dst=[phy=%p format=%d wh=[%ld %ld] clip=[%ld %ld %ld %ld]]\n",
            (void *) info.src_image_h.laddr[0], info.src_image_h.format,
            info.src_image_h.alpha, info.src_image_h.width,
            info.src_image_h.height, info.src_image_h.clip_rect.x,
            info.src_image_h.clip_rect.y, info.src_image_h.clip_rect.w,
            info.src_image_h.clip_rect.h, (void *) info.dst_image_h.laddr[0],
            info.dst_image_h.format, info.dst_image_h.width,
            info.dst_image_h.height, info.dst_image_h.clip_rect.x,
            info.dst_image_h.clip_rect.y, info.dst_image_h.clip_rect.w,
            info.dst_image_h.clip_rect.h);
    }
}

static void _g2d_blit_transform(lv_draw_buf_t * dest_buf, const lv_area_t * dest_area,
                                const lv_draw_buf_t * src_buf, const lv_area_t * src_area,
                                const lv_draw_image_dsc_t * dsc, const lv_area_t * clip_area,
                                const lv_area_t * buf_area)
{
    lv_color_format_t src_cf = src_buf->header.cf;
    uint32_t src_stride = src_buf->header.stride;
    int32_t src_w = lv_area_get_width(src_area);
    int32_t src_h = lv_area_get_height(src_area);

    bool has_scale = (dsc->scale_x != LV_SCALE_NONE) || (dsc->scale_y != LV_SCALE_NONE);
    uint8_t src_px_size = lv_color_format_get_size(src_cf);
    if(has_scale) {
        int32_t zoom_w = lv_area_get_width(dest_area);
        int32_t zoom_h = lv_area_get_height(dest_area);
        LV_LOG_INFO("cf:%d, scale:(%ld, %ld), img (w,h):(%d, %d), src: ( %ld, %ld) to dst: ( %ld, %ld)",
                    src_cf, dsc->scale_x, dsc->scale_y, src_buf->header.w, src_buf->header.h,
                    src_w, src_h, zoom_w, zoom_h);

        lv_draw_buf_t * scale_buf = lv_draw_buf_create(zoom_w, zoom_h, src_cf, LV_STRIDE_AUTO);
        if(NULL == scale_buf)
            return ;

        g2d_blt_h info;
        memset(&info, 0, sizeof(g2d_blt_h));
        info.flag_h = G2D_BLT_NONE_H;
        info.src_image_h.format = g2d_get_px_format(src_cf);
        info.src_image_h.clip_rect.x = 0;
        info.src_image_h.clip_rect.y = 0;
        info.src_image_h.clip_rect.w = src_w;
        info.src_image_h.clip_rect.h = src_h;
        info.src_image_h.width = src_buf->header.w;
        info.src_image_h.height = src_h;
        info.src_image_h.mode = G2D_PIXEL_ALPHA;
        info.src_image_h.alpha = 255;
        info.src_image_h.color = 0xee8899;
        info.src_image_h.align[0] = 0;
        info.src_image_h.align[1] = info.src_image_h.align[0];
        info.src_image_h.align[2] = info.src_image_h.align[0];
        info.src_image_h.laddr[0] = (uintptr_t)(src_buf->data);
        info.src_image_h.laddr[1] = (uintptr_t) 0;
        info.src_image_h.laddr[2] = (uintptr_t) 0;
        info.src_image_h.use_phy_addr = 1;

        info.dst_image_h.format = g2d_get_px_format(src_cf);
        info.dst_image_h.clip_rect.x = 0;
        info.dst_image_h.clip_rect.y = 0;
        info.dst_image_h.clip_rect.w = zoom_w;
        info.dst_image_h.clip_rect.h = zoom_h;
        info.dst_image_h.width = zoom_w;
        info.dst_image_h.height = zoom_h;
        info.dst_image_h.mode = G2D_PIXEL_ALPHA;
        info.dst_image_h.alpha = 255;
        info.dst_image_h.color = 0xee8899;
        info.dst_image_h.align[0] = 0;
        info.dst_image_h.align[1] = info.dst_image_h.align[0];
        info.dst_image_h.align[2] = info.dst_image_h.align[0];
        info.dst_image_h.laddr[0] = (uintptr_t)(scale_buf->data);
        info.dst_image_h.laddr[1] = (uintptr_t) 0;
        info.dst_image_h.laddr[2] = (uintptr_t) 0;
        info.dst_image_h.use_phy_addr = 1;

        unsigned long src_vaddr_start = (unsigned long)src_buf->data;
        unsigned long dest_vaddr_start = (unsigned long)scale_buf->data;

        hal_dcache_clean_invalidate(src_vaddr_start, src_stride * src_h);
        hal_dcache_clean_invalidate(dest_vaddr_start, zoom_w * zoom_h * src_px_size);
        if(sunxi_g2d_control(G2D_CMD_BITBLT_H, &info) < 0) {
            LV_LOG_WARN("Error: sunxifb_g2d_blit G2D_CMD_BITBLT_H failed/n");
            LV_LOG_WARN(
                "sunxifb_g2d_blit_to_fb src[phy=%p format=%d alpha=%d wh=[%ld %ld] clip=[%ld %ld %ld %ld]] "
                "dst=[phy=%p format=%d wh=[%ld %ld] clip=[%ld %ld %ld %ld]]\n",
                (void *) info.src_image_h.laddr[0], info.src_image_h.format,
                info.src_image_h.alpha, info.src_image_h.width,
                info.src_image_h.height, info.src_image_h.clip_rect.x,
                info.src_image_h.clip_rect.y, info.src_image_h.clip_rect.w,
                info.src_image_h.clip_rect.h, (void *) info.dst_image_h.laddr[0],
                info.dst_image_h.format, info.dst_image_h.width,
                info.dst_image_h.height, info.dst_image_h.clip_rect.x,
                info.dst_image_h.clip_rect.y, info.dst_image_h.clip_rect.w,
                info.dst_image_h.clip_rect.h);
            lv_draw_buf_destroy(scale_buf);
            return;
        }

        lv_area_t src_area_clip, dest_area_clip;
        lv_area_copy(&dest_area_clip, dest_area);
        if(!_lv_area_intersect(&dest_area_clip, &dest_area_clip, clip_area)) {
            lv_draw_buf_destroy(scale_buf);
            return;
        }
        lv_area_copy(&src_area_clip, &dest_area_clip);
        lv_area_move(&src_area_clip, -dest_area->x1, -dest_area->y1);
        _g2d_blit(dest_buf, &dest_area_clip,
                  scale_buf, &src_area_clip, dsc->opa);
        lv_draw_buf_destroy(scale_buf);
    }

}

static void _g2d_blit(lv_draw_buf_t * dest_buf, const lv_area_t * dest_area,
                      const lv_draw_buf_t * src_buf, const lv_area_t * src_area,
                      lv_opa_t opa)
{
    lv_color_format_t src_cf = src_buf->header.cf;
    lv_color_format_t dest_cf = dest_buf->header.cf;
    uint32_t src_stride = src_buf->header.stride;
    uint32_t dest_stride = dest_buf->header.stride;
    int32_t dest_w = lv_area_get_width(dest_area);
    int32_t dest_h = lv_area_get_height(dest_area);
    int32_t src_w = lv_area_get_width(src_area);
    int32_t src_h = lv_area_get_height(src_area);
    LV_LOG_INFO("src(%d), dest(%d), stride: src(%ld), dest(%ld),"
                "(w, h): ( %ld, %ld) to( %ld, %ld)",
                src_cf, dest_cf, src_stride, dest_stride,
                src_w, src_h, dest_w, dest_h);

    bool src_has_alpha = (src_cf == LV_COLOR_FORMAT_ARGB8888);
    uint8_t src_px_size = lv_color_format_get_size(src_cf);
    uint8_t dest_px_size = lv_color_format_get_size(dest_cf);

    lv_draw_buf_invalidate_cache((lv_draw_buf_t *)src_buf, src_area);

    unsigned long src_vaddr_start = (unsigned long)src_buf->data + src_stride * src_area->y1 + src_px_size * src_area->x1;
    unsigned long dest_vaddr_start = (unsigned long)dest_buf->data + dest_stride * dest_area->y1 + dest_px_size *
                                     dest_area->x1;

    hal_dcache_clean_invalidate(src_vaddr_start, src_stride * src_h);
    hal_dcache_clean_invalidate(dest_vaddr_start, dest_stride * dest_h);
    g2d_bld info;
    lv_memset(&info, 0, sizeof(g2d_bld));
    info.bld_cmd = G2D_BLD_SRCOVER;
    if(opa > LV_OPA_MAX && src_has_alpha) {
        info.src_image[1].mode = G2D_PIXEL_ALPHA;
        info.src_image[1].alpha = 255;
    }
    else if(opa <= LV_OPA_MAX && !src_has_alpha) {
        info.src_image[1].mode = G2D_GLOBAL_ALPHA;
        info.src_image[1].alpha = opa;
    }
    else {
        info.src_image[1].mode = G2D_MIXER_ALPHA;
        info.src_image[1].alpha = opa;
    }
    info.src_image[1].format = g2d_get_px_format(src_cf);
    info.src_image[1].clip_rect.x = src_area->x1;
    info.src_image[1].clip_rect.y = src_area->y1;
    info.src_image[1].clip_rect.w = src_w;
    info.src_image[1].clip_rect.h = src_h;
    info.src_image[1].width = src_buf->header.w;
    info.src_image[1].height = src_buf->header.h;
    info.src_image[1].color = 0xee8899;
    info.src_image[1].align[0] = 0;
    info.src_image[1].align[1] = info.src_image[0].align[0];
    info.src_image[1].align[2] = info.src_image[0].align[0];
    info.src_image[1].laddr[0] = (uintptr_t)(src_buf->data);
    info.src_image[1].laddr[1] = (uintptr_t) 0;
    info.src_image[1].laddr[2] = (uintptr_t) 0;
    info.src_image[1].use_phy_addr = 1;

    info.dst_image.format = g2d_get_px_format(dest_cf);
    info.dst_image.clip_rect.x = dest_area->x1;
    info.dst_image.clip_rect.y = dest_area->y1;
    info.dst_image.clip_rect.w = dest_w;
    info.dst_image.clip_rect.h = dest_h;
    info.dst_image.width = dest_buf->header.w;
    info.dst_image.height = dest_buf->header.h;
    info.dst_image.mode = G2D_PIXEL_ALPHA;
    info.dst_image.alpha = 255;
    info.dst_image.color = 0xee8899;
    info.dst_image.align[0] = 0;
    info.dst_image.align[1] = info.dst_image.align[0];
    info.dst_image.align[2] = info.dst_image.align[0];
    info.dst_image.laddr[0] = (uintptr_t)(dest_buf->data);
    info.dst_image.laddr[1] = (uintptr_t) 0;
    info.dst_image.laddr[2] = (uintptr_t) 0;
    info.dst_image.use_phy_addr = 1;
    /* src_image[1] is the top, src_image[0] is the bottom */
    /* src_image[0] is used as dst_image, no need to malloc a buffer */
    info.src_image[0] = info.dst_image;

    if(sunxi_g2d_control(G2D_CMD_BLD_H, &info) < 0) {
        LV_LOG_WARN("ERROR: sunxifb_g2d_blend G2D_CMD_BLD_H failed\n");
        LV_LOG_WARN(
            "sunxifb_g2d_blend "
            "src=[vir=%p phy=%p cmd=%x format=%d alpha=%d wh=[%ld %ld] clip=[%ld %ld %ld %ld]] "
            "dst=[vir=%p phy=%p format=%d wh=[%ld %ld] clip=[%ld %ld %ld %ld]]\n",
            (void *)info.src_image[1].laddr[0], (void *) info.src_image[1].laddr[0], info.bld_cmd,
            info.src_image[1].format, info.src_image[1].alpha,
            info.src_image[1].width, info.src_image[1].height,
            info.src_image[1].clip_rect.x, info.src_image[1].clip_rect.y,
            info.src_image[1].clip_rect.w, info.src_image[1].clip_rect.h,
            (void *)info.dst_image.laddr[0], (void *) info.dst_image.laddr[0], info.dst_image.format,
            info.dst_image.width, info.dst_image.height,
            info.dst_image.clip_rect.x, info.dst_image.clip_rect.y,
            info.dst_image.clip_rect.w, info.dst_image.clip_rect.h);
        return ;
    }
}

#endif /*LV_USE_DRAW_G2D*/
