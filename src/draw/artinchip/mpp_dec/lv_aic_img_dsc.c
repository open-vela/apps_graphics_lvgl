/*
 * Copyright (C) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdlib.h>

#include "aic_core.h"
#include "mpp_ge.h"
#include "lv_aic_stream.h"
#include "lv_mpp_dec.h"
#include "lv_aic_img_dsc.h"
#include "../ge2d/lv_draw_ge2d_utils.h"
#include "../ge2d/lv_draw_ge2d.h"

#if LV_USE_DRAW_GE2D

void lv_aic_dec_free(struct mpp_buf *mpp_buf)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (mpp_buf->phy_addr[i]) {
            mpp_dec_free_oom( (void*)(ulong)mpp_buf->phy_addr[i]);
            mpp_buf->phy_addr[i] = 0;
        }
    }
}

lv_result_t lv_aic_dec_alloc(struct mpp_buf *mpp_buf, int *size)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (size[i]) {
            unsigned int align_addr = (unsigned int)(ulong)mpp_dec_memalign_oom( size[i], CACHE_LINE_SIZE);
            if (align_addr == 0) {
                LV_LOG_ERROR("lv_aic_dec_alloc failed");
                goto alloc_error;
            }

            mpp_buf->phy_addr[i] = align_addr;
            aicos_dcache_clean_invalid_range((ulong *)(ulong)align_addr, size[i]);
        }
    }

    return LV_RESULT_OK;
alloc_error:
    lv_aic_dec_free(mpp_buf);
    return LV_RESULT_INVALID;
}

static lv_result_t lv_mpp_dec_frame(struct mpp_buf *buf, const char *src, uint32_t size, bool is_file)
{
    lv_stream_t stream;
    char *ptr;
    struct mpp_packet packet;
    lv_image_header_t header;
    struct mpp_frame dec_frame = { 0 };
    struct mpp_frame frame = { 0 };
    lv_fs_res_t fs_res = LV_FS_RES_FS_ERR;
    lv_result_t res = LV_RESULT_OK;
    uint32_t file_len = 0;
    enum mpp_codec_type type = MPP_CODEC_VIDEO_DECODER_PNG;
    struct decode_config config = { 0 };
    int buf_size[3] = { 0 };
    struct mpp_decoder *dec = NULL;
    struct frame_allocator *allocator = NULL;
    bool has_frame = false;
    uint32_t read_size = 0;
    bool is_aicp = false;

    if (is_file) {
        // get image info
        ptr = strrchr(src, '.');
        CHECK_PTR(ptr);

        if (!strncmp(ptr, ".png", 4)) {
            res = lv_png_decoder_info(src, &header, 0, true);
            type = MPP_CODEC_VIDEO_DECODER_PNG;
        } else if (image_suffix_is_jpg(ptr)) {
            res = lv_jpeg_decoder_info(src, &header, 0, true, false);
            type = MPP_CODEC_VIDEO_DECODER_MJPEG;
#ifdef AIC_MPP_AICP_DEC_ENABLE
        } else if (image_suffix_is_aicp(ptr)) {
            res = lv_jpeg_decoder_info(src, &header, 0, true, true);
            type = MPP_CODEC_VIDEO_DECODER_AICP;
#endif
        } else {
            LV_LOG_ERROR("unsupported format:%s", src);
            goto out;
        }
    } else {
        bool is_aicp_data = false;
#ifdef AIC_MPP_AICP_DEC_ENABLE
        // Check for AICP header
        if (size >= 4 && memcmp(src, "AICP", 4) == 0) {
            is_aicp_data = true;
        }
#endif
        res = lv_jpeg_decoder_info(src, &header, size, false, is_aicp_data);
        if (is_aicp_data) {
            type = MPP_CODEC_VIDEO_DECODER_AICP;
        } else {
            type = MPP_CODEC_VIDEO_DECODER_MJPEG;
        }

        if (res != LV_RESULT_OK) {
            res = lv_png_decoder_info(src, &header, size, false);
            type = MPP_CODEC_VIDEO_DECODER_PNG;
        }
    }
    CHECK_RET(res, LV_RESULT_OK);

    // open image file
    if (is_file)
        fs_res = lv_aic_stream_open_file(&stream, src);
    else
        fs_res = lv_aic_stream_open_buf(&stream, src, size);

    CHECK_RET(fs_res, LV_FS_RES_OK);

    lv_aic_stream_get_size(&stream, &file_len);
    if (!file_len) {
        LV_LOG_ERROR("No image data");
        goto out;
    }

    dec = mpp_decoder_create(type);
    CHECK_PTR(dec);

    config.bitstream_buffer_size = ALIGN_UP(file_len, 256);
    config.extra_frame_num = 0;
    config.packet_count = 1;

    config.pix_fmt = lv_fmt_to_mpp_fmt(header.cf);
    dec_frame.buf.size.width = header.w;
    dec_frame.buf.size.height = header.h;
    dec_frame.buf.format = config.pix_fmt;
    dec_frame.buf.buf_type = MPP_PHY_ADDR;

    int size_shift = 0;
#if defined(MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE)
    if (type == MPP_CODEC_VIDEO_DECODER_MJPEG) {
        size_shift = header.reserved_2;
    }
#endif

    if (type == MPP_CODEC_VIDEO_DECODER_AICP)
        is_aicp = true;

    lv_set_frame_buf_size(&dec_frame, buf_size, 0, is_aicp);
    if (size_shift > 0) {
        struct mpp_scale_ratio scale;
        scale.hor_scale = size_shift;
        scale.ver_scale = size_shift;
        mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_SCALE, &scale);
    }

    CHECK_RET(lv_aic_dec_alloc(&dec_frame.buf, buf_size), LV_RESULT_OK);

    // allocator will be released in decoder
    allocator = lv_open_allocator(&dec_frame);
    CHECK_PTR(allocator);

    mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_EXT_FRAME_ALLOCATOR, (void*)allocator);
    CHECK_RET(mpp_decoder_init(dec, &config), 0);
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, file_len);

    lv_aic_stream_read(&stream, packet.data, file_len, &read_size);
    packet.size = file_len;
    packet.flag = PACKET_FLAG_EOS;

    mpp_decoder_put_packet(dec, &packet);
    CHECK_RET(mpp_decoder_decode(dec), 0);

    mpp_decoder_get_frame(dec, &frame);
    mpp_decoder_put_frame(dec, &frame);

    memcpy(buf, &frame.buf, sizeof(struct mpp_buf));
    has_frame = true;

out:
    if (fs_res == LV_FS_RES_OK)
        lv_aic_stream_close(&stream);

    if (dec)
        mpp_decoder_destory(dec);

    if (!has_frame) {
        lv_aic_dec_free(&dec_frame.buf);
    }

    if (has_frame)
        return LV_RESULT_OK;
    else
        return LV_RESULT_INVALID;
}

static inline bool mpp_fmt_is_yuv(enum mpp_pixel_format fmt)
{
    if (fmt >= MPP_FMT_YUV420P)
        return true;
    else
        return false;
}

static int mpp_buf_yuv_to_rgb(struct mpp_buf *buf)
{
    int ret;
    enum mpp_pixel_format out_fmt = MPP_FMT_ARGB_8888;
    struct ge_bitblt blt = { 0 };
    struct mpp_ge *ge2d_dev = get_ge2d_device();
    int out_stride = 0;
    int out_width;
    int out_height;
    unsigned int out_addr;
    int out_size;

    if (buf->crop_en) {
        out_width = buf->crop.width;
        out_height = buf->crop.height;
    } else {
        out_width = buf->size.width;
        out_height = buf->size.height;
    }

    if (out_width < 4 || out_height < 4) {
        LV_LOG_ERROR("Unsupported width: %d height:%d", out_width, out_height);
        return -1;
    }

    /* src buf */
    memcpy(&blt.src_buf, buf, sizeof(struct mpp_buf));

