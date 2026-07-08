/*
 * Copyright (C) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "lvgl.h"
#include "aic_core.h"
#include "mpp_mem.h"
#include "mpp_ge.h"
#include "lv_mpp_dec.h"
#include "lv_aic_stream.h"
#include "lv_aic_bmp.h"
#include "../ge2d/lv_draw_ge2d_utils.h"

#define img_cache_p (LV_GLOBAL_DEFAULT()->img_cache)

#define PNG_HEADER_SIZE (8 + 12 + 13) //png signature + IHDR chuck
#define PNGSIG 0x89504e470d0a1a0aull
#define MNGSIG 0x8a4d4e470d0a1a0aull
#define JPEG_SOI 0xFFD8
#define JPEG_SOF 0xFFC0
#define AICP_SOF 0xFFC1
#define ALIGN_16B(x) (((x) + (15)) & ~(15))

/* [orig][flag][returned ptr...]
 * flag: NULL = system heap (kmm_free), non-NULL = LVGL heap (buf_free_cb)
 */
#define AIOCS_OOM_SLOTS 2

static inline bool aicos_oom_is_lvgl(void) {
    return lv_is_initialized() &&
           LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers.buf_malloc_cb != NULL &&
           img_cache_p != NULL;
}

void *mpp_dec_malloc_oom(size_t size)
{
    size_t alloc_size = size + AIOCS_OOM_SLOTS * sizeof(void *);
    void *orig;
    void *lvgl_flag = NULL;

    if(aicos_oom_is_lvgl()) {
        lv_draw_buf_handlers_t *h = &LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers;
        while(1) {
            orig = h->buf_malloc_cb(alloc_size, LV_COLOR_FORMAT_UNKNOWN);
            if(orig) { lvgl_flag = (void *)1; break; }
            if(!lv_cache_evict_one(img_cache_p, NULL)) return NULL;
        }
    } else {
        orig = kmm_malloc(alloc_size);
        if(!orig) return NULL;
    }

    void *ptr = (void *)((uint8_t *)orig + AIOCS_OOM_SLOTS * sizeof(void *));
    ((void **)ptr)[-1] = orig;
    ((void **)ptr)[-2] = lvgl_flag;
    return ptr;
}

void *mpp_dec_memalign_oom(size_t size, size_t align)
{
    size_t alloc_size = size + align - 1 + AIOCS_OOM_SLOTS * sizeof(void *);
    void *orig;
    void *lvgl_flag = NULL;

    if(aicos_oom_is_lvgl()) {
        lv_draw_buf_handlers_t *h = &LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers;
        while(1) {
            orig = h->buf_malloc_cb(alloc_size, LV_COLOR_FORMAT_UNKNOWN);
            if(orig) { lvgl_flag = (void *)1; break; }
            if(!lv_cache_evict_one(img_cache_p, NULL)) return NULL;
        }
    } else {
        orig = kmm_malloc(alloc_size);
        if(!orig) return NULL;
    }

    void *aligned = (void *)ALIGN_UP((ulong)orig + AIOCS_OOM_SLOTS * sizeof(void *), align);
    ((void **)aligned)[-1] = orig;
    ((void **)aligned)[-2] = lvgl_flag;
    return aligned;
}

void mpp_dec_free_oom(void *ptr)
{
    if(!ptr) return;

    void *orig = ((void **)ptr)[-1];

    if(((void **)ptr)[-2]) {
        lv_draw_buf_handlers_t *h = &LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers;
        h->buf_free_cb(orig);
    } else {
        kmm_free(orig);
    }
}

lv_color_format_t mpp_fmt_to_lv_fmt(enum mpp_pixel_format cf)
{
    lv_color_format_t fmt = LV_COLOR_FORMAT_ARGB8888;

    switch(cf) {
        case MPP_FMT_RGB_565:
            fmt = LV_COLOR_FORMAT_RGB565;
            break;
        case MPP_FMT_RGB_888:
            fmt = LV_COLOR_FORMAT_RGB888;
            break;
        case MPP_FMT_ARGB_8888:
            fmt = LV_COLOR_FORMAT_ARGB8888;
            break;
        case MPP_FMT_XRGB_8888:
            fmt = LV_COLOR_FORMAT_XRGB8888;
            break;
        case MPP_FMT_YUV420P:
            fmt = LV_COLOR_FORMAT_I420;
            break;
        case MPP_FMT_YUV422P:
            fmt = LV_COLOR_FORMAT_I422;
            break;
        case MPP_FMT_YUV444P:
            fmt = LV_COLOR_FORMAT_I444;
            break;
        case MPP_FMT_YUV400:
            fmt = LV_COLOR_FORMAT_I400;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }

    return fmt;
}

