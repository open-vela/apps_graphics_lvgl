/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include "lv_draw_ge2d.h"
#if LV_USE_DRAW_GE2D

#include "lv_draw_ge2d_utils.h"
#include <math.h>

#define GE2D_DEBUG_ENABLE 0

#if defined(AIC_CHIP_D12X) ||defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
#define CONFIG_GE2D_SPLIT_OPT
#endif

#if GE2D_DEBUG_ENABLE
#define GE2D_DEBUG_PRINTF(fmt, ...) LV_LOG_USER(fmt, ##__VA_ARGS__)
#else
#define GE2D_DEBUG_PRINTF(fmt, ...) \
    do {                            \
    } while (0)
#endif

#define MAX_TIMES 256

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

static void swap_int32(int32_t *a, int32_t *b)
{
    int32_t temp = *a;
    *a = *b;
    *b = temp;
}

static int ge2d_sync(struct mpp_ge *ge2d_dev)
{
    int ret;

    ret = mpp_ge_emit(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("emit fail");
        return ret;
    }

    ret = mpp_ge_sync(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("sync fail");
        return ret;
    }

    return ret;
}

static void rotate_area(lv_area_t *area, int32_t pivot_x, int32_t pivot_y, int32_t rotation)
{
    if (rotation == 0) {
        return;
    }

    /* Calculate coordinates relative to rotation center */
    int32_t x1_rel = area->x1 - pivot_x + 1;
    int32_t y1_rel = area->y1 - pivot_y + 1;
    int32_t x2_rel = area->x2 - pivot_x + 1;
    int32_t y2_rel = area->y2 - pivot_y + 1;

    int32_t new_x1, new_y1, new_x2, new_y2;

    /* Transform coordinates according to rotation angle */
    switch (rotation) {
        case 900: /* 90 degrees clockwise */
            /* (x,y) -> (y, -x) */
            new_x1 = y1_rel;
            new_y1 = -x2_rel;
            new_x2 = y2_rel;
            new_y2 = -x1_rel;
            break;

        case 1800: /* 180 degrees */
            /* (x,y) -> (-x, -y) */
            new_x1 = -x2_rel;
            new_y1 = -y2_rel;
            new_x2 = -x1_rel;
            new_y2 = -y1_rel;
            break;

        case 2700: /* 270 degrees clockwise (or 90 degrees counterclockwise) */
            /* (x,y) -> (-y, x) */
            new_x1 = -y2_rel;
            new_y1 = x1_rel;
            new_x2 = -y1_rel;
            new_y2 = x2_rel;
            break;

        default:
            /* Not a multiple of 90 degrees, do nothing */
            return;
    }

    /* Convert back to absolute coordinates */
    area->x1 = new_x1 + pivot_x - 1;
    area->y1 = new_y1 + pivot_y - 1;
    area->x2 = new_x2 + pivot_x - 1;
    area->y2 = new_y2 + pivot_y - 1;

    /* Ensure x1<=x2, y1<=y2 */
    if (area->x1 > area->x2) {
        swap_int32(&area->x1, &area->x2);
    }
    if (area->y1 > area->y2) {
        swap_int32(&area->y1, &area->y2);
    }
}

