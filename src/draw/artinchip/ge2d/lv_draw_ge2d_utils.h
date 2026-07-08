/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_DRAW_GE2D_UTILS_H
#define LV_DRAW_GE2D_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "mpp_types.h"

static inline bool ge2d_dst_fmt_supported(lv_color_format_t cf)
{
    bool supported = false;

    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_RGB888:
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
            supported = true;
            break;
        default:
            break;
    }

    return supported;
}

static inline bool ge2d_src_fmt_supported(lv_color_format_t cf)
{
    bool supported = false;

    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_RGB888:
        case LV_COLOR_FORMAT_ARGB8888:
        case LV_COLOR_FORMAT_XRGB8888:
        case LV_COLOR_FORMAT_I420:
        case LV_COLOR_FORMAT_I422:
        case LV_COLOR_FORMAT_I444:
        case LV_COLOR_FORMAT_I400:
        case LV_COLOR_FORMAT_RAW:
            supported = true;
            break;
        default:
            break;
    }

    return supported;
}

static inline bool lv_fmt_is_yuv(lv_color_format_t fmt)
{
    if (fmt >= LV_COLOR_FORMAT_YUV_START)
        return true;
    else
        return false;
}

enum mpp_pixel_format lv_fmt_to_mpp_fmt(lv_color_format_t cf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DRAW_GE2D_UTILS_H*/