static int mpp_get_rgb_stride(int width, enum mpp_pixel_format fmt)
{
    int stride;

    switch(fmt) {
        case MPP_FMT_RGB_565:
            stride = ALIGN_16B(width) * 2;
            break;
        case MPP_FMT_RGB_888:
            stride = ALIGN_16B(width) * 3;
            break;
        case MPP_FMT_ARGB_8888:
            stride = ALIGN_16B(width) * 4;
            break;
        case MPP_FMT_XRGB_8888:
            stride = ALIGN_16B(width) * 4;
            break;
        default:
            stride = ALIGN_16B(width) * 4;
            LV_LOG_ERROR("unsupported format:%d", fmt);
            break;
    }
    return stride;
}

static int get_jpeg_format(uint8_t *buf, enum mpp_pixel_format *pix_fmt)
{
    int i;
    uint32_t h_count_flag;
    uint32_t v_count_flag;
    uint8_t h_count[3] = { 0 };
    uint8_t v_count[3] = { 0 };
    uint8_t nb_components = *buf++;

    for (i = 0; i < nb_components; i++) {
        uint8_t h_v_cnt;

        // skip component id
        buf++;
        h_v_cnt = *buf++;
        h_count[i] = h_v_cnt >> 4;
        v_count[i] = h_v_cnt & 0xf;

        // skip quant_index
        buf++;
    }

    h_count_flag =  h_count[2] | (h_count[1] << 4) | (h_count[0] << 8);
    v_count_flag =  v_count[2] | (v_count[1] << 4) | (v_count[0] << 8);

    if (h_count_flag == 0x211 && v_count_flag == 0x211) {
        *pix_fmt = MPP_FMT_YUV420P;
    } else if (h_count_flag == 0x211 && v_count_flag == 0x111) {
        *pix_fmt = MPP_FMT_YUV422P;
    } else if (h_count_flag == 0x111 && v_count_flag == 0x111) {
        *pix_fmt = MPP_FMT_YUV444P;
    } else if (h_count_flag == 0x111 && v_count_flag == 0x222) {
        *pix_fmt = MPP_FMT_YUV444P;
    } else if (h_count[1] == 0 && v_count[1] == 0 && h_count[2] == 0 &&
               v_count[2] == 0) {
        *pix_fmt = MPP_FMT_YUV400;
    } else {
        LV_LOG_ERROR("Not support format! h_count: %d %d %d, v_count: %d %d %d",
            h_count[0], h_count[1], h_count[2],
            v_count[0], v_count[1], v_count[2]);
        return -1;
    }

#ifndef AIC_VE_DRV_V10
#if LV_COLOR_DEPTH  == 16
    *pix_fmt = MPP_FMT_RGB_565;
#else
    *pix_fmt = MPP_FMT_RGB_888;
#endif
#endif

#ifdef AIC_VE_DRV_V31
    if (nb_components == 4)
        *pix_fmt = MPP_FMT_ARGB_8888;
#else
    if (nb_components == 4) {
        LV_LOG_ERROR("Unsupported nb_components: %d", nb_components);
        return -1;
    }
#endif
    return 0;
}

static inline lv_result_t check_jpeg_soi(lv_stream_t *stream)
{
    uint32_t read_num;
    uint8_t buf[128];
    lv_fs_res_t fs_res;
    lv_result_t res = LV_RESULT_OK;

    // read JPEG SOI
    fs_res = lv_aic_stream_read(stream, buf, 2, &read_num);
    if (fs_res != LV_FS_RES_OK || read_num != 2) {
        res = LV_RESULT_INVALID;
        goto read_err;
    }

    // check SOI
    if (buf[0] != 0xff || buf[1] != 0xd8) {
        res = LV_RESULT_INVALID;
        goto read_err;
    }

    // check SOI
    if (stream_to_u16(buf) != JPEG_SOI) {
        res = LV_RESULT_INVALID;
        goto read_err;
    }

read_err:
    return res;
}