static void config_scale(struct ge_bitblt *blt)
{
    int channel_num;
    int i;
    int dx[2] = { 0 };
    int dy[2] = { 0 };
    int h_phase[2] = { 0 };
    int v_phase[2] = { 0 };
    int in_w_ch0 = blt->src_buf.crop.width;
    int in_h_ch0 = blt->src_buf.crop.height;

    dx[0] = blt->scale_phase.dx_16[0];
    dy[0] = blt->scale_phase.dy_16[0];
    h_phase[0] = blt->scale_phase.h_phase_16[0];
    v_phase[0] = blt->scale_phase.v_phase_16[0];

    switch (blt->src_buf.format) {
        case MPP_FMT_ARGB_8888:
        case MPP_FMT_ABGR_8888:
        case MPP_FMT_RGBA_8888:
        case MPP_FMT_BGRA_8888:
        case MPP_FMT_XRGB_8888:
        case MPP_FMT_XBGR_8888:
        case MPP_FMT_RGBX_8888:
        case MPP_FMT_BGRX_8888:
        case MPP_FMT_RGB_888:
        case MPP_FMT_BGR_888:
        case MPP_FMT_ARGB_1555:
        case MPP_FMT_ABGR_1555:
        case MPP_FMT_RGBA_5551:
        case MPP_FMT_BGRA_5551:
        case MPP_FMT_RGB_565:
        case MPP_FMT_BGR_565:
        case MPP_FMT_ARGB_4444:
        case MPP_FMT_ABGR_4444:
        case MPP_FMT_RGBA_4444:
        case MPP_FMT_BGRA_4444:
            /* RGB format - single channel */
            channel_num = 1;
            break;

        case MPP_FMT_YUV400:
            /* YUV400 - single channel grayscale */
            channel_num = 1;
            break;

        case MPP_FMT_YUV420P:
        case MPP_FMT_NV12:
        case MPP_FMT_NV21:
            /* YUV420 - 2 channels */
            channel_num = 2;
            dx[0] = dx[0] & (~1); /* Align to even number */
            dy[0] = dy[0] & (~1);
            h_phase[0] = h_phase[0] & (~1);
            v_phase[0] = v_phase[0] & (~1);

            blt->scale_phase.in_w_ch1 = in_w_ch0 >> 1;
            blt->scale_phase.in_h_ch1 = in_h_ch0 >> 1;

            /* UV component (half of Y) */
            dx[1] = dx[0] >> 1;
            dy[1] = dy[0] >> 1;
            h_phase[1] = h_phase[0] >> 1;
            v_phase[1] = v_phase[0] >> 1;
            break;

        case MPP_FMT_YUV422P:
        case MPP_FMT_NV16:
        case MPP_FMT_NV61:
        case MPP_FMT_YUYV:
        case MPP_FMT_YVYU:
        case MPP_FMT_UYVY:
        case MPP_FMT_VYUY:
            /* YUV422 - 2 channels */
            channel_num = 2;
            dx[0] = dx[0] & (~1); /* Horizontal alignment */
            h_phase[0] = h_phase[0] & (~1);

            blt->scale_phase.in_w_ch1 = in_w_ch0 >> 1;
            blt->scale_phase.in_h_ch1 = in_h_ch0;

            /* UV component (horizontal halved, vertical unchanged) */
            dx[1] = dx[0] >> 1;
            dy[1] = dy[0];
            h_phase[1] = h_phase[0] >> 1;
            v_phase[1] = v_phase[0];
            break;

        case MPP_FMT_YUV444P:
            /* YUV444 - 2 channels */
            channel_num = 2;
            dx[1] = dx[0];
            dy[1] = dy[0];
            h_phase[1] = h_phase[0];
            v_phase[1] = v_phase[0];
            blt->scale_phase.in_w_ch1 = in_w_ch0;
            blt->scale_phase.in_h_ch1 = in_h_ch0;
            break;

        default:
            return;
    }

    /* Set scaling parameters */
    blt->scale_phase.channel_num = channel_num;
    blt->scale_phase.scaler_en = 1;

    for (i = 0; i < channel_num; i++) {
        blt->scale_phase.dx_16[i] = dx[i];
        blt->scale_phase.dy_16[i] = dy[i];
        blt->scale_phase.h_phase_16[i] = h_phase[i];
        blt->scale_phase.v_phase_16[i] = v_phase[i];
    }
}

