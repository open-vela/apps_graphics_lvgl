/*
 * Copyright (C) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_AIC_IMG_DSC_H
#define LV_AIC_IMG_DSC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

lv_image_dsc_t *lv_aic_img_dsc_create(const char *file_path);

lv_image_dsc_t *lv_aic_img_dsc_create_from_buf(const char *buf, uint32_t size);

void lv_aic_img_dsc_destory(lv_image_dsc_t *img);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //LV_USE_AIC_IMG_DSC