static lv_result_t jpeg_get_img_size(lv_stream_t *stream, int *w, int *h, enum mpp_pixel_format *pix_fmt, bool is_aicp)
{
    uint32_t read_num;
    uint8_t buf[128];
    lv_fs_res_t fs_res;
    lv_result_t res = LV_RESULT_OK;

    if (check_jpeg_soi(stream) != LV_RESULT_OK) {
        res = LV_RESULT_INVALID;
        LV_LOG_ERROR("check jpeg soi failed");
        goto read_err;
    }

    // find SOF
    while (1) {
        int size;
        uint16_t marker;
        fs_res = lv_aic_stream_read(stream, buf, 4, &read_num);
        if (fs_res != LV_FS_RES_OK || read_num != 4) {
            res = LV_RESULT_INVALID;
            LV_LOG_ERROR("get chunk header failed");
            goto read_err;
        }

        marker = stream_to_u16(buf);
        if (marker == JPEG_SOF || (is_aicp && marker == AICP_SOF)) {
            fs_res = lv_aic_stream_read(stream, buf, 15, &read_num);
            if (fs_res != LV_FS_RES_OK) {
                res = LV_RESULT_INVALID;
                LV_LOG_ERROR("read chunk data failed");
                goto read_err;
            }

            *h = stream_to_u16(buf + 1);
            *w = stream_to_u16(buf + 3);

            if (get_jpeg_format(buf + 5, pix_fmt) < 0) {
                res = LV_RESULT_INVALID;
                LV_LOG_ERROR("get format failed");
                goto read_err;
            }
            break;
        } else {
            size = stream_to_u16(buf + 2);
             fs_res = lv_aic_stream_seek(stream, size - 2, SEEK_CUR);
            if (fs_res != LV_FS_RES_OK) {
                res = LV_RESULT_INVALID;
                LV_LOG_ERROR("read chunk data failed");
                goto read_err;
            }
        }
    }
read_err:
    return res;
}

static lv_result_t jpeg_decoder_info_from_stream(lv_stream_t *stream, lv_image_header_t *header, bool is_aicp)
{
    int width;
    int height;
    enum mpp_pixel_format format = MPP_FMT_ARGB_8888;

#ifdef AIC_MPP_AICP_DEC_ENABLE
    if(is_aicp) {
        uint8_t aicp_header[4];
        uint32_t read_num;
        lv_fs_res_t res = lv_aic_stream_read(stream, aicp_header, 4, &read_num);
        if(res != LV_FS_RES_OK || read_num != 4 || memcmp(aicp_header, "AICP", 4) != 0)
            return LV_RESULT_INVALID;
    }
#endif

    if(jpeg_get_img_size(stream, &width, &height, &format, is_aicp) != LV_RESULT_OK)
        return LV_RESULT_INVALID;

#if defined(MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE)
    if(!is_aicp) {
        int size_shift = jpeg_size_limit(width, height);
        width = width >> size_shift;
        height = height >> size_shift;
        header->reserved_2 = size_shift;
    }
#endif

    header->w = width;
    header->h = height;
    header->cf = mpp_fmt_to_lv_fmt(format);

    if(!lv_fmt_is_yuv(header->cf))
        header->stride = mpp_get_rgb_stride(width, format);

    return LV_RESULT_OK;
}

lv_result_t lv_jpeg_decoder_info(const char *src, lv_image_header_t *header, uint32_t size, bool is_file, bool is_aicp)
{
    lv_fs_res_t res;
    lv_stream_t stream;

    if(is_file)
        res = lv_aic_stream_open_file(&stream, src);
    else
        res = lv_aic_stream_open_buf(&stream, src, size);

    if(res != LV_FS_RES_OK) return LV_RESULT_INVALID;

    res = jpeg_decoder_info_from_stream(&stream, header, is_aicp);
    lv_aic_stream_close(&stream);
    return res;
}