static int calculate_crop_area(int32_t *src_start_x_16, int32_t *src_start_y_16,
                               int32_t *dest_crop_x, int32_t *dest_crop_y,
                               int32_t *dest_w, int32_t *dest_h,
                               int32_t dx_16, int32_t dy_16,
                               int32_t src_w, int32_t src_h,
                               int32_t *src_crop_x, int32_t *src_crop_y,
                               int32_t *needed_input_w, int32_t *needed_input_h)
{
    int32_t src_start_x = *src_start_x_16;
    int32_t src_start_y = *src_start_y_16;
    int32_t d_crop_x = *dest_crop_x;
    int32_t d_crop_y = *dest_crop_y;
    int32_t d_w = *dest_w;
    int32_t d_h = *dest_h;
    int max_times = 0;
    int32_t need_w_fixed_16;
    int32_t need_h_fixed_16;

    while (src_start_x < 0 && max_times < MAX_TIMES) {
        src_start_x += dx_16;
        d_crop_x += 1;
        d_w -=1;
        max_times++;
    }

    max_times = 0;
    while (src_start_y < 0 && max_times < MAX_TIMES) {
        src_start_y += dy_16;
        d_crop_y += 1;
        d_h -=1;
        max_times++;
    }

    need_w_fixed_16 = src_start_x + (d_w - 1) * dx_16;
    need_h_fixed_16 = src_start_y + (d_h - 1) * dy_16;

    max_times = 0;
    while ((((need_w_fixed_16 + 0xffff) >> 16) > src_w - 1) && max_times < MAX_TIMES) {
        need_w_fixed_16 -= dx_16;
        d_w -= 1;
        max_times++;
    }

    max_times = 0;
    while ((((need_h_fixed_16 + 0xffff) >> 16) > src_h - 1) && max_times < MAX_TIMES) {
        need_h_fixed_16 -= dy_16;
        d_h -= 1;
        max_times++;
    }

    /* Starting position (integer part) */
    *src_crop_x = src_start_x >> 16;
    *src_crop_y = src_start_y >> 16;

    *needed_input_w = ((need_w_fixed_16 + 0xffff) >> 16) - *src_crop_x;
    *needed_input_h = ((need_h_fixed_16 + 0xffff) >> 16) - *src_crop_y;

    *src_start_x_16 = src_start_x;
    *src_start_y_16 = src_start_y;

    *dest_w = d_w;
    *dest_h = d_h;
    *dest_crop_x = d_crop_x;
    *dest_crop_y = d_crop_y;

    /* Add starting position offset */
#if defined(AIC_CHIP_D12X)
    *needed_input_w += 1;
    *needed_input_h += 1;
#else
    *needed_input_w += 2;
    *needed_input_h += 2;
#endif

    return 0;
}

#ifdef CONFIG_GE2D_SPLIT_OPT
static void calculate_split_params(int32_t dx_16, int32_t dest_w, int32_t src_start_x_16,
                                   int32_t src_w, int32_t src_crop_x, int32_t *dst_w_sec,
                                   int32_t *src_w_sec, int32_t *phase_x_sec, int32_t *offset_x_sec,
                                   int32_t *adjusted_dest_w, int32_t *needed_input_w)
{
    *dst_w_sec = 0;
    *src_w_sec = 0;
    *phase_x_sec = 0;
    *offset_x_sec = 0;
    *adjusted_dest_w = dest_w;

    if (dx_16 < 65536 && dx_16 > 65536 / 32 * 29 && dest_w >= 32) {
        int begin_phase = src_start_x_16 & 0xFFFF;
        int next_phase = begin_phase;
        int blk_cnt = 0;
        int out_num = 0;
        int split_flag = 0;
        int final_phase = (src_start_x_16 & 0xFFFF) + dx_16 * (dest_w - 1);

        for (int i = 0; i < dest_w; i++) {
            next_phase += dx_16;
            if ((((next_phase >> 16) - (begin_phase >> 16)) < 31) && next_phase < final_phase) {
                blk_cnt++;
            } else {
                blk_cnt++;
                begin_phase = next_phase;
                if (out_num + 32 == dest_w) {
                    split_flag = 1;
                    break;
                }
                out_num += blk_cnt;
                blk_cnt = 0;
            }
        }

        if (split_flag) {
            *dst_w_sec = 16;
            *adjusted_dest_w = dest_w - *dst_w_sec;

            *needed_input_w = ((src_start_x_16 & 0xffff) + (*adjusted_dest_w - 1) * dx_16 + 0xffff) >> 16;
#if defined(AIC_CHIP_D12X)
            *needed_input_w += 1;
#else
            *needed_input_w += 2;
#endif
            *phase_x_sec = (src_start_x_16 + dx_16 * (*adjusted_dest_w)) & 0xffff;
            *offset_x_sec = (src_start_x_16 + dx_16 * (*adjusted_dest_w)) >> 16;
            *src_w_sec =  (((*phase_x_sec + dx_16 * (*dst_w_sec - 1)) + 0xffff) >> 16);
#if defined(AIC_CHIP_D12X)
            *src_w_sec += 1;
#else
            *src_w_sec += 2;
#endif
        }
    }
}

