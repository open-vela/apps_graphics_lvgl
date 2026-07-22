/**
 * @file lv_draw_sifli_epic.c
 *
 */

/**
 * Copyright 2024 SiFli Technologies
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_sifli_epic.h"

#if LV_USE_SIFLI_EPIC
#include "lv_sifli_epic_utils.h"
#include "lv_sifli_epic_cfg.h"
#include "../../../display/lv_display_private.h"
#include "../../../font/lv_font.h"
#include "../../../misc/lv_log.h"
#include "../../../misc/lv_text.h"

#if defined(__NuttX__) && LV_USE_SIFLI_EPIC_DRAW_THREAD
#error "SiFli EPIC draw thread is not supported on NuttX"
#endif

/*********************
 *      DEFINES
 *********************/

#define DRAW_UNIT_ID_SIFLI_EPIC 11

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * Evaluate a task and set the score and preferred EPIC unit.
 * @param draw_unit Draw unit
 * @param task Draw task
 * @return 1 if task is preferred, 0 otherwise
 */
static int32_t _epic_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);

/**
 * Dispatch a task to the EPIC unit.
 * @param draw_unit Draw unit
 * @param layer Layer to draw
 * @return 1 if task was dispatched, 0 otherwise
 */
static int32_t _epic_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);

/**
 * Delete the EPIC draw unit.
 * @param draw_unit Draw unit
 * @return Always returns 0
 */
static int32_t _epic_delete(lv_draw_unit_t * draw_unit);

#if LV_USE_SIFLI_EPIC_DRAW_THREAD
    /**
    * Render thread callback.
    * @param ptr Pointer to draw unit
    */
    static void _epic_render_thread_cb(void * ptr);
#endif

/**
 * Execute drawing operation.
 * @param u EPIC draw unit
 */
static void _epic_execute_drawing(lv_draw_sifli_epic_unit_t * u);

static lv_layer_t * _epic_get_task_layer(const lv_draw_task_t * task);

/**********************
 *  STATIC VARIABLES
 **********************/

static bool epic_draw_unit_initialized = false;
/**********************
 *      MACROS
 **********************/

/* Vector glyphs are not handled by the EPIC label path below.
 * Keep the check lightweight and fall back only when the first glyph is vector. */
