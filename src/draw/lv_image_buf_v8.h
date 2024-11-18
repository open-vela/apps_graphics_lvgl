/**
 * @file lv_image_buf_v8.h
 *
 */

#ifndef LV_IMAGE_BUF_V8_H
#define LV_IMAGE_BUF_V8_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include <stdint.h>
#include "lv_image_dsc.h"

/*********************
 *      DEFINES
 *********************/
#define LV_IMAGE_RLE_HEADER_MAGIC        0x5aa521e0

/**********************
 *      TYPEDEFS
 **********************/

/**
 * The first 8 bit is very important to distinguish the different source types.
 * For more info see `lv_img_get_src_type()` in lv_img.c
 * On big endian systems the order is reversed so cf and always_zero must be at
 * the end of the struct.
 */
#if LV_BIG_ENDIAN_SYSTEM
typedef struct {

    uint32_t h : 11; /*Height of the image map*/
    uint32_t w : 11; /*Width of the image map*/
    uint32_t reserved : 2; /*Reserved to be used later*/
    uint32_t always_zero : 3; /*It the upper bits of the first byte. Always zero to look like a
                                 non-printable character*/
    uint32_t cf : 5;          /*Color format: See `lv_img_color_format_t`*/

} lv_image_header_v8_t;
#else
typedef struct {

    uint32_t cf : 5;          /*Color format: See `lv_img_color_format_t`*/
    uint32_t always_zero : 3; /*It the upper bits of the first byte. Always zero to look like a
                                 non-printable character*/
    uint32_t tiled : 1;       /* 1: tiled, 0: not tiled*/
    uint32_t reserved : 1; /*Reserved to be used later*/

    uint32_t w : 11; /*Width of the image map*/
    uint32_t h : 11; /*Height of the image map*/
} lv_image_header_v8_t;
#endif

typedef struct {
    uint32_t magic;         /* 0x5aa521e0 */
    uint32_t blksize: 4;    /* block size, the encoded data unit bytes. */
    uint32_t len_orig: 24;  /* Original data length. */
    uint32_t reserved: 4;
} lv_rle_header_v8_t;

typedef struct {
    lv_image_header_v8_t header;
    lv_rle_header_v8_t rleheader;
} lv_bin_file_header_v8_t;

/**********************
 *      MACROS
 **********************/

/**
 * Convert a v8 header to a v9 header.
 * @param src_header the v9 header to convert
 */
void lv_image_header_convert_from_v8(lv_image_header_t * src_header);

/**
 * get image header size accounting for the color format
 * @param dsc the image header
 * @return the size of image header
 */
uint32_t lv_image_header_get_size(lv_image_header_t * header);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_IMAGE_BUF_V8_H*/
