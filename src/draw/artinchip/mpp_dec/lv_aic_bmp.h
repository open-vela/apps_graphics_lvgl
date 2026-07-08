/*
 * Copyright (C) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_AIC_BMP_H
#define LV_AIC_BMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "lv_aic_stream.h"

#if LV_USE_AIC_BMP

lv_result_t lv_check_bmp_header(lv_stream_t *stream);

lv_result_t lv_bmp_decoder_info(const char *src, lv_image_header_t *header, uint32_t size, bool is_file);

lv_result_t lv_bmp_decoder_info_from_stream(lv_stream_t *stream, lv_image_header_t *header);

lv_result_t lv_bmp_dec_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //LV_AIC_BMP_H