static bool _epic_label_has_vector_glyph(const lv_draw_label_dsc_t * draw_dsc)
{
#if LV_USE_FREETYPE && LV_USE_VECTOR_GRAPHIC && LV_USE_THORVG
    if(draw_dsc == NULL || draw_dsc->font == NULL || draw_dsc->text == NULL || draw_dsc->text[0] == '\0') {
        return false;
    }

    lv_font_glyph_dsc_t glyph_dsc;
    uint32_t txt_ofs = 0;
    uint32_t letter = lv_text_encoded_next(draw_dsc->text, &txt_ofs);

    if(letter == '\0') {
        return false;
    }

    if(lv_font_get_glyph_dsc(draw_dsc->font, &glyph_dsc, letter, '\0') &&
       glyph_dsc.format == LV_FONT_GLYPH_FORMAT_VECTOR) {
        return true;
    }
#else
    LV_UNUSED(draw_dsc);
#endif

    return false;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_sifli_epic_init(void)
{
    if(epic_draw_unit_initialized) {
        return;
    }

    /* Initialize EPIC hardware */
    lv_epic_init();
    if(!lv_epic_is_initialized()) {
        LV_LOG_WARN("SiFli EPIC initialization failed");
        return;
    }

    /* Initialize draw buffer handlers */
    lv_draw_buf_sifli_epic_init_handlers();

    /* Create EPIC draw unit */
    lv_draw_sifli_epic_unit_t * draw_epic_unit = lv_draw_create_unit(sizeof(lv_draw_sifli_epic_unit_t));
    draw_epic_unit->base_unit.evaluate_cb = _epic_evaluate;
    draw_epic_unit->base_unit.dispatch_cb = _epic_dispatch;
    draw_epic_unit->base_unit.delete_cb = _epic_delete;
    draw_epic_unit->base_unit.name = "SiFli_EPIC";

#if LV_USE_SIFLI_EPIC_DRAW_THREAD
    lv_thread_init(&draw_epic_unit->thread, LV_THREAD_PRIO_HIGH,
                   _epic_render_thread_cb, LV_DRAW_THREAD_STACKSIZE, draw_epic_unit);
#endif

    epic_draw_unit_initialized = true;
}

void lv_draw_sifli_epic_deinit(void)
{
    if(!epic_draw_unit_initialized) {
        return;
    }

    lv_epic_deinit();
    epic_draw_unit_initialized = false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_layer_t * _epic_get_task_layer(const lv_draw_task_t * task)
{
    if(task == NULL || task->draw_dsc == NULL) {
        return NULL;
    }

    const lv_draw_dsc_base_t * draw_dsc_base = (const lv_draw_dsc_base_t *)task->draw_dsc;
    return draw_dsc_base->layer;
}

static int32_t _epic_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);

    lv_layer_t * target_layer;

    if(task == NULL) {
        return 0;
    }

    target_layer = _epic_get_task_layer(task);
    if(target_layer == NULL) {
        return 0;
    }

    if(!lv_epic_cf_supported(target_layer->color_format, 0)) {
        return 0;
    }

    switch(task->type) {
        case LV_DRAW_TASK_TYPE_FILL: {
                const lv_draw_fill_dsc_t * draw_dsc = (const lv_draw_fill_dsc_t *)task->draw_dsc;

                if(target_layer == NULL) {
                    return 0;
                }

                /* EPIC doesn't support rounded corners */
                if(draw_dsc->radius != 0) {
                    return 0;
                }

                /* Check gradient support */
                if(draw_dsc->grad.dir != LV_GRAD_DIR_NONE) {
                    if(draw_dsc->grad.dir != LV_GRAD_DIR_HOR && draw_dsc->grad.dir != LV_GRAD_DIR_VER) {
                        return 0;
                    }

                    /* Only support 2-stop gradients */
                    if(draw_dsc->grad.stops_count != 2) {
                        return 0;
                    }

                    /* Check if output format supports gradient */
                    lv_color_format_t dest_cf = target_layer->color_format;
                    uint32_t epic_cf = lv_img_cf_to_epic_cf(dest_cf);
                    if(!EPIC_SUPPROT_OUT_FORMAT(epic_cf)) {
                        return 0;
                    }
                }

                if(task->preference_score > 70) {
                    task->preference_score = 70;
                    task->preferred_draw_unit_id = DRAW_UNIT_ID_SIFLI_EPIC;
                }
                return 1;
            }

        case LV_DRAW_TASK_TYPE_IMAGE: {
                const lv_draw_image_dsc_t * draw_dsc = (const lv_draw_image_dsc_t *)task->draw_dsc;
                bool has_transform = (draw_dsc->rotation != 0 ||
                                      draw_dsc->scale_x != LV_SCALE_NONE ||
                                      draw_dsc->scale_y != LV_SCALE_NONE);

                if(draw_dsc->src == NULL) {
                    return 0;
                }

                if(draw_dsc->clip_radius != 0) {
                    return 0;
                }

                if(draw_dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
                    return 0;
                }

                /* Check if recolor is used */
                bool has_recolor = (draw_dsc->recolor_opa != LV_OPA_TRANSP);
                if(has_recolor) {
                    return 0;
                }

                /* Check color format support */
                if(!lv_epic_cf_supported(draw_dsc->header.cf, draw_dsc->header.flags)) {
                    return 0;
                }

                /* SF32LB52 cannot transform its A8 mask layer.  The image path
                 * stages immutable variable images as interleaved ARGB8565. */
                if((LV_COLOR_FORMAT_RGB565A8 == draw_dsc->header.cf) && has_transform) {
#ifdef CONFIG_LV_USE_SIFLI_EPIC_IMAGE_SRAM_STAGE
                    if(lv_image_src_get_type(draw_dsc->src) != LV_IMAGE_SRC_VARIABLE) {
                        return 0;
                    }

                    const lv_image_dsc_t * src_dsc = (const lv_image_dsc_t *)draw_dsc->src;
                    if(src_dsc->data == NULL ||
                       (src_dsc->header.flags & LV_IMAGE_FLAGS_MODIFIABLE) != 0U) {
                        return 0;
                    }
#else
                    return 0;
#endif
                }

                if(task->preference_score > 80) {
                    task->preference_score = 80;
                    task->preferred_draw_unit_id = DRAW_UNIT_ID_SIFLI_EPIC;
                }
                return 1;
            }

        case LV_DRAW_TASK_TYPE_BORDER: {
                const lv_draw_border_dsc_t * draw_dsc = (const lv_draw_border_dsc_t *)task->draw_dsc;

                if(draw_dsc->radius != 0) {
                    return 0;
                }

                if(task->preference_score > 90) {
                    task->preference_score = 90;
                    task->preferred_draw_unit_id = DRAW_UNIT_ID_SIFLI_EPIC;
                }
                return 1;
            }

#ifdef EPIC_SUPPORT_A8
        case LV_DRAW_TASK_TYPE_LABEL: {
                const lv_draw_label_dsc_t * draw_dsc = (const lv_draw_label_dsc_t *)task->draw_dsc;
                lv_layer_t * target_layer = _epic_get_task_layer(task);

                if(target_layer == NULL) {
                    return 0;
                }

                if(draw_dsc->font == NULL || draw_dsc->text == NULL || draw_dsc->text[0] == '\0') {
                    return 0;
                }

                if(_epic_label_has_vector_glyph(draw_dsc)) {
                    return 0;
                }

                if(!lv_epic_cf_supported(target_layer->color_format, 0)) {
                    return 0;
                }

                if(task->preference_score > 95) {
                    task->preference_score = 95;
                    task->preferred_draw_unit_id = DRAW_UNIT_ID_SIFLI_EPIC;
                }
                return 1;
            }
#endif

        case LV_DRAW_TASK_TYPE_LAYER: {
                const lv_draw_image_dsc_t * draw_dsc = (const lv_draw_image_dsc_t *)task->draw_dsc;
                lv_layer_t * layer_to_draw = (lv_layer_t *)draw_dsc->src;

                if(layer_to_draw == NULL) {
                    return 0;
                }

                if(layer_to_draw->draw_buf == NULL) {
                    return 0;
                }

                /* Check if recolor is used */
                bool has_recolor = (draw_dsc->recolor_opa != LV_OPA_TRANSP);
                if(has_recolor) {
                    return 0;
                }

                /* Check if transform is used */
                bool has_transform = (draw_dsc->rotation != 0 ||
                                      draw_dsc->scale_x != LV_SCALE_NONE ||
                                      draw_dsc->scale_y != LV_SCALE_NONE);

                /* Check color format support */
                if(!lv_epic_cf_supported(layer_to_draw->color_format, 0)) {
                    return 0;
                }

                /* RGB565A8 with transform is not supported */
                if((LV_COLOR_FORMAT_RGB565A8 == layer_to_draw->color_format) && has_transform) {
                    return 0;
                }

                if(task->preference_score > 80) {
                    task->preference_score = 80;
                    task->preferred_draw_unit_id = DRAW_UNIT_ID_SIFLI_EPIC;
                }
                return 1;
            }

        default:
            return 0;
    }

    return 0;
}

