/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lv_draw_ge2d.h"
#include "lv_draw_ge2d_utils.h"

#if LV_USE_DRAW_GE2D

#define DRAW_UNIT_ID_GE2D 10

static struct mpp_ge *ge2d_dev = NULL;

static int32_t ge2d_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *task);

static int32_t ge2d_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);

static int32_t ge2d_delete(lv_draw_unit_t * draw_unit);

static void ge2d_execute_drawing(lv_draw_ge2d_unit_t *u);

#if LV_USE_OS
static void ge2d_render_thread_cb(void *ptr);
#endif

#define _draw_info LV_GLOBAL_DEFAULT()->draw_info

static void lv_ge2d_init(void)
{
    ge2d_dev = mpp_ge_open();
    if (!ge2d_dev) {
        LV_LOG_ERROR("open ge device fail");
    }
}

static void lv_ge2d_deinit(void)
{
    if (ge2d_dev) {
        mpp_ge_close(ge2d_dev);
        ge2d_dev = NULL;
    }
}

struct mpp_ge *get_ge2d_device(void)
{
    return ge2d_dev;
}

void lv_draw_ge2d_init(void)
{
    lv_draw_ge2d_unit_t *draw_ge2d_unit = lv_draw_create_unit(sizeof(lv_draw_ge2d_unit_t));
    draw_ge2d_unit->base_unit.evaluate_cb = ge2d_evaluate;
    draw_ge2d_unit->base_unit.dispatch_cb = ge2d_dispatch;
    draw_ge2d_unit->base_unit.delete_cb = LV_USE_OS ? ge2d_delete : NULL;

    lv_ge2d_init();

#if LV_USE_OS
    lv_thread_init(&draw_ge2d_unit->thread, LV_THREAD_PRIO_HIGH, ge2d_render_thread_cb,
                   LV_DRAW_THREAD_STACKSIZE, draw_ge2d_unit);
#endif
}

void lv_draw_ge2d_deinit(void)
{
    lv_ge2d_deinit();
}

void lv_draw_ge2d_rotate(const void *src_buf, void *dest_buf, int32_t src_width, int32_t src_height,
                         int32_t src_stride, int32_t dest_stride, lv_display_rotation_t rotation,
                         lv_color_format_t cf)
{
    struct ge_bitblt blt = { 0 };
    enum mpp_pixel_format fmt = lv_fmt_to_mpp_fmt(cf);

    switch (rotation) {
    case LV_DISPLAY_ROTATION_0:
        blt.ctrl.flags = MPP_ROTATION_0;
        break;
    case LV_DISPLAY_ROTATION_90:
        /* LV_DISP_ROT_90 means display rotate 90 degrees counterclockwise,
         * so set degree to MPP_ROTATION_270
         */
        blt.ctrl.flags = MPP_ROTATION_270;
        break;
    case LV_DISPLAY_ROTATION_180:
        blt.ctrl.flags = MPP_ROTATION_180;
        break;
    case LV_DISPLAY_ROTATION_270:
        blt.ctrl.flags = MPP_ROTATION_90;
        break;
    default:
        break;
    }

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (uint32_t)(ulong)src_buf;
    blt.src_buf.stride[0] = src_stride;
    blt.src_buf.size.width = src_width;
    blt.src_buf.size.height = src_height;
    blt.src_buf.format = fmt;

    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (uint32_t)(ulong)dest_buf;
    blt.dst_buf.stride[0] = dest_stride;
    blt.dst_buf.format = fmt;
    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) {
        blt.dst_buf.size.width = src_height;
        blt.dst_buf.size.height = src_width;
    } else {
        blt.dst_buf.size.width = src_width;
        blt.dst_buf.size.height = src_height;
    }

    int ret = mpp_ge_bitblt(ge2d_dev, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail");
        return;
    }

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

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
static bool inline lv_draw_ge2d_buf_address_valid(lv_draw_buf_t *draw_buf)
{
    if ((uint32_t)(long)draw_buf->data < (uint32_t)0x40000000) {
        return false;
    } else {
        return true;
    }
}
#endif