static inline lv_result_t check_png_sig(lv_stream_t *stream)
{
    uint32_t read_num;
    unsigned char buf[64];
    uint64_t sig;

    lv_aic_stream_read(stream, buf, 8, &read_num);
    sig = stream_to_u64(buf);
    if (sig != PNGSIG && sig != MNGSIG) {
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

static lv_result_t png_decoder_info_from_stream(lv_stream_t *stream, lv_image_header_t *header)
{
    uint32_t read_num;
    uint8_t buf[64];
    int color_type;
    lv_fs_res_t res;

    res = lv_aic_stream_read(stream, buf, PNG_HEADER_SIZE, &read_num);
    if(res != LV_FS_RES_OK || read_num != PNG_HEADER_SIZE)
        return LV_RESULT_INVALID;

    uint64_t sig = stream_to_u64(buf);
    if(sig != PNGSIG && sig != MNGSIG)
        return LV_RESULT_INVALID;

    header->w = stream_to_u32(buf + 8 + 8);
    header->h = stream_to_u32(buf + 8 + 8 + 4);

    color_type = buf[8 + 8 + 8 + 1];
    if(color_type == 2) {
        header->cf = mpp_fmt_to_lv_fmt(MPP_FMT_RGB_888);
        header->stride = mpp_get_rgb_stride(header->w, MPP_FMT_RGB_888);
    } else {
        header->cf = mpp_fmt_to_lv_fmt(MPP_FMT_ARGB_8888);
        header->stride = mpp_get_rgb_stride(header->w, MPP_FMT_ARGB_8888);
    }

    return LV_RESULT_OK;
}

lv_result_t lv_png_decoder_info(const char *src, lv_image_header_t *header, uint32_t size, bool is_file)
{
    lv_fs_res_t res;
    lv_stream_t stream;

    if(is_file)
        res = lv_aic_stream_open_file(&stream, src);
    else
        res = lv_aic_stream_open_buf(&stream, src, size);

    if(res != LV_FS_RES_OK) return LV_RESULT_INVALID;

    res = png_decoder_info_from_stream(&stream, header);
    lv_aic_stream_close(&stream);
    return res;
}

static lv_res_t fake_decoder_info(const void *src, lv_image_header_t *header)
{
    int width;
    int height;
    int alpha_en;
    unsigned int color;

    {
        char *p = (char *)src + 3;
        char *n;
        width = (int)strtol(p, &n, 10);
        height = (int)strtol(n + 1, &n, 10);
        alpha_en = (int)strtol(n + 1, &n, 10);
        color = (unsigned int)strtoul(n + 1, NULL, 16);
    }
    header->w = width;
    header->h = height;
    LV_UNUSED(alpha_en);
    LV_UNUSED(color);
    header->stride = mpp_get_rgb_stride(header->w, MPP_FMT_ARGB_8888);
    header->cf = LV_COLOR_FORMAT_RAW;
    return LV_RES_OK;
}

static lv_result_t lv_mpp_dec_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc, lv_image_header_t *header)
{
    const void *src = dsc->src;
    char* ptr = NULL;
    lv_result_t res = LV_RESULT_INVALID;

    LV_UNUSED(decoder);

    if (lv_image_src_get_type(src) == LV_IMAGE_SRC_FILE) {
        ptr = strrchr(src, '.');
        if (!ptr) return LV_RESULT_INVALID;

        /* dsc->file is already opened by image_decoder_get_info(),
         * wrap it in a stream directly. */
        lv_stream_t stream;
        lv_memcpy(&stream.f, &dsc->file, sizeof(lv_fs_file_t));
        stream.is_file = 1;
        stream.cur_index = 0;

        if (!strncmp(ptr, ".png", 4)) {
            res = png_decoder_info_from_stream(&stream, header);
        } else if (image_suffix_is_jpg(ptr)) {
            res = jpeg_decoder_info_from_stream(&stream, header, false);
#ifdef AIC_MPP_AICP_DEC_ENABLE
        } else if (image_suffix_is_aicp(ptr)) {
            res = jpeg_decoder_info_from_stream(&stream, header, true);
#endif
        } else if (!strncmp(ptr, ".fake", 5)) {
            res = fake_decoder_info(src, header);
        }
#if LV_USE_AIC_BMP
        else if (!strncmp(ptr, ".bmp", 4) || !strncmp(ptr, ".BMP", 4)) {
            res = lv_bmp_decoder_info_from_stream(&stream, header);
        }
#endif

        return res;
    } else if (lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        lv_color_format_t cf = ((lv_img_dsc_t *)src)->header.cf;
        char *data = (char *)((lv_img_dsc_t *)src)->data;
        uint32_t data_size = ((lv_img_dsc_t *)src)->data_size;

        if (cf == LV_COLOR_FORMAT_RAW || cf == LV_COLOR_FORMAT_RAW) {
            bool is_aicp_data = false;
#ifdef AIC_MPP_AICP_DEC_ENABLE
            // Check for AICP header
            if (data_size >= 4 && memcmp(data, "AICP", 4) == 0) {
                is_aicp_data = true;
            }
#endif
            res = lv_jpeg_decoder_info(data, header, data_size, false, is_aicp_data);

            if (res != LV_RESULT_OK) {
                res = lv_png_decoder_info(data, header, data_size, false);
#if LV_USE_AIC_BMP
                if (res != LV_RESULT_OK) {
                    res = lv_bmp_decoder_info(data, header, data_size, false);
                }
#endif
            }
        }
    }

    return res;
}

struct ext_frame_allocator {
    struct frame_allocator base;
    struct mpp_frame* frame;
};

static int alloc_frame_buffer(struct frame_allocator *p, struct mpp_frame* frame,
                              int width, int height, enum mpp_pixel_format format)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)p;

    memcpy(frame, impl->frame, sizeof(struct mpp_frame));
    return 0;
}