static int32_t _epic_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_sifli_epic_unit_t * draw_epic_unit = (lv_draw_sifli_epic_unit_t *)draw_unit;

    /* Try to get a ready to draw task */
    lv_draw_task_t * task = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_SIFLI_EPIC);

    if(task == NULL) {

        return -1;
    }

    /* Return immediately if EPIC is busy */
    if(draw_epic_unit->task_act) {

        return 0;
    }

    /* Let the SW unit draw this task if not preferred for EPIC */
    if(task->preferred_draw_unit_id != DRAW_UNIT_ID_SIFLI_EPIC) {

        return -1;
    }

    /* Allocate buffer for layer */
    if(lv_draw_layer_alloc_buf(layer) == NULL) {
        EPIC_ASSERT_MSG(false, "EPIC: Failed to allocate layer draw buffer");

        return -1;
    }

    /* Mark task as in progress */
    task->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    draw_epic_unit->base_unit.target_layer = layer;
    draw_epic_unit->base_unit.clip_area = &task->clip_area;


#if LV_USE_SIFLI_EPIC_DRAW_THREAD

    draw_epic_unit->task_act = task;
    /* Let the render thread work */
    if(draw_epic_unit->inited) {
        lv_thread_sync_signal(&draw_epic_unit->sync);
    }