static bool ge2d_draw_img_supported(const lv_draw_image_dsc_t *draw_dsc)
{
    bool recolor = (draw_dsc->recolor_opa > LV_OPA_MIN);
    bool scale = draw_dsc->scale_x != LV_SCALE_NONE || draw_dsc->scale_y != LV_SCALE_NONE;

    if (recolor)
        return false;

    if (lv_image_src_get_type(draw_dsc->src) >= LV_IMAGE_SRC_SYMBOL)
        return false;

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
    if (lv_image_src_get_type(draw_dsc->src) == LV_IMAGE_SRC_VARIABLE) {
        if (!lv_draw_ge2d_buf_address_valid((lv_draw_buf_t *)draw_dsc->src)) {
            return false;
        }
    }
#endif

    if (draw_dsc->header.w * draw_dsc->header.h < LV_GE2D_FILL_OPA_SIZE_LIMIT) {
        if (lv_image_src_get_type(draw_dsc->src) == LV_IMAGE_SRC_FILE) {
            if (!strcmp(lv_fs_get_ext(draw_dsc->src), "fake")) {
                return true;
            }
        }
        return false;
    }

    if (lv_fmt_is_yuv(draw_dsc->header.cf)) {
        if (draw_dsc->rotation % 900)
            return false;
    }

    if (draw_dsc->rotation % 900 && scale)
        return false;

    // check scale ratio
    if (draw_dsc->scale_x < LV_SCALE_NONE / 16 || draw_dsc->scale_y < LV_SCALE_NONE / 16  ||
        draw_dsc->scale_x > LV_SCALE_NONE * 16 || draw_dsc->scale_y > LV_SCALE_NONE * 16) {
            return false;
    }

    if (draw_dsc->header.cf < LV_COLOR_FORMAT_YUV_START) {
        if (draw_dsc->header.stride % 8) {
            LV_LOG_TRACE("stride:%d", draw_dsc->header.stride);
            return false;
        }
    }

    if (draw_dsc->bitmap_mask_src)
        return false;

    return true;
}

static int32_t ge2d_evaluate(lv_draw_unit_t *u, lv_draw_task_t *t)
{
    LV_UNUSED(u);

    const lv_draw_dsc_base_t *draw_dsc_base = (lv_draw_dsc_base_t *)t->draw_dsc;

    if (!ge2d_dst_fmt_supported(draw_dsc_base->layer->color_format))
        return 0;

    switch(t->type) {
        case LV_DRAW_TASK_TYPE_FILL: {
                const lv_draw_fill_dsc_t * draw_dsc = (lv_draw_fill_dsc_t *) t->draw_dsc;
                lv_draw_buf_t *dest_buf = draw_dsc->base.layer->draw_buf;

                if (!dest_buf)
                    return 0;

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
                if (!lv_draw_ge2d_buf_address_valid(dest_buf))
                    return 0;
#endif
                if ((draw_dsc->radius != 0) || (draw_dsc->grad.dir != (lv_grad_dir_t)LV_GRAD_DIR_NONE))
                    return 0;

                if (t->preference_score > 70) {
                    t->preference_score = 70;
                    t->preferred_draw_unit_id = DRAW_UNIT_ID_GE2D;
                }
                return 0;
            }

        case LV_DRAW_TASK_TYPE_LAYER: {
                const lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *)t->draw_dsc;
                lv_layer_t *layer= (lv_layer_t *)draw_dsc->src;
                lv_draw_buf_t *dest_buf = draw_dsc->base.layer->draw_buf;

                if (!dest_buf)
                    return 0;

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
                if (!lv_draw_ge2d_buf_address_valid(dest_buf))
                    return 0;
#endif
                if (!ge2d_src_fmt_supported(layer->color_format))
                    return 0;

                if (!ge2d_draw_img_supported(draw_dsc))
                    return 0;

                if (t->preference_score > 70) {
                    t->preference_score = 70;
                    t->preferred_draw_unit_id = DRAW_UNIT_ID_GE2D;
                }
                return 0;
            }

        case LV_DRAW_TASK_TYPE_IMAGE: {
                lv_draw_image_dsc_t *draw_dsc = (lv_draw_image_dsc_t *)t->draw_dsc;
                lv_draw_buf_t *dest_buf = draw_dsc->base.layer->draw_buf;

                if (!dest_buf)
                    return 0;

#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
                if (!lv_draw_ge2d_buf_address_valid(dest_buf)) {
                    return 0;
                }
#endif
                if (!ge2d_src_fmt_supported(draw_dsc->header.cf))
                    return 0;

                if (!ge2d_draw_img_supported(draw_dsc))
                    return 0;

                if (t->preference_score > 70) {
                    t->preference_score = 70;
                    t->preferred_draw_unit_id = DRAW_UNIT_ID_GE2D;
                }

                return 0;
            }
        default:
            return 0;
    }

    return 0;
}

