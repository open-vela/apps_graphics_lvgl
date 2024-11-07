/**
 * @file lv_color_v8.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_color_v8.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_color_format_t lv_color_format_convert_from_v8(lv_color_format_v8_t cf)
{
    if(cf == LV_IMG_CF_TRUE_COLOR) {
#if LV_COLOR_DEPTH == 16
        return LV_COLOR_FORMAT_RGB565;
#elif LV_COLOR_DEPTH == 24
        return LV_COLOR_FORMAT_RGB888;
#elif LV_COLOR_DEPTH == 32
        return LV_COLOR_FORMAT_XRGB8888;
#endif
    }
    else if(cf == LV_IMG_CF_TRUE_COLOR_ALPHA) {
#if LV_COLOR_DEPTH == 16
        return LV_COLOR_FORMAT_ARGB8565;
#elif LV_COLOR_DEPTH == 24
        return LV_COLOR_FORMAT_ARGB8888;
#elif LV_COLOR_DEPTH == 32
        return LV_COLOR_FORMAT_ARGB8888;
#endif
    }

    return (lv_color_format_t)cf;
}