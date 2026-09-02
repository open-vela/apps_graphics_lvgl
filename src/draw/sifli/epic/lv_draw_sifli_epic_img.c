/**
 * @file lv_draw_sifli_epic_img.c
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
#include <string.h>

#include "lv_sifli_epic_utils.h"
#include "../../../draw/lv_draw_image.h"
#include "../../../draw/lv_image_decoder.h"
#include "../../../misc/lv_area.h"


/*********************
 *      DEFINES
 *********************/

#define IMAGE_STAGE_CAPACITY            (100U * 100U * 3U)
#define IMAGE_STAGE_REPLACE_INTERVAL_MS 500U

static void epic_img_draw_core(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * draw_dsc,
                               const lv_image_decoder_dsc_t * decoder_dsc, lv_draw_image_sup_t * sup,
                               const lv_area_t * img_coords, const lv_area_t * clipped_img_area);

#ifdef CONFIG_LV_USE_SIFLI_EPIC_IMAGE_SRAM_STAGE

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static const lv_image_dsc_t * stage_immutable_image(const lv_image_dsc_t * src_dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

static struct {
    const uint8_t * src;
    uint32_t src_size;
    uint32_t src_stride;
    lv_color_format_t src_cf;
    uint32_t loaded_at;
    lv_image_dsc_t dsc;
} image_stage;

static LV_ATTRIBUTE_MEM_ALIGN uint8_t image_stage_data[IMAGE_STAGE_CAPACITY];

#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_sifli_epic_img(lv_draw_task_t * task)
{
    const lv_draw_image_dsc_t * dsc = (const lv_draw_image_dsc_t *)task->draw_dsc;
    const lv_area_t * coords = &task->area;
    const lv_draw_dsc_base_t * draw_dsc_base = (const lv_draw_dsc_base_t *)task->draw_dsc;
    lv_layer_t * layer = draw_dsc_base->layer;

    if(dsc->opa <= LV_OPA_MIN) {
        return;
    }

    if(layer == NULL) {
        return;
    }

    lv_draw_unit_t draw_unit = {0};
    draw_unit.target_layer = layer;
    draw_unit.clip_area = &task->clip_area;

    lv_draw_image_dsc_t local_dsc = *dsc;
    local_dsc.base.layer = layer;

    bool has_transform = dsc->rotation != 0 || dsc->scale_x != LV_SCALE_NONE || dsc->scale_y != LV_SCALE_NONE;
    const lv_image_dsc_t * staged = NULL;

    if(dsc->header.cf == LV_COLOR_FORMAT_RGB565 || dsc->header.cf == LV_COLOR_FORMAT_RGB565A8) {
#ifdef CONFIG_LV_USE_SIFLI_EPIC_IMAGE_SRAM_STAGE
        if(lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_VARIABLE) {
            staged = stage_immutable_image((const lv_image_dsc_t *)dsc->src);
        }
#endif

        if(staged != NULL) {
            local_dsc.src = staged;
            local_dsc.header = staged->header;
        }
        else if(dsc->header.cf == LV_COLOR_FORMAT_RGB565A8 && has_transform) {
            lv_draw_sw_image(&draw_unit, &local_dsc, coords);
            return;
        }
    }

    if(dsc->tile) {
        _lv_draw_image_tiled_helper(&draw_unit, &local_dsc, coords, epic_img_draw_core);
    }
    else {
        _lv_draw_image_normal_helper(&draw_unit, &local_dsc, coords, epic_img_draw_core);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef CONFIG_LV_USE_SIFLI_EPIC_IMAGE_SRAM_STAGE
static const lv_image_dsc_t * stage_immutable_image(const lv_image_dsc_t * src_dsc)
{
    if(src_dsc == NULL || src_dsc->data == NULL ||
       (src_dsc->header.cf != LV_COLOR_FORMAT_RGB565 &&
        src_dsc->header.cf != LV_COLOR_FORMAT_RGB565A8) ||
       (src_dsc->header.flags & LV_IMAGE_FLAGS_MODIFIABLE) != 0U) {
        return NULL;
    }

    uint32_t width = src_dsc->header.w;
    uint32_t height = src_dsc->header.h;
    uint32_t src_stride = src_dsc->header.stride;
    uint32_t staged_stride = width * 2U;
    uint32_t staged_size = staged_stride * height;
    uint32_t source_size = src_stride * height;
    lv_color_format_t staged_cf = LV_COLOR_FORMAT_RGB565;

    if(src_dsc->header.cf == LV_COLOR_FORMAT_RGB565A8) {
        if((src_stride & 1U) != 0U) {
            return NULL;
        }

        uint32_t alpha_stride = src_stride / 2U;
        source_size += alpha_stride * height;
        staged_stride = width * 3U;
        staged_size = staged_stride * height;
        staged_cf = LV_COLOR_FORMAT_ARGB8565;
    }

    if(width == 0U || height == 0U || src_stride < width * 2U ||
       src_dsc->data_size < source_size) {
        return NULL;
    }

    if(image_stage.src == src_dsc->data && image_stage.src_size == src_dsc->data_size &&
       image_stage.src_stride == src_stride && image_stage.src_cf == src_dsc->header.cf &&
       image_stage.dsc.header.w == width && image_stage.dsc.header.h == height) {
        return &image_stage.dsc;
    }

    if(staged_size > IMAGE_STAGE_CAPACITY) {
        return NULL;
    }

    if(image_stage.src != NULL && lv_tick_elaps(image_stage.loaded_at) < IMAGE_STAGE_REPLACE_INTERVAL_MS) {
        return NULL;
    }

    if(src_dsc->header.cf == LV_COLOR_FORMAT_RGB565) {
        for(uint32_t y = 0; y < height; y++) {
            memcpy(image_stage_data + y * staged_stride,
                   src_dsc->data + y * src_stride, staged_stride);
        }
    }
    else {
        uint32_t alpha_stride = src_stride / 2U;
        const uint8_t * alpha_plane = src_dsc->data + src_stride * height;
        for(uint32_t y = 0; y < height; y++) {
            const uint8_t * rgb = src_dsc->data + y * src_stride;
            const uint8_t * alpha = alpha_plane + y * alpha_stride;
            uint8_t * dst = image_stage_data + y * staged_stride;

            for(uint32_t x = 0; x < width; x++) {
                dst[x * 3U] = rgb[x * 2U];
                dst[x * 3U + 1U] = rgb[x * 2U + 1U];
                dst[x * 3U + 2U] = alpha[x];
            }
        }
    }

    image_stage.src = src_dsc->data;
    image_stage.src_size = src_dsc->data_size;
    image_stage.src_stride = src_stride;
    image_stage.src_cf = src_dsc->header.cf;
    image_stage.loaded_at = lv_tick_get();
    image_stage.dsc = *src_dsc;
    image_stage.dsc.header.cf = staged_cf;
    image_stage.dsc.header.stride = staged_stride;
    image_stage.dsc.header.flags &= ~(LV_IMAGE_FLAGS_ALLOCATED | LV_IMAGE_FLAGS_MODIFIABLE);
    image_stage.dsc.data = image_stage_data;
    image_stage.dsc.data_size = staged_size;

    return &image_stage.dsc;
}
#endif

static void epic_img_draw_core(lv_draw_unit_t * draw_unit, const lv_draw_image_dsc_t * draw_dsc,
                               const lv_image_decoder_dsc_t * decoder_dsc, lv_draw_image_sup_t * sup,
                               const lv_area_t * img_coords, const lv_area_t * clipped_img_area)
{
    const lv_draw_buf_t * decoded = decoder_dsc->decoded;
    const uint8_t * src_buf;
    lv_color_format_t cf;
    uint32_t img_total_width;
    const bool is_ezip = (decoded != NULL) && ((decoded->header.flags & LV_IMAGE_FLAGS_EZIP) != 0U);
#if defined(EPIC_INPUT_JPEG)
    const bool is_jpeg = (decoded != NULL) && ((decoded->header.flags & LV_IMAGE_FLAGS_JPEG) != 0U);
#else
    const bool is_jpeg = false;
#endif

    if(decoded == NULL || decoded->data == NULL) {
        return;
    }

    if(draw_dsc->clip_radius != 0 || draw_dsc->blend_mode != LV_BLEND_MODE_NORMAL) {
        return;
    }

    lv_draw_buf_flush_cache(decoded, NULL);

    src_buf = decoded->data;
    cf = decoded->header.cf;

    if(is_ezip || is_jpeg) {
        img_total_width = decoded->header.w;
    }
    else {
        img_total_width = lv_epic_stride_to_width(decoded->header.stride, cf);
    }

    EPIC_LayerConfigTypeDef input_layers[4];
    EPIC_LayerConfigTypeDef output_layer;
    uint8_t input_layer_cnt = 2;

    lv_draw_task_t task;
    task.draw_dsc = (void *)draw_dsc;
    task.area = *img_coords;
    task._real_area = *img_coords;
    task.clip_area = *draw_unit->clip_area;
    task.clip_area_original = task.clip_area;

    if(lv_epic_setup_layers(&input_layers[0], &output_layer, &task, clipped_img_area)) {
        return;
    }

    HAL_EPIC_LayerConfigInit(&input_layers[1]);
    input_layers[1].transform_cfg.angle = (draw_dsc->rotation + 3600) % 3600;
    input_layers[1].transform_cfg.pivot_x = draw_dsc->pivot.x;
    input_layers[1].transform_cfg.pivot_y = draw_dsc->pivot.y;
    input_layers[1].transform_cfg.scale_x = LV_SCALE_NONE * EPIC_INPUT_SCALE_NONE / (uint32_t)draw_dsc->scale_x;
    input_layers[1].transform_cfg.scale_y = LV_SCALE_NONE * EPIC_INPUT_SCALE_NONE / (uint32_t)draw_dsc->scale_y;
    input_layers[1].alpha = draw_dsc->opa;
    input_layers[1].x_offset = img_coords->x1;
    input_layers[1].y_offset = img_coords->y1;

    if(is_ezip) {
        input_layers[1].color_mode = EPIC_INPUT_EZIP;
    }
#if defined(EPIC_INPUT_JPEG)
    else if(is_jpeg) {
        input_layers[1].color_mode = EPIC_INPUT_JPEG;
    }
#endif
    else {
        input_layers[1].color_mode = lv_img_cf_to_epic_cf(cf);
    }

    input_layers[1].data = (uint8_t *)src_buf;
    input_layers[1].data_size = decoded->data_size;
    input_layers[1].width = lv_area_get_width(img_coords);
    input_layers[1].height = lv_area_get_height(img_coords);
    input_layers[1].total_width = img_total_width;

#ifdef EPIC_SUPPORT_A8
    if(input_layers[1].color_mode == EPIC_INPUT_A8) {
        input_layers[1].color_en = true;
        input_layers[1].color_r = sup->alpha_color.red;
        input_layers[1].color_g = sup->alpha_color.green;
        input_layers[1].color_b = sup->alpha_color.blue;
        input_layers[1].ax_mode = ALPHA_BLEND_RGBCOLOR;
    }
    else
#endif
#ifdef EPIC_SUPPORT_L8
        if(input_layers[1].color_mode == EPIC_INPUT_L8) {
            input_layers[1].lookup_table = (uint8_t *)decoder_dsc->palette;
            input_layers[1].color_en = false;
        }
        else
#endif
        {
            input_layers[1].color_en = false;
        }

#ifdef EPIC_SUPPORT_MASK
    if(cf == LV_COLOR_FORMAT_RGB565A8) {
        HAL_EPIC_LayerConfigInit(&input_layers[input_layer_cnt]);
        input_layers[input_layer_cnt].alpha = draw_dsc->opa;
        input_layers[input_layer_cnt].x_offset = img_coords->x1;
        input_layers[input_layer_cnt].y_offset = img_coords->y1;
        input_layers[input_layer_cnt].color_mode = EPIC_INPUT_A8;
        input_layers[input_layer_cnt].data = ((uint8_t *)src_buf) + decoded->header.h * decoded->header.stride;
        input_layers[input_layer_cnt].ax_mode = ALPHA_BLEND_OVERWRITE;
        input_layers[input_layer_cnt].width = lv_area_get_width(img_coords);
        input_layers[input_layer_cnt].height = lv_area_get_height(img_coords);
        input_layers[input_layer_cnt].total_width = decoded->header.w;
        input_layer_cnt++;
    }
#endif

    lv_image_decoder_dsc_t mask_decoder_dsc;
    bool mask_decoder_opened = false;

    if(draw_dsc->bitmap_mask_src != NULL) {
        lv_area_t mask_area;
        const lv_image_dsc_t * mask_dsc = draw_dsc->bitmap_mask_src;
        const lv_area_t * image_area = lv_area_get_width(&draw_dsc->original_area) < 0 ?
                                       img_coords : &draw_dsc->original_area;

        if(lv_image_src_get_type(mask_dsc) == LV_IMAGE_SRC_VARIABLE &&
           (mask_dsc->header.cf == LV_COLOR_FORMAT_A8 || mask_dsc->header.cf == LV_COLOR_FORMAT_L8)) {
            const lv_image_header_t * header = &mask_dsc->header;

            lv_area_set(&mask_area, 0, 0, header->w - 1, header->h - 1);
            lv_area_align(image_area, &mask_area, LV_ALIGN_CENTER, 0, 0);

            HAL_EPIC_LayerConfigInit(&input_layers[input_layer_cnt]);
            input_layers[input_layer_cnt].alpha = LV_OPA_COVER;
            input_layers[input_layer_cnt].x_offset = mask_area.x1;
            input_layers[input_layer_cnt].y_offset = mask_area.y1;
            input_layers[input_layer_cnt].color_mode = EPIC_INPUT_A8;
            input_layers[input_layer_cnt].data = (uint8_t *)mask_dsc->data;
            input_layers[input_layer_cnt].ax_mode = ALPHA_BLEND_OVERWRITE;
            input_layers[input_layer_cnt].width = lv_area_get_width(&mask_area);
            input_layers[input_layer_cnt].height = lv_area_get_height(&mask_area);
            input_layers[input_layer_cnt].total_width = header->stride;
            input_layer_cnt++;
        }
        else if(lv_image_decoder_open(&mask_decoder_dsc, draw_dsc->bitmap_mask_src, NULL) == LV_RESULT_OK &&
                mask_decoder_dsc.decoded != NULL) {
            const lv_draw_buf_t * mask_img = mask_decoder_dsc.decoded;
            mask_decoder_opened = true;

            if(mask_img->header.cf == LV_COLOR_FORMAT_A8 || mask_img->header.cf == LV_COLOR_FORMAT_L8) {
                lv_area_set(&mask_area, 0, 0, mask_img->header.w - 1, mask_img->header.h - 1);
                lv_area_align(image_area, &mask_area, LV_ALIGN_CENTER, 0, 0);

                HAL_EPIC_LayerConfigInit(&input_layers[input_layer_cnt]);
                input_layers[input_layer_cnt].alpha = LV_OPA_COVER;
                input_layers[input_layer_cnt].x_offset = mask_area.x1;
                input_layers[input_layer_cnt].y_offset = mask_area.y1;
                input_layers[input_layer_cnt].color_mode = EPIC_INPUT_A8;
                input_layers[input_layer_cnt].data = (uint8_t *)mask_img->data;
                input_layers[input_layer_cnt].ax_mode = ALPHA_BLEND_OVERWRITE;
                input_layers[input_layer_cnt].width = lv_area_get_width(&mask_area);
                input_layers[input_layer_cnt].height = lv_area_get_height(&mask_area);
                input_layers[input_layer_cnt].total_width = mask_img->header.stride;
                input_layer_cnt++;
            }
            else {
            }
        }
        else {
        }
    }

    (void)lv_epic_blend(input_layers, input_layer_cnt, &output_layer);

    if(mask_decoder_opened) {
        lv_image_decoder_close(&mask_decoder_dsc);
    }
}

#endif /*LV_USE_SIFLI_EPIC*/