static int free_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    return 0;
}

static int close_allocator(struct frame_allocator *p)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)p;

    free(impl);

    return 0;
}

static struct alloc_ops def_ops = {
    .alloc_frame_buffer = alloc_frame_buffer,
    .free_frame_buffer = free_frame_buffer,
    .close_allocator = close_allocator,
};

struct frame_allocator* lv_open_allocator(struct mpp_frame* frame)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)malloc(sizeof(struct ext_frame_allocator));

    if(impl == NULL) {
        return NULL;
    }

    memset(impl, 0, sizeof(struct ext_frame_allocator));

    impl->frame = frame;
    impl->base.ops = &def_ops;
    return &impl->base;
}

void lv_frame_buf_free(mpp_decoder_data_t *mpp_data)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (mpp_data->data[i]) {
            mpp_dec_free_oom((void*)mpp_data->data[i]);
            mpp_data->data[i] = NULL;
        }
    }
}

lv_result_t lv_frame_buf_alloc(mpp_decoder_data_t *mpp_data, struct mpp_buf *alloc_buf,
                              int *size, uint32_t cf)
{
    int i;
    int data_size = 0;
    for (i = 0; i < 3; i++) {
        if (size[i]) {
            mpp_data->data[i] = (uint8_t *)mpp_dec_malloc_oom(size[i] + CACHE_LINE_SIZE - 1);
            if (!mpp_data->data[i]) {
                goto alloc_error;
            } else {
                unsigned int align_addr = (unsigned int)ALIGN_UP((ulong)mpp_data->data[i], CACHE_LINE_SIZE);
                alloc_buf->phy_addr[i] = align_addr;
                aicos_dcache_clean_invalid_range((ulong *)(ulong)align_addr, ALIGN_UP(size[i], CACHE_LINE_SIZE));
                data_size += size[i];
            }
        }
    }

    mpp_data->decoded.header.w = alloc_buf->size.width;
    mpp_data->decoded.header.h = alloc_buf->size.height;
    mpp_data->decoded.header.cf = cf;
    mpp_data->decoded.header.magic = LV_IMAGE_HEADER_MAGIC;

    if (lv_fmt_is_yuv(cf)) {
        mpp_data->decoded.data =(uint8_t *)(&mpp_data->dec_buf);
        mpp_data->decoded.data_size = data_size;
        mpp_data->decoded.unaligned_data = (uint8_t *)(&mpp_data->dec_buf);
    } else {
        mpp_data->decoded.header.stride = alloc_buf->stride[0];
        mpp_data->decoded.data =(uint8_t *)((ulong)alloc_buf->phy_addr[0]);
        mpp_data->decoded.data_size = size[0];
        mpp_data->decoded.unaligned_data = mpp_data->data[0];
    }
    
    /*
     * FIXME: MPP alloc/free uses mpp_dec_malloc_oom/mpp_dec_free_oom, which
     * adds AIOCS_OOM_SLOTS overhead. The pointers stored in unaligned_data
     * and data do NOT match what default_handlers->buf_free_cb expects.
     *
     * Currently handlers->buf_free_cb is never called on MPP buffers because:
     * - flags does not include LV_IMAGE_FLAGS_ALLOCATED → lv_draw_buf_destroy skips free
     * - flags does not include LV_IMAGE_FLAGS_MODIFIABLE → flush_cache is skipped
     * - MPP close path uses mpp_dec_data_release → lv_frame_buf_free → mpp_dec_free_oom
     *
     * handlers is set here only to pass the LV_ASSERT in lv_image_decoder.c:173.
     * A proper long-term fix would be to register MPP-specific handlers with
     * no-op or MPP-aware free/invalidate/flush callbacks.
     */
    mpp_data->decoded.handlers = lv_draw_buf_get_handlers();

    return LV_RESULT_OK;
alloc_error:
    lv_frame_buf_free(mpp_data);
    return LV_RESULT_INVALID;
}