static inline void execute_drawing_unit(lv_draw_sw_unit_t *u)
{
    ge2d_execute_drawing(u);

    u->task_act->state = LV_DRAW_TASK_STATE_READY;
    u->task_act = NULL;

    /*The draw unit is free now. Request a new dispatching as it can get a new task*/
    lv_draw_dispatch_request();
}

static int32_t ge2d_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    LV_PROFILER_BEGIN;
    lv_draw_ge2d_unit_t * draw_ge2d_unit = (lv_draw_ge2d_unit_t *) draw_unit;

    /*Return immediately if it's busy with draw task*/
    if (draw_ge2d_unit->task_act) {
        LV_PROFILER_END;
        return 0;
    }

    lv_draw_task_t *t = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_GE2D);

    if (t == NULL || t->preferred_draw_unit_id != DRAW_UNIT_ID_GE2D) {
        LV_PROFILER_END;
        return -1;
    }

    void *buf = lv_draw_layer_alloc_buf(layer);
    if (buf == NULL) {
        LV_PROFILER_END;
        return -1;
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_ge2d_unit->base_unit.target_layer = layer;
    draw_ge2d_unit->base_unit.clip_area = &t->clip_area;
    draw_ge2d_unit->task_act = t;

#if LV_USE_OS
    if(draw_ge2d_unit->inited)
        lv_thread_sync_signal(&draw_ge2d_unit->sync);
#else
    execute_drawing_unit(draw_ge2d_unit);
#endif
    LV_PROFILER_END;
    return 1;
}

static int32_t ge2d_delete(lv_draw_unit_t *draw_unit)
{
#if LV_USE_OS
    lv_draw_ge2d_unit_t * draw_ge2d_unit = (lv_draw_ge2d_unit_t *) draw_unit;

    LV_LOG_INFO("ge2d_delete");
    draw_ge2d_unit->exit_status = true;

    if(draw_ge2d_unit->inited)
        lv_thread_sync_signal(&draw_ge2d_unit->sync);

    return lv_thread_delete(&draw_ge2d_unit->thread);
#else
    LV_UNUSED(draw_unit);
    return 0;
#endif
}

static inline void lv_flush_draw_area(lv_draw_unit_t *draw_unit,
                                       const lv_area_t *draw_area,
                                       lv_layer_t *layer)
{
    lv_area_t clipped_area;
    if (!_lv_area_intersect(&clipped_area, draw_area, draw_unit->clip_area))
        return;

    lv_area_move(&clipped_area, -layer->buf_area.x1, -layer->buf_area.y1);
    lv_draw_buf_flush_cache(layer->draw_buf, &clipped_area);
}

static inline void lv_flush_clip_area(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
    lv_area_t clipped_area;
    lv_area_copy(&clipped_area, draw_unit->clip_area);
    lv_area_move(&clipped_area, -layer->buf_area.x1, -layer->buf_area.y1);
    lv_draw_buf_flush_cache(layer->draw_buf, &clipped_area);
}