#if LV_COLOR_DEPTH == 16
    out_fmt = MPP_FMT_RGB_565;
    out_stride = ALIGN_UP(out_width * 2, 8);
#elif LV_COLOR_DEPTH == 24
    out_fmt = MPP_FMT_RGB_888;
    out_stride = ALIGN_UP(out_width * 3, 8);
#else
    out_fmt = MPP_FMT_ARGB_8888;
    out_stride = ALIGN_UP(out_width * 4, 8);
#endif

    out_size = out_stride * out_height;
    out_addr = (unsigned int)(ulong)mpp_dec_memalign_oom( out_size, CACHE_LINE_SIZE);
    if (!out_addr) {
        LV_LOG_ERROR("alloc out buf failed");
        return -1;
    }

    aicos_dcache_clean_invalid_range((ulong *)(ulong)out_addr, out_size);

    /* dst buf */
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = out_addr;
    blt.dst_buf.stride[0] = out_stride;
    blt.dst_buf.format = out_fmt;
    blt.dst_buf.size.width = out_width;
    blt.dst_buf.size.height = out_height;

    ret = mpp_ge_bitblt(ge2d_dev, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail\n");
        goto failed;
    }
    ret = mpp_ge_emit(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("emit fail\n");
        goto failed;
    }
    ret = mpp_ge_sync(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("sync fail\n");
        goto failed;
    }

    /* free source buffer */
    lv_aic_dec_free(&blt.src_buf);

    /* get new buffer */
    memcpy(buf, &blt.dst_buf, sizeof(struct mpp_buf));

    return 0;

