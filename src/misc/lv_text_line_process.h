/**
* @file lv_text_line_process.h
*
 */


#ifndef LV_TEXT_LINE_PROCESS_H
#define LV_TEXT_LINE_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_types.h"

#include "lv_assert.h"
#include "lv_text_word_process.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_text_position_t pos;
    uint32_t line_height;
    uint32_t line_spacing;
    uint32_t real_width;
    uint32_t ideal_width;
} lv_text_line_process_line_info_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_iter_t * lv_text_line_process_iter_create(const char * txt, const lv_font_t * font,
                                             uint16_t max_width, uint32_t letter_space, uint8_t tab_width, bool long_break);

void lv_text_line_process_iter_destroy(lv_iter_t * iter);

/*************************
 *    GLOBAL VARIABLES
 *************************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TEXT_LINE_PROCESS_H*/