#else
    draw_epic_unit->task_act = task;
    /* Execute drawing immediately */
    _epic_execute_drawing(draw_epic_unit);

    draw_epic_unit->task_act->state = LV_DRAW_TASK_STATE_READY;
    draw_epic_unit->task_act = NULL;

    /* Request new dispatching */
    lv_draw_dispatch_request();
#endif

    return 1;
}

static int32_t _epic_delete(lv_draw_unit_t * draw_unit)
{
#if LV_USE_SIFLI_EPIC_DRAW_THREAD
    lv_draw_sifli_epic_unit_t * draw_epic_unit = (lv_draw_sifli_epic_unit_t *)draw_unit;

    draw_epic_unit->exit_status = true;

    if(draw_epic_unit->inited) {
        lv_thread_sync_signal(&draw_epic_unit->sync);
    }

    return lv_thread_delete(&draw_epic_unit->thread);
#else
    LV_UNUSED(draw_unit);
    return 0;
#endif
}

static void _epic_execute_drawing(lv_draw_sifli_epic_unit_t * u)
{
    lv_draw_task_t * task = u->task_act;
    lv_layer_t * layer = ((lv_draw_unit_t *)u)->target_layer;

    if(layer && layer->draw_buf) {
        lv_area_t draw_area;

        if(_lv_area_intersect(&draw_area, &task->_real_area, &task->clip_area)) {
            lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
            lv_draw_buf_flush_cache(layer->draw_buf, &draw_area);
        }
    }

    /* Execute based on task type */
    switch(task->type) {
        case LV_DRAW_TASK_TYPE_BORDER:
            lv_draw_sifli_epic_border(task);
            break;

        case LV_DRAW_TASK_TYPE_LABEL:
            lv_draw_sifli_epic_label(task);
            break;

        case LV_DRAW_TASK_TYPE_FILL:
            lv_draw_sifli_epic_fill(task);
            break;

        case LV_DRAW_TASK_TYPE_IMAGE:
            lv_draw_sifli_epic_img(task);
            break;

        case LV_DRAW_TASK_TYPE_LAYER:
            lv_draw_sifli_epic_layer(task);
            break;

        default:
            break;
    }

    if(layer && layer->draw_buf) {
        lv_area_t draw_area;

        if(_lv_area_intersect(&draw_area, &task->_real_area, &task->clip_area)) {
            lv_area_move(&draw_area, -layer->buf_area.x1, -layer->buf_area.y1);
            lv_draw_buf_invalidate_cache(layer->draw_buf, &draw_area);
        }
    }

}

#if LV_USE_SIFLI_EPIC_DRAW_THREAD
static void _epic_render_thread_cb(void * ptr)
{
    lv_draw_sifli_epic_unit_t * u = ptr;

    lv_thread_sync_init(&u->sync);
    u->inited = true;

    while(1) {
        /* Wait for sync if there is no task set */
        while(u->task_act == NULL) {
            if(u->exit_status) {
                break;
            }
            lv_thread_sync_wait(&u->sync);
        }

        if(u->exit_status) {
            break;
        }

        _epic_execute_drawing(u);

        /* Signal the ready state to dispatcher */
        u->task_act->state = LV_DRAW_TASK_STATE_READY;

        /* Cleanup */
        u->task_act = NULL;

        /* Request new dispatching */
        lv_draw_dispatch_request();
    }

    u->inited = false;
    lv_thread_sync_delete(&u->sync);
}
#endif

#endif /*LV_USE_SIFLI_EPIC*/