void lv_set_frame_buf_size(struct mpp_frame *frame, int *buf_size, int size_shift, bool is_aicp)
{
    int height_align;
    int width = frame->buf.size.width;
    int height = frame->buf.size.height;

    if (size_shift > 0) {
        height_align = ALIGN_16B(ALIGN_16B(height) >> size_shift);
        frame->buf.size.width =  width >> size_shift;;
        frame->buf.size.height = height >> size_shift;
    } else {
        height_align = ALIGN_16B(height);
    }

    switch (frame->buf.format) {
    case MPP_FMT_YUV420P:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift);
        else
            frame->buf.stride[0] =  ALIGN_16B(width);

        frame->buf.stride[1] =  frame->buf.stride[0] >> 1;
        frame->buf.stride[2] =  frame->buf.stride[0] >> 1;
        buf_size[0] = frame->buf.stride[0] * height_align;
        buf_size[1] = frame->buf.stride[1] * (height_align >> 1);
        buf_size[2] = frame->buf.stride[2] * (height_align >> 1);
        break;
    case MPP_FMT_YUV422P:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift);
        else
            frame->buf.stride[0] =  ALIGN_16B(width);

        frame->buf.stride[1] =  frame->buf.stride[0] >> 1;
        frame->buf.stride[2] =  frame->buf.stride[0] >> 1;
        buf_size[0] = frame->buf.stride[0] * height_align;
        buf_size[1] = frame->buf.stride[1] * height_align;
        buf_size[2] = frame->buf.stride[2] * height_align;
        break;
    case MPP_FMT_YUV444P:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift);
        else
            frame->buf.stride[0] =  ALIGN_16B(width);

        frame->buf.stride[1] =  frame->buf.stride[0];
        frame->buf.stride[2] =  frame->buf.stride[0];
        buf_size[0] = frame->buf.stride[0] * height_align;
        buf_size[1] = frame->buf.stride[1] * height_align;
        buf_size[2] = frame->buf.stride[2] * height_align;
        break;
    case MPP_FMT_RGB_565:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift) * 2;
        else
            frame->buf.stride[0] =  ALIGN_16B(width) * 2;

        buf_size[0] = frame->buf.stride[0] * height_align;
        break;
    case MPP_FMT_RGB_888:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift) * 3;
        else
            frame->buf.stride[0] =  ALIGN_16B(width) * 3;

        buf_size[0] = frame->buf.stride[0] * height_align;
        break;
    case MPP_FMT_ARGB_8888:
        if (size_shift > 0) {
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift) * 4;
            buf_size[0] = frame->buf.stride[0] * height_align;
        } else {
            if (is_aicp) {
                // Use 8-byte alignment for AICP format as requested
                frame->buf.stride[0] =  ALIGN_UP(width, 8) * 4;
                buf_size[0] = frame->buf.stride[0] * ALIGN_UP(height, 8);
            } else {
                frame->buf.stride[0] =  ALIGN_16B(width * 4);
                buf_size[0] = ALIGN_UP(frame->buf.stride[0] * height, CACHE_LINE_SIZE);
            }
        }
        break;
    default:
        LV_LOG_ERROR("unsupported format:%d", frame->buf.format);
        break;
    }
}

