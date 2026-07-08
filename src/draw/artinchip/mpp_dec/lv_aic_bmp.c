/*
 * Copyright (C) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdlib.h>

#include "aic_core.h"
#include "lv_aic_stream.h"
#include "lv_mpp_dec.h"
#include "lv_aic_bmp.h"

#if LV_USE_AIC_BMP

lv_result_t lv_check_bmp_header(lv_stream_t *stream)
{
    uint32_t read_num;
    uint8_t buf[128];
    lv_fs_res_t fs_res;
    lv_result_t res = LV_RESULT_OK;

    fs_res = lv_aic_stream_read(stream, buf, 2, &read_num);
    if (fs_res != LV_FS_RES_OK || read_num != 2) {
        res = LV_RESULT_INVALID;
        goto read_err;
    }

    if (buf[0] != 0x42 || buf[1] != 0x4d) {
        res = LV_RESULT_INVALID;
        goto read_err;
    }

read_err:
    return res;
}

lv_result_t lv_bmp_decoder_info_from_stream(lv_stream_t *stream, lv_image_header_t *header)
{
    uint32_t read_num = 0;
    uint8_t buf[64] = { 0 };
    int32_t w = 0;
    int32_t h = 0;
    uint16_t bpp = 0;

    lv_fs_res_t res = lv_aic_stream_read(stream, buf, 54, &read_num);
    if(res != LV_FS_RES_OK || read_num != 54)
        return LV_RESULT_INVALID;

    lv_memcpy(&w, buf + 18, 4);
    lv_memcpy(&h, buf + 22, 4);
    lv_memcpy(&bpp, buf + 28, 2);

    header->w = w;
    header->h = LV_ABS(h);
    header->stride = ((bpp * w + 31) / 32) * 4;
    header->stride = ALIGN_UP((int32_t)(header->stride), 8);

    switch(bpp) {
        case 16:  header->cf = LV_COLOR_FORMAT_RGB565;   break;
        case 24:  header->cf = LV_COLOR_FORMAT_RGB888;   break;
        case 32:  header->cf = LV_COLOR_FORMAT_ARGB8888; break;
        default:  return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

lv_result_t lv_bmp_decoder_info(const char *src, lv_image_header_t *header, uint32_t size, bool is_file)
{
    lv_fs_res_t res;
    lv_stream_t stream;

    if(is_file)
        res = lv_aic_stream_open_file(&stream, src);
    else
        res = lv_aic_stream_open_buf(&stream, src, size);

    if(res != LV_FS_RES_OK) return LV_RESULT_INVALID;

    res = lv_bmp_decoder_info_from_stream(&stream, header);
    lv_aic_stream_close(&stream);
    return res;
}

lv_result_t lv_bmp_dec_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    lv_result_t res = LV_RESULT_OK;
    lv_stream_t stream;
    uint8_t headers[64];
    uint32_t read_num = 0;
    lv_fs_res_t fs_res;
    int32_t width;
    int32_t height;
    int32_t abs_height;
    uint16_t bpp;
    int32_t stride;
    int32_t stride_aligned;
    int32_t image_size;
    unsigned int align_addr;
    uint8_t *buf = NULL;
    int i;

    if (lv_aic_stream_open(&stream, dsc->src) != LV_FS_RES_OK)
        return LV_RESULT_INVALID;

    mpp_decoder_data_t *mpp_data = (mpp_decoder_data_t *)lv_malloc_zeroed(sizeof(mpp_decoder_data_t));
    CHECK_PTR(mpp_data);

    fs_res = lv_aic_stream_read(&stream, headers, 54, &read_num);
    if (fs_res != LV_FS_RES_OK || read_num != 54) {
        LV_LOG_ERROR("lv_aic_stream_read failed");
        res = LV_RESULT_INVALID;
        goto out;
    }

    lv_memcpy(&width, headers + 18, 4);
    lv_memcpy(&height, headers + 22, 4);
    lv_memcpy(&bpp, headers + 28, 2);

    stride = ((bpp * width + 31) / 32) * 4;
    stride_aligned = ALIGN_UP(stride, 8);
    abs_height = LV_ABS(height);
    image_size = ALIGN_UP(abs_height * stride_aligned, CACHE_LINE_SIZE);

    mpp_data->data[0] = (uint8_t *)mpp_dec_malloc_oom(image_size + CACHE_LINE_SIZE - 1);
    if (!mpp_data->data[0]) {
        res = LV_RESULT_INVALID;
        goto out;
    } else {
        align_addr = (unsigned int)ALIGN_UP((ulong)mpp_data->data[0], CACHE_LINE_SIZE);
    }

    mpp_data->decoded.header.w = width;
    mpp_data->decoded.header.h = abs_height;
    mpp_data->decoded.header.cf = dsc->header.cf;
    mpp_data->decoded.header.magic = LV_IMAGE_HEADER_MAGIC;
    mpp_data->decoded.header.stride = stride_aligned;
    mpp_data->decoded.data =(uint8_t *)((ulong)align_addr);
    mpp_data->decoded.data_size = image_size;
    mpp_data->decoded.unaligned_data = mpp_data->data[0];

    if (height < 0) {
        buf = mpp_data->decoded.data;
        if (stride == stride_aligned) {
            lv_aic_stream_read(&stream, buf, image_size, &read_num);
        } else {
            for (i = 0; i < abs_height; i++) {
                lv_aic_stream_read(&stream, buf, stride, &read_num);
                buf += stride_aligned;
            }
        }
    } else {
        for (i = height - 1; i >= 0; i--) {
            buf = mpp_data->decoded.data + i * stride_aligned;
            lv_aic_stream_read(&stream, buf, stride, &read_num);
        }
    }
    aicos_dcache_clean_invalid_range((ulong *)(ulong)align_addr, ALIGN_UP(image_size, CACHE_LINE_SIZE));

    dsc->decoded = &mpp_data->decoded;
    if (!dsc->args.no_cache && lv_image_cache_is_enabled()) {
        lv_image_cache_data_t search_key;
        search_key.src_type = dsc->src_type;
        search_key.src = dsc->src;
        search_key.slot.size = dsc->decoded->data_size;
        mpp_data->cached = true;
        lv_cache_entry_t *cache_entry = lv_image_decoder_add_to_cache(decoder, &search_key, dsc->decoded, mpp_data);
        CHECK_PTR(cache_entry);
        dsc->cache_entry = cache_entry;
    }

out:
    if (res == LV_RESULT_INVALID) {
        dsc->decoded = NULL;
        if (mpp_data) {
            lv_frame_buf_free(mpp_data);
            lv_free(mpp_data);
        }
    }

    lv_aic_stream_close(&stream);
    return res;
}

#endif