static int handle_secondary_block(struct mpp_ge *ge2d_dev, struct ge_bitblt *blt,
                                  int32_t src_crop_x, int32_t src_crop_y,
                                  int32_t needed_input_h, int32_t dest_w, int32_t dst_x,
                                  int32_t dst_y, int32_t dest_h, int32_t dx_16,
                                  int32_t dy_16, int32_t phase_x_sec, int32_t src_start_y_16,
                                  int32_t dst_w_sec, int32_t src_w_sec, int32_t offset_x_sec,
                                  lv_draw_unit_t *draw_unit, const lv_draw_image_dsc_t *draw_dsc,
                                  const lv_draw_sw_blend_dsc_t *blend_dsc)
{
    int ret;

    blt->src_buf.crop.x = offset_x_sec;
    blt->src_buf.crop.y = src_crop_y;
    blt->src_buf.crop.width = src_w_sec;
    blt->src_buf.crop.height = needed_input_h;
    blt->dst_buf.crop.x = dst_x + dest_w;
    blt->dst_buf.crop.y = dst_y;
    blt->dst_buf.crop.width = dst_w_sec;
    blt->dst_buf.crop.height = dest_h;
    blt->scale_phase.dx_16[0] = dx_16;
    blt->scale_phase.dy_16[0] = dy_16;
    blt->scale_phase.h_phase_16[0] = phase_x_sec;
    blt->scale_phase.v_phase_16[0] = src_start_y_16 & 0xFFFF;

    if (blt->src_buf.crop.width < 4 || blt->src_buf.crop.height < 4 ||
         blt->dst_buf.crop.width < 4 || blt->dst_buf.crop.height < 4) {
        GE2D_DEBUG_PRINTF("Secondary block min size requirement not met. Src: %dx%d, Dst: %dx%d. Falling back to software.",
                          blt->src_buf.crop.width, blt->src_buf.crop.height,
                          blt->dst_buf.crop.width, blt->dst_buf.crop.height);
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return -1;
    }
    config_scale(blt);

    ret = mpp_ge_bitblt(ge2d_dev, blt);
    if (ret < 0) {
        GE2D_DEBUG_PRINTF("ERROR: mpp_ge_bitblt for secondary block failed with ret=%d\n", ret);
        return ret;
    }

    return 0;
}
#endif
void lv_draw_ge2d_scale(lv_draw_unit_t *draw_unit, const lv_draw_sw_blend_dsc_t *blend_dsc, const lv_draw_image_dsc_t *draw_dsc,
                        const lv_area_t *clipped_area, const lv_draw_buf_t *decoded)
{
    int32_t src_w = 0;
    int32_t src_h = 0;
    int rotation = 0;
    lv_layer_t *layer = NULL;
    lv_color_format_t dst_color_format = 0;
    uint32_t layer_stride_byte = 0;
    lv_area_t dest_area = { 0 };
    int32_t dest_w = 0;
    int32_t dest_h = 0;
    int32_t scale_x_8 = 0;
    int32_t scale_y_8 = 0;
    int32_t inv_scale_x_16 = 0;
    int32_t inv_scale_y_16 = 0;
    int32_t src_start_x_16 = 0;
    int32_t src_start_y_16 = 0;
    int32_t dx_16 = 0;
    int32_t dy_16 = 0;
    lv_opa_t opa = 0;
    int32_t src_stride = 0;
    lv_color_format_t src_color_format = 0;
    const uint8_t *src_buf = NULL;
    const uint8_t *dst_buf = NULL;
    int32_t dst_x = 0;
    int32_t dst_y = 0;
    struct mpp_ge *ge2d_dev = NULL;
    struct ge_bitblt blt = { 0 };
    int32_t src_crop_x = 0;
    int32_t src_crop_y = 0;
    int32_t needed_input_w = 0;
    int32_t needed_input_h = 0;
    int ret = 0;

    /* This version does not support YUV format */
    /* To scale normally, fall back to software rendering */
    if (lv_fmt_is_yuv(blend_dsc->src_color_format)) {
        GE2D_DEBUG_PRINTF("Source is YUV format\n");
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return;
    }

    src_w = lv_area_get_width(blend_dsc->src_area);
    src_h = lv_area_get_height(blend_dsc->src_area);
    rotation = draw_dsc->rotation % 3600;
    if (rotation < 0)
        rotation += 3600;
    layer = draw_unit->target_layer;
    dst_color_format = layer->color_format;
    layer_stride_byte = layer->draw_buf->header.stride;

    lv_area_copy(&dest_area, clipped_area);

    /* Revert the target area to pre-rotation state for easier calculation */
    rotate_area(&dest_area, draw_dsc->pivot.x + blend_dsc->src_area->x1,
                draw_dsc->pivot.y + blend_dsc->src_area->y1, -rotation);

    lv_area_move(&dest_area, -blend_dsc->src_area->x1, -blend_dsc->src_area->y1);

    dest_w = lv_area_get_width(&dest_area);
    dest_h = lv_area_get_height(&dest_area);

    GE2D_DEBUG_PRINTF("Input Params:\n");
    GE2D_DEBUG_PRINTF("  src_w=%d, src_h=%d\n", (int)src_w, (int)src_h);
    GE2D_DEBUG_PRINTF("  scale_x=%d, scale_y=%d\n", (int)draw_dsc->scale_x, (int)draw_dsc->scale_y);
    GE2D_DEBUG_PRINTF("  pivot.x=%d, pivot.y=%d\n", (int)draw_dsc->pivot.x, (int)draw_dsc->pivot.y);
    GE2D_DEBUG_PRINTF("Source Area: x1=%d, y1=%d, x2=%d, y2=%d\n", (int)blend_dsc->src_area->x1,
                      (int)blend_dsc->src_area->y1, (int)blend_dsc->src_area->x2, (int)blend_dsc->src_area->y2);
    GE2D_DEBUG_PRINTF("  opa=%d\n", blend_dsc->opa);
    GE2D_DEBUG_PRINTF("Dest Area: x1=%d, y1=%d, x2=%d, y2=%d\n", (int)dest_area.x1, (int)dest_area.y1,
                      (int)dest_area.x2, (int)dest_area.y2);

    /* Scaling ratio (8.8 fixed point) */
    scale_x_8 = draw_dsc->scale_x;
    scale_y_8 = draw_dsc->scale_y;

    /* Inverse scaling ratio (16.16 fixed point) */
    inv_scale_x_16 = (256 * 256 * 256) / scale_x_8;
    inv_scale_y_16 = (256 * 256 * 256) / scale_y_8;

    src_start_x_16 =
        ((dest_area.x1 - draw_dsc->pivot.x) * inv_scale_x_16) + (draw_dsc->pivot.x << 16);
    src_start_y_16 =
        ((dest_area.y1 - draw_dsc->pivot.y) * inv_scale_y_16) + (draw_dsc->pivot.y << 16);

    GE2D_DEBUG_PRINTF("Scale Factors: scale_x_8=%d, scale_y_8=%d\n", (int)scale_x_8, (int)scale_y_8);
    GE2D_DEBUG_PRINTF("Inverse Scale: inv_scale_x_16=%d, inv_scale_y_16=%d\n", (int)inv_scale_x_16,
                      (int)inv_scale_y_16);
    GE2D_DEBUG_PRINTF("Source Start before: src_start_x_16=%d, src_start_y_16=%d\n", (int)src_start_x_16,
                      (int)src_start_y_16);

    /* Step size (16.16 fixed point) */
    dx_16 = inv_scale_x_16;
    dy_16 = inv_scale_y_16;

    GE2D_DEBUG_PRINTF("Source Start after: src_start_x_16=%d, src_start_y_16=%d\n", (int)src_start_x_16,
                      (int)src_start_y_16);
    GE2D_DEBUG_PRINTF("Source Start Integer: x=%d, y=%d\n", (int)src_start_x_16 >> 16,
                      (int)src_start_y_16 >> 16);
    GE2D_DEBUG_PRINTF("Step Size: dx_16=%d, dy_16=%d\n", (int)dx_16, (int)dy_16);

    opa = blend_dsc->opa;
    src_stride = blend_dsc->src_stride;
    src_color_format = blend_dsc->src_color_format;
    src_buf = blend_dsc->src_buf;
    dst_buf = layer->draw_buf->data;

    dst_x = clipped_area->x1 - layer->buf_area.x1;
    dst_y = clipped_area->y1 - layer->buf_area.y1;

    GE2D_DEBUG_PRINTF("  dest_area=%d, dest_area->y1=%d\n", (int)dest_area.x1, (int)dest_area.y1);
    GE2D_DEBUG_PRINTF("  layer->buf_area.x1=%d, layer->buf_area.y1=%d\n", (int)layer->buf_area.x1,
                      (int)layer->buf_area.y1);
    GE2D_DEBUG_PRINTF("Destination: dst_x=%d, dst_y=%d\n", (int)dst_x, (int)dst_y);
    GE2D_DEBUG_PRINTF("Source Color Format: %d\n", (int)src_color_format);
    GE2D_DEBUG_PRINTF("Destination Color Format: %d\n", (int)dst_color_format);

    /* Calculate crop area */
    calculate_crop_area(&src_start_x_16, &src_start_y_16, &dst_x, &dst_y, &dest_w, &dest_h,
                        dx_16, dy_16, decoded->header.w, decoded->header.h, &src_crop_x, &src_crop_y,
                        &needed_input_w, &needed_input_h);

    ge2d_dev = get_ge2d_device();
    if (!ge2d_dev) {
        GE2D_DEBUG_PRINTF("ERROR: GE2D device is NULL\n");
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return;
    }

    if (lv_fmt_is_yuv(src_color_format)) {
        GE2D_DEBUG_PRINTF("Source is YUV format\n");
        if (yuv_size_is_invalid(src_w, src_h, dest_w, dest_h)) {
            GE2D_DEBUG_PRINTF("ERROR: YUV size invalid: src_w=%d, src_h=%d, dst_w=%d, dst_h=%d\n",
                              (int)src_w, (int)src_h, (int)dest_w, (int)dest_h);
            lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
            return;
        }

        struct mpp_buf *buf = (struct mpp_buf *)src_buf;
        blt.src_buf.phy_addr[0] = buf->phy_addr[0];
        blt.src_buf.phy_addr[1] = buf->phy_addr[1];
        blt.src_buf.phy_addr[2] = buf->phy_addr[2];
        blt.src_buf.stride[0] = buf->stride[0];
        blt.src_buf.stride[1] = buf->stride[1];
    } else {
        GE2D_DEBUG_PRINTF("Source is RGB format\n");
        if (rgb_size_is_invalid(src_w, src_h, dest_w, dest_h)) {
            GE2D_DEBUG_PRINTF("ERROR: RGB size invalid: src_w=%d, src_h=%d, dst_w=%d, dst_h=%d\n",
                              (int)src_w, (int)src_h, (int)dest_w, (int)dest_h);
            lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
            return;
        }

        blt.src_buf.phy_addr[0] = (unsigned int)(ulong)src_buf;
        blt.src_buf.stride[0] = src_stride;
    }

    blt.scale_phase.scale_phase_en = 1;
    blt.scale_phase.dx_16[0] = dx_16;
    blt.scale_phase.dy_16[0] = dy_16;
    blt.scale_phase.h_phase_16[0] = src_start_x_16 & 0xFFFF;
    blt.scale_phase.v_phase_16[0] = src_start_y_16 & 0xFFFF;

    GE2D_DEBUG_PRINTF("Scale Phase Params:\n");
    GE2D_DEBUG_PRINTF("  dx_16[0]=%d\n", blt.scale_phase.dx_16[0]);
    GE2D_DEBUG_PRINTF("  dy_16[0]=%d\n", blt.scale_phase.dy_16[0]);
    GE2D_DEBUG_PRINTF("  h_phase_16[0]=%d\n", blt.scale_phase.h_phase_16[0]);
    GE2D_DEBUG_PRINTF("  v_phase_16[0]=%d\n", blt.scale_phase.v_phase_16[0]);

    src_w = needed_input_w + src_crop_x;
#ifdef CONFIG_GE2D_SPLIT_OPT
    int32_t dst_w_sec = 0;
    int32_t src_w_sec = 0;
    int32_t phase_x_sec = 0;
    int32_t offset_x_sec = 0;

    calculate_split_params(dx_16, dest_w, src_start_x_16, decoded->header.w, src_crop_x,
                           &dst_w_sec, &src_w_sec, &phase_x_sec, &offset_x_sec,
                           &dest_w, &needed_input_w);

#endif

    /* Ensure width and height are valid */
    if (needed_input_w < 4 || needed_input_h < 4) {
        GE2D_DEBUG_PRINTF("ERROR: Invalid needed input size: w=%d, h=%d\n", (int)needed_input_w,
                          (int)needed_input_h);
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return;
    }

    if (rgb_size_is_invalid(src_w, src_h, dest_w, dest_h)) {
        GE2D_DEBUG_PRINTF("invalid src_w:%d, src_h:%d, dst_w:%d, dst_h:%d", (int)src_w,(int) src_h, (int)dest_w, (int)dest_h);
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return;
    }

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.format = lv_fmt_to_mpp_fmt(src_color_format);
    blt.src_buf.crop_en = 1;
    blt.src_buf.size.width = needed_input_w + src_crop_x;
    blt.src_buf.size.height = needed_input_h + src_crop_y;
    blt.src_buf.crop.x = src_crop_x;
    blt.src_buf.crop.y = src_crop_y;
    blt.src_buf.crop.width = needed_input_w;
    blt.src_buf.crop.height = needed_input_h;

    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (unsigned int)(ulong)dst_buf;
    blt.dst_buf.stride[0] = layer_stride_byte;
    blt.dst_buf.size.width = layer->draw_buf->header.w;
    blt.dst_buf.size.height = layer->draw_buf->header.h;
    blt.dst_buf.format = lv_fmt_to_mpp_fmt(dst_color_format);
    blt.dst_buf.crop_en = 1;
    blt.dst_buf.crop.x = dst_x;
    blt.dst_buf.crop.y = dst_y;
    blt.dst_buf.crop.width = dest_w;
    blt.dst_buf.crop.height = dest_h;

    if (rotation % 1800)
        swap_int32((int32_t*)&blt.dst_buf.crop.width, (int32_t*)&blt.dst_buf.crop.height);

    /* Set alpha blending */
    if (opa >= LV_OPA_MAX && src_color_format != LV_COLOR_FORMAT_ARGB8888) {
        blt.ctrl.alpha_en = 0;
        GE2D_DEBUG_PRINTF("Alpha disabled (opa=%d)\n", (int)opa);
    } else {
        blt.ctrl.alpha_en = 1;
        GE2D_DEBUG_PRINTF("Alpha enabled (opa=%d)\n", (int)opa);
    }

    blt.ctrl.src_alpha_mode = 2;
    blt.ctrl.src_global_alpha = opa;
    blt.ctrl.flags = rotation / 900;

    GE2D_DEBUG_PRINTF(
        "Control Params: alpha_en=%d, src_alpha_mode=%d, src_global_alpha=%d, flags=%d\n",
        blt.ctrl.alpha_en, blt.ctrl.src_alpha_mode, blt.ctrl.src_global_alpha, blt.ctrl.flags);

    /* Check if within valid range */
    if (src_crop_x >= 0 && src_crop_y >= 0 && (src_crop_x + needed_input_w) <= decoded->header.w + 1 &&
        (src_crop_y + needed_input_h) <= decoded->header.h + 1) {
        GE2D_DEBUG_PRINTF("  Coordinates are within source bounds - OK\n");
    } else {
        LV_LOG_ERROR("  ERROR: Coordinates are outside source bounds!\n");
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return;
    }

    GE2D_DEBUG_PRINTF("blt.src_buf.size.width=%d\n", blt.src_buf.size.width);
    GE2D_DEBUG_PRINTF("blt.src_buf.size.height=%d\n", blt.src_buf.size.height);
    GE2D_DEBUG_PRINTF("blt.src_buf.format=%d\n", blt.src_buf.format);
    GE2D_DEBUG_PRINTF("blt.src_buf.crop_en=%d\n", blt.src_buf.crop_en);
    GE2D_DEBUG_PRINTF("blt.src_buf.crop.x=%d\n", blt.src_buf.crop.x);
    GE2D_DEBUG_PRINTF("blt.src_buf.crop.y=%d\n", blt.src_buf.crop.y);
    GE2D_DEBUG_PRINTF("blt.src_buf.crop.width=%d\n", blt.src_buf.crop.width);
    GE2D_DEBUG_PRINTF("blt.src_buf.crop.height=%d\n", blt.src_buf.crop.height);

    GE2D_DEBUG_PRINTF("blt.dst_buf.stride[0]=%d\n", blt.dst_buf.stride[0]);
    GE2D_DEBUG_PRINTF("blt.dst_buf.size.width=%d\n", blt.dst_buf.size.width);
    GE2D_DEBUG_PRINTF("blt.dst_buf.size.height=%d\n", blt.dst_buf.size.height);
    GE2D_DEBUG_PRINTF("blt.dst_buf.crop.x=%d\n", blt.dst_buf.crop.x);
    GE2D_DEBUG_PRINTF("blt.dst_buf.crop.y=%d\n", blt.dst_buf.crop.y);
    GE2D_DEBUG_PRINTF("blt.dst_buf.crop.width=%d\n", blt.dst_buf.crop.width);
    GE2D_DEBUG_PRINTF("blt.dst_buf.crop.height=%d\n", blt.dst_buf.crop.height);

    config_scale(&blt);

    ret = mpp_ge_bitblt(ge2d_dev, &blt);

    if (ret < 0) {
        GE2D_DEBUG_PRINTF("ERROR: mpp_ge_bitblt failed with ret=%d\n", (int)ret);
        lv_draw_sw_image(draw_unit, draw_dsc, blend_dsc->blend_area);
        return;
    }

#ifdef CONFIG_GE2D_SPLIT_OPT
    if (dst_w_sec) {
        blt.src_buf.size.width = src_w;
        handle_secondary_block(ge2d_dev, &blt, src_crop_x, src_crop_y,
                               needed_input_h, dest_w, dst_x, dst_y, dest_h,
                               dx_16, dy_16, phase_x_sec, src_start_y_16,
                               dst_w_sec, src_w_sec, offset_x_sec,
                               draw_unit, draw_dsc, blend_dsc);
    }
#endif

    ret = ge2d_sync(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("ge2d_sync\n");
        return;
    }

}

#endif /*LV_USE_DRAW_GE2D*/