static enum mpp_codec_type detect_codec_type(lv_stream_t *stream, lv_image_decoder_dsc_t *dsc)
{
    enum mpp_codec_type type = MPP_CODEC_VIDEO_DECODER_PNG;
    char* ptr = NULL;

    if (lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_FILE) {
        ptr = strrchr(dsc->src, '.');
        if (ptr && image_suffix_is_jpg(ptr)) {
            type = MPP_CODEC_VIDEO_DECODER_MJPEG;
#ifdef AIC_MPP_AICP_DEC_ENABLE
        } else if (ptr && image_suffix_is_aicp(ptr)) {
            type = MPP_CODEC_VIDEO_DECODER_AICP;
#endif
        }
    } else if (lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_VARIABLE) {
#ifdef AIC_MPP_AICP_DEC_ENABLE
        uint8_t aicp_header[4];
        uint32_t read_num;
        lv_result_t res;

        // Check for AICP header first
        res = lv_aic_stream_read(stream, aicp_header, 4, &read_num);
        if (res == LV_FS_RES_OK && read_num == 4 && memcmp(aicp_header, "AICP", 4) == 0) {
            type = MPP_CODEC_VIDEO_DECODER_AICP;
        } else {
            lv_aic_stream_reset(stream);
#endif
            if (check_jpeg_soi(stream) == LV_RESULT_OK) {
                type = MPP_CODEC_VIDEO_DECODER_MJPEG;
            } else {
                lv_aic_stream_reset(stream);
            }
#ifdef AIC_MPP_AICP_DEC_ENABLE
        }
#endif
        lv_aic_stream_reset(stream);
    }

    return type;
}

static lv_result_t lv_mpp_dec_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    lv_result_t res = LV_RESULT_OK;
    uint32_t file_len = 0;
    lv_stream_t stream;
    struct mpp_packet packet;
    int width = 0;
    int height = 0;
    enum mpp_codec_type type;
    struct decode_config config = { 0 };
    int buf_size[3] = { 0 };
    struct mpp_decoder *dec = NULL;
    struct frame_allocator *allocator = NULL;
    int size_shift = 0;
    bool is_aicp = false;

#if LV_USE_AIC_BMP
    if (lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_FILE) {
        const char *ext = lv_fs_get_ext(dsc->src);
        if (lv_strcmp(ext, "bmp") == 0 || lv_strcmp(ext, "BMP") == 0) {
            return lv_bmp_dec_open(decoder, dsc);
        }
    } else if (lv_image_src_get_type(dsc->src) == LV_IMAGE_SRC_VARIABLE) {
        if (lv_aic_stream_open(&stream, dsc->src) != LV_FS_RES_OK)
            return LV_RESULT_INVALID;

        if (lv_check_bmp_header(&stream) == LV_RESULT_OK) {
            lv_aic_stream_close(&stream);
            return lv_bmp_dec_open(decoder, dsc);
        }

        lv_aic_stream_close(&stream);
    }
#endif

    if (lv_aic_stream_open(&stream, dsc->src) != LV_FS_RES_OK)
        return LV_RESULT_INVALID;

    mpp_decoder_data_t *mpp_data = (mpp_decoder_data_t *)lv_malloc_zeroed(sizeof(mpp_decoder_data_t));
    CHECK_PTR(mpp_data);

    width = dsc->header.w;
    height = dsc->header.h;
    config.pix_fmt = lv_fmt_to_mpp_fmt(dsc->header.cf);

    // Detect codec type
    type = detect_codec_type(&stream, dsc);

    lv_aic_stream_get_size(&stream, &file_len);
    dec = mpp_decoder_create(type);
    CHECK_PTR(dec);

    config.bitstream_buffer_size = ALIGN_UP(file_len, 256);
    config.extra_frame_num = 0;
    config.packet_count = 1;

    struct mpp_frame dec_frame;
    memset(&dec_frame, 0, sizeof(struct mpp_frame));
    dec_frame.buf.size.width = width;
    dec_frame.buf.size.height = height;
    dec_frame.buf.format = config.pix_fmt;
    dec_frame.buf.buf_type = MPP_PHY_ADDR;