failed:
    mpp_dec_free_oom( (void *)(ulong)out_addr);
    return -1;
}

static lv_image_dsc_t *lv_aic_img_dsc_create_base(const char *src,  uint32_t size, bool is_file)
{
    lv_image_dsc_t *img_dsc;
    struct mpp_buf buf = { 0 };
    lv_result_t res = LV_RESULT_OK;

    if (is_file)
        res = lv_mpp_dec_frame(&buf, src, 0, true);
    else
        res = lv_mpp_dec_frame(&buf, src, size, false);

    if (res != LV_RESULT_OK) {
        LV_LOG_ERROR("lv decoder failed");
        return NULL;
    }

    img_dsc = lv_malloc(sizeof(lv_image_dsc_t));
    if(!img_dsc) {
        LV_LOG_ERROR("malloc lv_image_dsc_t failed");
        goto out;
    }

    memset(img_dsc, 0, sizeof(lv_image_dsc_t));
    if (mpp_fmt_is_yuv(buf.format)) {
        if (mpp_buf_yuv_to_rgb(&buf) != 0) {
            LV_LOG_ERROR("mpp_buf_yuv_to_rgb failed");
            goto out;
        }
    }

    if (buf.crop_en) {
        img_dsc->header.w = buf.crop.width;
        img_dsc->header.h = buf.crop.height;
    } else {
        img_dsc->header.w = buf.size.width;
        img_dsc->header.h = buf.size.height;
    }

    img_dsc->header.cf = mpp_fmt_to_lv_fmt(buf.format);
    img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc->header.stride = buf.stride[0];
    img_dsc->data_size = img_dsc->header.stride * img_dsc->header.h;
    img_dsc->data = (uint8_t *)(ulong)buf.phy_addr[0];

    return img_dsc;
out:
    if (img_dsc)
        lv_free(img_dsc);

    lv_aic_dec_free(&buf);

    return NULL;
}

lv_image_dsc_t *lv_aic_img_dsc_create(const char *file_path)
{
    return lv_aic_img_dsc_create_base(file_path, 0, true);
}

lv_image_dsc_t *lv_aic_img_dsc_create_from_buf(const char *buf, uint32_t size)
{
    return lv_aic_img_dsc_create_base(buf, size, false);
}

void lv_aic_img_dsc_destory(lv_image_dsc_t *img_dsc)
{
    if (img_dsc) {
        if (img_dsc->data) {
            uint8_t *data = (uint8_t *)img_dsc->data;
            mpp_dec_free_oom( data);
        }
        lv_free(img_dsc);
    }
}

#endif
