/**
 * @file lv_draw_sifli_epic_label.c
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
#include "../../../draw/lv_draw_label.h"
#include "../../../draw/lv_draw_rect.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t draw_unit;
    lv_draw_buf_t * glyph_stage[2];
    uint8_t next_stage;
} epic_label_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void draw_letter_cb(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc,
                           lv_draw_fill_dsc_t * fill_draw_dsc, const lv_area_t * fill_area);
static void draw_placeholder_border(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc);
static void draw_bitmap_glyph(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc);
static void draw_glyph_as_image(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc);
static void draw_fill_part(lv_draw_unit_t * draw_unit, lv_draw_fill_dsc_t * fill_draw_dsc, const lv_area_t * fill_area);
static bool create_task_from_draw_unit(lv_draw_unit_t * draw_unit, lv_draw_task_t * task, void * draw_dsc,
                                       const lv_area_t * area);
static lv_draw_buf_t * stage_glyph_bitmap(epic_label_ctx_t * ctx, const lv_draw_buf_t * glyph_buf);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_sifli_epic_label(lv_draw_task_t * task)
{
    const lv_draw_label_dsc_t * dsc = (const lv_draw_label_dsc_t *)task->draw_dsc;

    if(dsc->opa <= LV_OPA_MIN || dsc->text == NULL || dsc->text[0] == '\0') {
        return;
    }

#ifndef EPIC_SUPPORT_A8
    LV_UNUSED(task);
#else
    const lv_draw_dsc_base_t * draw_dsc_base = (const lv_draw_dsc_base_t *)task->draw_dsc;
    lv_layer_t * layer = draw_dsc_base->layer;

    if(layer == NULL) {
        return;
    }

    epic_label_ctx_t ctx = {0};
    ctx.draw_unit.target_layer = layer;
    ctx.draw_unit.clip_area = &task->clip_area;

    lv_draw_label_iterate_characters(&ctx.draw_unit, dsc, &task->area, draw_letter_cb);
    (void)lv_epic_cont_blend_reset();

    for(uint32_t i = 0; i < 2U; i++) {
        if(ctx.glyph_stage[i] != NULL) {
            lv_draw_buf_destroy(ctx.glyph_stage[i]);
        }
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void draw_letter_cb(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc,
                           lv_draw_fill_dsc_t * fill_draw_dsc, const lv_area_t * fill_area)
{
    if(glyph_draw_dsc) {
        switch(glyph_draw_dsc->format) {
            case LV_FONT_GLYPH_FORMAT_NONE: {
#if LV_USE_FONT_PLACEHOLDER
                    draw_placeholder_border(draw_unit, glyph_draw_dsc);
#endif
                    break;
                }

            case LV_FONT_GLYPH_FORMAT_A1:
            case LV_FONT_GLYPH_FORMAT_A2:
            case LV_FONT_GLYPH_FORMAT_A4:
            case LV_FONT_GLYPH_FORMAT_A8:
                if(glyph_draw_dsc->rotation % 3600 == 0) {
                    draw_bitmap_glyph(draw_unit, glyph_draw_dsc);
                }
                else {
                    draw_glyph_as_image(draw_unit, glyph_draw_dsc);
                }
                break;

            case LV_FONT_GLYPH_FORMAT_IMAGE:
                draw_glyph_as_image(draw_unit, glyph_draw_dsc);
                break;

            default:
                break;
        }
    }

    if(fill_draw_dsc && fill_area) {
        draw_fill_part(draw_unit, fill_draw_dsc, fill_area);
    }
}

static void draw_placeholder_border(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc)
{
    if(glyph_draw_dsc->bg_coords == NULL) {
        return;
    }

    lv_draw_border_dsc_t border_draw_dsc;
    lv_draw_border_dsc_init(&border_draw_dsc);
    border_draw_dsc.opa = glyph_draw_dsc->opa;
    border_draw_dsc.color = glyph_draw_dsc->color;
    border_draw_dsc.width = 1;
    border_draw_dsc.base.layer = draw_unit->target_layer;

    lv_draw_task_t border_task;
    if(!create_task_from_draw_unit(draw_unit, &border_task, &border_draw_dsc, glyph_draw_dsc->bg_coords)) {
        return;
    }

    lv_draw_sifli_epic_border(&border_task);
}

static void draw_bitmap_glyph(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc)
{
    if(glyph_draw_dsc->opa <= LV_OPA_MIN || glyph_draw_dsc->g == NULL || glyph_draw_dsc->letter_coords == NULL) {
        return;
    }

    if(glyph_draw_dsc->glyph_data == NULL) {
        return;
    }

    const lv_draw_buf_t * glyph_buf = (const lv_draw_buf_t *)glyph_draw_dsc->glyph_data;
    if(glyph_buf->data == NULL) {
        EPIC_ASSERT_MSG(false, "EPIC: Bitmap glyph buffer has no data");
        return;
    }

    epic_label_ctx_t * ctx = (epic_label_ctx_t *)draw_unit;
    lv_draw_buf_t * staged_buf = stage_glyph_bitmap(ctx, glyph_buf);
    if(staged_buf == NULL) {
        (void)lv_epic_cont_blend_reset();
        staged_buf = (lv_draw_buf_t *)glyph_buf;
    }

    EPIC_LayerConfigTypeDef bg_layer;
    EPIC_LayerConfigTypeDef output_layer;

    lv_draw_fill_dsc_t layer_dsc;
    lv_draw_fill_dsc_init(&layer_dsc);
    layer_dsc.base.layer = draw_unit->target_layer;

    lv_draw_task_t task;
    if(!create_task_from_draw_unit(draw_unit, &task, &layer_dsc, glyph_draw_dsc->letter_coords)) {
        return;
    }

    if(lv_epic_setup_layers(&bg_layer, &output_layer, &task, glyph_draw_dsc->letter_coords)) {
        return;
    }

    EPIC_LayerConfigTypeDef input_layers[2];
    input_layers[0] = bg_layer;
    HAL_EPIC_LayerConfigInit(&input_layers[1]);

    input_layers[1].data = staged_buf->data;
    input_layers[1].color_mode = lv_img_cf_to_epic_cf(staged_buf->header.cf);
    input_layers[1].width = staged_buf->header.w;
    input_layers[1].height = staged_buf->header.h;
    input_layers[1].total_width = lv_epic_stride_to_width(staged_buf->header.stride, staged_buf->header.cf);
    input_layers[1].x_offset = glyph_draw_dsc->letter_coords->x1;
    input_layers[1].y_offset = glyph_draw_dsc->letter_coords->y1;
    input_layers[1].alpha = glyph_draw_dsc->opa;
    input_layers[1].color_en = true;
    input_layers[1].color_r = glyph_draw_dsc->color.red;
    input_layers[1].color_g = glyph_draw_dsc->color.green;
    input_layers[1].color_b = glyph_draw_dsc->color.blue;
    input_layers[1].ax_mode = ALPHA_BLEND_RGBCOLOR;

    if(lv_epic_cont_blend(input_layers, 2, &output_layer) != HAL_OK) {
        (void)lv_epic_blend(input_layers, 2, &output_layer);
    }
}

static void draw_glyph_as_image(lv_draw_unit_t * draw_unit, lv_draw_glyph_dsc_t * glyph_draw_dsc)
{
    if(glyph_draw_dsc->opa <= LV_OPA_MIN || glyph_draw_dsc->g == NULL || glyph_draw_dsc->letter_coords == NULL) {
        return;
    }

    if(glyph_draw_dsc->glyph_data == NULL) {
        return;
    }

    lv_draw_image_dsc_t img_dsc;
    lv_draw_image_dsc_init(&img_dsc);
    img_dsc.rotation = glyph_draw_dsc->rotation;
    img_dsc.scale_x = LV_SCALE_NONE;
    img_dsc.scale_y = LV_SCALE_NONE;
    img_dsc.opa = glyph_draw_dsc->opa;
    img_dsc.src = glyph_draw_dsc->glyph_data;
    img_dsc.recolor = glyph_draw_dsc->color;
    img_dsc.pivot.x = glyph_draw_dsc->pivot.x;
    img_dsc.pivot.y = glyph_draw_dsc->g ? (glyph_draw_dsc->g->box_h + glyph_draw_dsc->g->ofs_y) : 0;
    img_dsc.base.layer = draw_unit->target_layer;

    lv_draw_task_t img_task;
    if(!create_task_from_draw_unit(draw_unit, &img_task, &img_dsc, glyph_draw_dsc->letter_coords)) {
        return;
    }

    lv_draw_sifli_epic_img(&img_task);
}

static void draw_fill_part(lv_draw_unit_t * draw_unit, lv_draw_fill_dsc_t * fill_draw_dsc, const lv_area_t * fill_area)
{
    lv_draw_fill_dsc_t local_dsc = *fill_draw_dsc;
    local_dsc.base.layer = draw_unit->target_layer;

    lv_draw_task_t fill_task;
    if(!create_task_from_draw_unit(draw_unit, &fill_task, &local_dsc, fill_area)) {
        return;
    }

    lv_draw_sifli_epic_fill(&fill_task);
}

static bool create_task_from_draw_unit(lv_draw_unit_t * draw_unit, lv_draw_task_t * task, void * draw_dsc,
                                       const lv_area_t * area)
{
    if(draw_unit == NULL || draw_unit->target_layer == NULL || draw_unit->clip_area == NULL ||
       task == NULL || draw_dsc == NULL || area == NULL) {
        return false;
    }

    *task = (lv_draw_task_t) {0};
    task->draw_dsc = draw_dsc;
    task->area = *area;
    task->_real_area = *area;
    task->clip_area = *draw_unit->clip_area;
    task->clip_area_original = task->clip_area;

    return true;
}

static lv_draw_buf_t * stage_glyph_bitmap(epic_label_ctx_t * ctx, const lv_draw_buf_t * glyph_buf)
{
    uint8_t stage_index = ctx->next_stage;
    lv_draw_buf_t * stage = ctx->glyph_stage[stage_index];

    if(stage != NULL &&
       lv_draw_buf_reshape(stage, glyph_buf->header.cf, glyph_buf->header.w,
                           glyph_buf->header.h, glyph_buf->header.stride) == NULL) {
        lv_draw_buf_destroy(stage);
        stage = NULL;
    }

    if(stage == NULL) {
        stage = lv_draw_buf_create(glyph_buf->header.w, glyph_buf->header.h,
                                   glyph_buf->header.cf, glyph_buf->header.stride);
        if(stage == NULL) {
            return NULL;
        }
        ctx->glyph_stage[stage_index] = stage;
    }

    lv_draw_buf_copy(stage, NULL, glyph_buf, NULL);
    ctx->next_stage ^= 1U;
    return stage;
}

#endif /*LV_USE_SIFLI_EPIC*/
