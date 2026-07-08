/*
 * Copyright (C) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_AIC_STREAM_H
#define LV_AIC_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
    lv_fs_file_t f;
    int cur_index;
    const uint8_t *src;
    int is_file;
    int data_size;
} lv_stream_t;

static inline uint64_t stream_to_u64(uint8_t *ptr)
{
    return ((uint64_t)ptr[0] << 56) | ((uint64_t)ptr[1] << 48) |
           ((uint64_t)ptr[2] << 40) | ((uint64_t)ptr[3] << 32) |
           ((uint64_t)ptr[4] << 24) | ((uint64_t)ptr[5] << 16) |
           ((uint64_t)ptr[6] << 8) | ((uint64_t)ptr[7]);
}

static inline unsigned int stream_to_u32(uint8_t *ptr)
{
    return ((unsigned int)ptr[0] << 24) |
           ((unsigned int)ptr[1] << 16) |
           ((unsigned int)ptr[2] << 8) |
           (unsigned int)ptr[3];
}

static inline unsigned short stream_to_u16(uint8_t *ptr)
{
    return  ((unsigned int)ptr[0] << 8) | (unsigned int)ptr[1];
}

lv_fs_res_t lv_aic_stream_open(lv_stream_t *stream, const void *src);

lv_fs_res_t lv_aic_stream_open_file(lv_stream_t *stream, const char *path);

lv_fs_res_t lv_aic_stream_open_buf(lv_stream_t *stream, const char *buf, uint32_t size);

lv_fs_res_t lv_aic_stream_close(lv_stream_t *stream);

lv_fs_res_t lv_aic_stream_seek(lv_stream_t *stream, uint32_t pos, lv_fs_whence_t whence);

lv_fs_res_t lv_aic_stream_read(lv_stream_t *stream, void *buf, uint32_t btr, uint32_t *br);

void lv_aic_stream_reset(lv_stream_t *stream);

lv_result_t lv_aic_stream_get_size(lv_stream_t *stream, uint32_t *file_size);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //LV_AIC_STREAM_H
