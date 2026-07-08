/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lv_draw_ge2d_utils.h"
#include "lv_draw_ge2d.h"

enum mpp_pixel_format lv_fmt_to_mpp_fmt(lv_color_format_t cf)
{
    enum mpp_pixel_format fmt = LV_COLOR_FORMAT_RGB565;

    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
            fmt = MPP_FMT_RGB_565;
            break;
        case LV_COLOR_FORMAT_RGB888:
            fmt = MPP_FMT_RGB_888;
            break;
        case LV_COLOR_FORMAT_ARGB8888:
            fmt = MPP_FMT_ARGB_8888;
            break;
        case LV_COLOR_FORMAT_XRGB8888:
            fmt = MPP_FMT_XRGB_8888;
            break;
        case LV_COLOR_FORMAT_I420:
            fmt = MPP_FMT_YUV420P;
            break;
        case LV_COLOR_FORMAT_I422:
            fmt = MPP_FMT_YUV422P;
            break;
        case LV_COLOR_FORMAT_I444:
            fmt = MPP_FMT_YUV444P;
            break;
        case LV_COLOR_FORMAT_I400:
            fmt = MPP_FMT_YUV400;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }

    return fmt;
}
