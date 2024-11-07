#ifndef LV_COLOT_V8_H
#define LV_COLOT_V8_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"
#include "lv_color.h"
#include "../draw/lv_draw_buf.h"
#include "lv_types.h"
#include "../draw/lv_image_dsc.h"
#include "../draw/lv_image_decoder.h"

/*Image color format*/
enum {
    LV_IMG_CF_UNKNOWN = 0,

    LV_IMG_CF_RAW,              /**< Contains the file as it is. Needs custom decoder function*/
    LV_IMG_CF_RAW_ALPHA,        /**< Contains the file as it is. The image has alpha. Needs custom decoder
                                   function*/
    LV_IMG_CF_RAW_CHROMA_KEYED, /**< Contains the file as it is. The image is chroma keyed. Needs
                                   custom decoder function*/

    LV_IMG_CF_TRUE_COLOR,              /**< Color format and depth should match with LV_COLOR settings*/
    LV_IMG_CF_TRUE_COLOR_ALPHA,        /**< Same as `LV_IMG_CF_TRUE_COLOR` but every pixel has an alpha byte*/
    LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED, /**< Same as `LV_IMG_CF_TRUE_COLOR` but LV_COLOR_TRANSP pixels
                                          will be transparent*/

    LV_IMG_CF_INDEXED_1BIT, /**< Can have 2 different colors in a palette (can't be chroma keyed)*/
    LV_IMG_CF_INDEXED_2BIT, /**< Can have 4 different colors in a palette (can't be chroma keyed)*/
    LV_IMG_CF_INDEXED_4BIT, /**< Can have 16 different colors in a palette (can't be chroma keyed)*/
    LV_IMG_CF_INDEXED_8BIT, /**< Can have 256 different colors in a palette (can't be chroma keyed)*/

    LV_IMG_CF_ALPHA_1BIT, /**< Can have one color and it can be drawn or not*/
    LV_IMG_CF_ALPHA_2BIT, /**< Can have one color but 4 different alpha value*/
    LV_IMG_CF_ALPHA_4BIT, /**< Can have one color but 16 different alpha value*/
    LV_IMG_CF_ALPHA_8BIT, /**< Can have one color but 256 different alpha value*/

    LV_IMG_CF_RGB888,
    LV_IMG_CF_RGBA8888,
    LV_IMG_CF_RGBX8888,
    LV_IMG_CF_RGB565,
    LV_IMG_CF_RGBA5658,
    LV_IMG_CF_RGB565A8,

    LV_IMG_CF_RESERVED_15,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_16,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_17,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_18,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_19,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_20,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_21,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_22,              /**< Reserved for further use.*/
    LV_IMG_CF_RESERVED_23,              /**< Reserved for further use.*/

    LV_IMG_CF_USER_ENCODED_0,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_1,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_2,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_3,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_4,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_5,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_6,          /**< User holder encoding format.*/
    LV_IMG_CF_USER_ENCODED_7,          /**< User holder encoding format.*/
};
typedef uint8_t lv_color_format_v8_t;

/**
 * Convert lv_color_format_v8_t to lv_color_format_t
 * @param cf the color format v8
 * @return the color format
 */
lv_color_format_t lv_color_format_convert_from_v8(lv_color_format_v8_t cf);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  /*LV_COLOT_V8_H*/