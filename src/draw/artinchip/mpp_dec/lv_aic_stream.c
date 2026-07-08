/*
 * Copyright (C) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>

#include "lv_aic_stream.h"

lv_fs_res_t lv_aic_stream_open(lv_stream_t *stream, const void *src)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    stream->src = ((lv_img_dsc_t *)src)->data;
    stream->cur_index = 0;
    if (lv_image_src_get_type(src) == LV_IMAGE_SRC_FILE) {
        stream->is_file = 1;
        res = lv_fs_open(&stream->f, src, LV_FS_MODE_RD);
    } else {
        stream->is_file = 0;
        stream->data_size = ((lv_img_dsc_t *)src)->data_size;
    }

    return res;
}

lv_fs_res_t lv_aic_stream_open_file(lv_stream_t *stream, const char *path)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    stream->src = NULL;
    stream->cur_index = 0;
    stream->is_file = 1;
    res = lv_fs_open(&stream->f, path, LV_FS_MODE_RD);

    return res;
}

lv_fs_res_t lv_aic_stream_open_buf(lv_stream_t *stream, const char *buf, uint32_t size)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    stream->src = (uint8_t *)buf;
    stream->cur_index = 0;
    stream->is_file = 0;
    stream->data_size = (int)size;

    return res;
}

lv_fs_res_t lv_aic_stream_close(lv_stream_t *stream)
{
   if (stream->is_file)
        lv_fs_close(&stream->f);

    return LV_FS_RES_OK;
}

lv_fs_res_t lv_aic_stream_seek(lv_stream_t *stream, uint32_t pos, lv_fs_whence_t whence)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    if (stream->is_file) {
        res = lv_fs_seek(&stream->f, pos, whence);
    } else {
        // only support SEEK_CUR
        stream->cur_index += pos;
    }

    return res;
}

lv_fs_res_t lv_aic_stream_read(lv_stream_t *stream, void *buf, uint32_t btr, uint32_t *br)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    if (stream->is_file) {
        res = lv_fs_read(&stream->f, buf, btr, br);
    } else {
        if (stream->cur_index + btr <= stream->data_size) {
            memcpy(buf, &stream->src[stream->cur_index], btr);
            stream->cur_index += btr;
            *br = btr;
        } else {
            LV_LOG_WARN("data_size:%d, cur_index:%d", stream->data_size, stream->cur_index);
        }
    }

    return res;
}

void lv_aic_stream_reset(lv_stream_t *stream)
{
    stream->cur_index = 0;
    return;
}

lv_result_t lv_aic_stream_get_size(lv_stream_t *stream, uint32_t *file_size)
{
    lv_result_t res = LV_RESULT_OK;

    if (stream->is_file) {
        lv_fs_seek(&stream->f, 0, SEEK_END);
        lv_fs_tell(&stream->f, file_size);
        lv_fs_seek(&stream->f, 0, SEEK_SET);
    } else {
        *file_size = stream->data_size;
    }

    return res;
}
