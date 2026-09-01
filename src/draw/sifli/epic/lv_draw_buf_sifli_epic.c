/**
 * @file lv_draw_buf_sifli_epic.c
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
#include "../../../core/lv_global.h"
#include "../../../misc/lv_log.h"
#include "lv_sifli_epic_cfg.h"

/*********************
 *      DEFINES
 *********************/

#define font_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->font_draw_buf_handlers)
#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void buf_invalidate_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area);
static void buf_flush_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area);
static bool draw_buf_area_to_cache_region(const lv_draw_buf_t * draw_buf, const lv_area_t * area,
                                          uint8_t ** start, uint32_t * size);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_buf_sifli_epic_init_handlers(void)
{
    lv_draw_buf_handlers_t * handlers = lv_draw_buf_get_handlers();
    lv_draw_buf_handlers_t * font_handlers = font_draw_buf_handlers;
    lv_draw_buf_handlers_t * image_handlers = image_cache_draw_buf_handlers;

    handlers->invalidate_cache_cb = buf_invalidate_cache;
    handlers->flush_cache_cb = buf_flush_cache;
    font_handlers->invalidate_cache_cb = buf_invalidate_cache;
    font_handlers->flush_cache_cb = buf_flush_cache;
    image_handlers->invalidate_cache_cb = buf_invalidate_cache;
    image_handlers->flush_cache_cb = buf_flush_cache;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void buf_invalidate_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    uint8_t * start;
    uint32_t size;

    if(draw_buf == NULL || draw_buf->data == NULL) {
        return;
    }

    if(!draw_buf_area_to_cache_region(draw_buf, area, &start, &size)) {
        return;
    }

    if(!lv_epic_is_cached_ram((uint32_t)(uintptr_t)start, size)) {
        return;
    }

    lv_epic_invalidate_cache_range(start, size);
}

static void buf_flush_cache(const lv_draw_buf_t * draw_buf, const lv_area_t * area)
{
    uint8_t * start;
    uint32_t size;

    if(draw_buf == NULL || draw_buf->data == NULL) {
        return;
    }

    if(!draw_buf_area_to_cache_region(draw_buf, area, &start, &size)) {
        return;
    }

    if(!lv_epic_is_cached_ram((uint32_t)(uintptr_t)start, size)) {
        return;
    }

    lv_epic_flush_cache_range(start, size);
}

static bool draw_buf_area_to_cache_region(const lv_draw_buf_t * draw_buf, const lv_area_t * area,
                                          uint8_t ** start, uint32_t * size)
{
    LV_ASSERT_NULL(draw_buf);
    LV_ASSERT_NULL(start);
    LV_ASSERT_NULL(size);

    if(draw_buf->data == NULL) {
        return false;
    }

    uint32_t stride = draw_buf->header.stride;
    uint32_t height = area ? (uint32_t)lv_area_get_height(area) : draw_buf->header.h;
    int32_t y1 = area ? area->y1 : 0;

    if(height == 0U || y1 < 0) {
        return false;
    }

    *start = draw_buf->data + (uint32_t)y1 * stride;
    *size = stride * height;
    return true;
}

#endif /*LV_USE_SIFLI_EPIC*/