static inline void lv_flush_image_area(lv_draw_unit_t *draw_unit,
                                       const lv_draw_image_dsc_t *draw_dsc,
                                       const lv_area_t *draw_area,
                                       lv_layer_t *layer)
{
    if (draw_dsc->tile)
        lv_flush_clip_area(draw_unit, layer);
    else
        lv_flush_draw_area(draw_unit, draw_area, layer);
}

static void ge2d_execute_drawing(lv_draw_ge2d_unit_t *u)
{
    lv_draw_task_t *t = u->task_act;
    lv_draw_unit_t *draw_unit = (lv_draw_unit_t *)u;
    lv_layer_t *layer = draw_unit->target_layer;

    switch (t->type) {
        case LV_DRAW_TASK_TYPE_FILL:
            lv_flush_draw_area(draw_unit, &t->area, layer);
            lv_draw_ge2d_fill(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_IMAGE:
            lv_flush_image_area(draw_unit, t->draw_dsc, &t->area, layer);
            lv_draw_ge2d_img(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_LAYER:
            lv_flush_clip_area(draw_unit, layer);
            lv_draw_ge2d_layer(draw_unit, t->draw_dsc, &t->area);
            break;
        default:
            break;
    }

#if LV_USE_PARALLEL_DRAW_DEBUG
    /*Layers manage it for themselves*/
    if (t->type != LV_DRAW_TASK_TYPE_LAYER) {
        lv_area_t draw_area;
        if (!_lv_area_intersect(&draw_area, &t->area, u->base_unit.clip_area))
            return;

        int32_t idx = 0;
        lv_draw_unit_t * draw_unit_tmp = _draw_info.unit_head;
        while (draw_unit_tmp != (lv_draw_unit_t *)u) {
            draw_unit_tmp = draw_unit_tmp->next;
            idx++;
        }
        lv_draw_fill_dsc_t fill_dsc;
        lv_draw_fill_dsc_init(&fill_dsc);
        fill_dsc.color = lv_palette_main(idx % _LV_PALETTE_LAST);
        fill_dsc.opa = LV_OPA_10;
        lv_draw_sw_fill((lv_draw_unit_t *)u, &fill_dsc, &draw_area);

        lv_draw_border_dsc_t border_dsc;
        lv_draw_border_dsc_init(&border_dsc);
        border_dsc.color = fill_dsc.color;
        border_dsc.opa = LV_OPA_80;
        border_dsc.width = 1;
        lv_draw_sw_border((lv_draw_unit_t *)u, &border_dsc, &draw_area);

        lv_point_t txt_size;
        lv_text_get_size(&txt_size, "W", LV_FONT_DEFAULT, 0, 0, 100, LV_TEXT_FLAG_NONE);

        lv_area_t txt_area;
        txt_area.x1 = draw_area.x1;
        txt_area.y1 = draw_area.y1;
        txt_area.x2 = draw_area.x1 + txt_size.x - 1;
        txt_area.y2 = draw_area.y1 + txt_size.y - 1;

        lv_draw_fill_dsc_init(&fill_dsc);
        fill_dsc.color = lv_color_white();
        lv_draw_sw_fill((lv_draw_unit_t *)u, &fill_dsc, &txt_area);

        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", idx);
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = lv_color_make(0x00, 0x00, 0xff);
        label_dsc.text = buf;
        lv_draw_sw_label((lv_draw_unit_t *)u, &label_dsc, &txt_area);
    }
#endif
}

#if LV_USE_OS
static void ge2d_render_thread_cb(void *ptr)
{
    lv_draw_ge2d_unit_t *u = ptr;

    lv_thread_sync_init(&u->sync);
    u->inited = true;

    while(1) {
        while (u->task_act == NULL) {
            if(u->exit_status)
                break;

            lv_thread_sync_wait(&u->sync);
        }

        if (u->exit_status) {
            LV_LOG_INFO("Ready to exit ge2d draw thread.");
            break;
        }
        execute_drawing_unit(u);
    }

    u->inited = false;
    lv_thread_sync_delete(&u->sync);
    LV_LOG_INFO("Exit ge2d draw thread.");
}
#endif

#endif /*LV_USE_DRAW_GE2D*/