#if defined(MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE)
    if (type == MPP_CODEC_VIDEO_DECODER_MJPEG)
        size_shift = dsc->header.reserved_2;
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

    CHECK_RET(lv_frame_buf_alloc(mpp_data, &dec_frame.buf, buf_size, dsc->header.cf), LV_RESULT_OK);

    // allocator will be released in decoder
    allocator = lv_open_allocator(&dec_frame);
    CHECK_PTR(allocator);

    mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_EXT_FRAME_ALLOCATOR, (void*)allocator);
    CHECK_RET(mpp_decoder_init(dec, &config), 0);
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, file_len);

    uint32_t read_size = 0;

    lv_aic_stream_read(&stream, packet.data, file_len, &read_size);
    packet.size = file_len;
    packet.flag = PACKET_FLAG_EOS;

    mpp_decoder_put_packet(dec, &packet);
    CHECK_RET(mpp_decoder_decode(dec), 0);

    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(dec, &frame);

    memcpy(&mpp_data->dec_buf, &frame.buf, sizeof(struct mpp_buf));
    mpp_decoder_put_frame(dec, &frame);
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
    if (dec)
        mpp_decoder_destory(dec);

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

static void mpp_dec_data_release(mpp_decoder_data_t *decoder_data)
{
    if (decoder_data) {
        lv_frame_buf_free(decoder_data);
        lv_free(decoder_data);
    }
}

static void lv_mpp_dec_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc)
{
    mpp_decoder_data_t *mpp_data =  (mpp_decoder_data_t *)dsc->decoded;
    LV_UNUSED(decoder);

    if (!mpp_data->cached) {
        mpp_dec_data_release(mpp_data);
    }

    return;
}

static void mpp_dec_cache_free_cb(void *node, void *user_data)
{
    lv_image_cache_data_t *entry = (lv_image_cache_data_t *)node;
    LV_UNUSED(user_data);

    /* MPP-decoded entries have user_data pointing to mpp_decoder_data_t */
    if(entry->user_data) {
        mpp_decoder_data_t *mpp_data = (mpp_decoder_data_t *)entry->user_data;

        mpp_dec_data_release(mpp_data);

        if(entry->src_type == LV_IMAGE_SRC_FILE)
            lv_free((void *)entry->src);
        return;
    }

    /* Default cleanup for non-MPP entries */
    lv_draw_buf_t *decoded = (lv_draw_buf_t *)entry->decoded;
    if(decoded && lv_draw_buf_has_flag(decoded, LV_IMAGE_FLAGS_ALLOCATED)) {
        lv_draw_buf_destroy(decoded);
    }

    if(entry->src_type == LV_IMAGE_SRC_FILE)
        lv_free((void *)entry->src);
}

void lv_mpp_dec_init(void)
{
    lv_image_decoder_t *dec = lv_image_decoder_create();

    lv_image_decoder_set_info_cb(dec, lv_mpp_dec_info);
    lv_image_decoder_set_open_cb(dec, lv_mpp_dec_open);
    lv_image_decoder_set_close_cb(dec, lv_mpp_dec_close);

    /* Replace image cache free callback to handle MPP-decoded entries */
    if(lv_image_cache_is_enabled()) {
        lv_cache_set_free_cb(img_cache_p, mpp_dec_cache_free_cb, NULL);
    }
}

void lv_mpp_dec_deinit(void)
{
    lv_image_decoder_t * dec = NULL;
    while ((dec = lv_image_decoder_get_next(dec)) != NULL) {
        if (dec->info_cb == lv_mpp_dec_info) {
            lv_image_decoder_delete(dec);
            break;
        }
    }
}

