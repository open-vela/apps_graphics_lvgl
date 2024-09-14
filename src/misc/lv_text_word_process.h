/**
* @file lv_text_word_process.h
*
 */


#ifndef LV_TEXT_WORD_PROCESS_H
#define LV_TEXT_WORD_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_types.h"

#include "lv_assert.h"

/*********************
 *      DEFINES
 *********************/
typedef enum {
    LV_TEXT_WORD_PROCESS_LATIN,
    LV_TEXT_WORD_PROCESS_CJK,
    LV_TEXT_WORD_PROCESS_HYPHEN,
    LV_TEXT_WORD_PROCESS_NUMBER,
    LV_TEXT_WORD_PROCESS_OPEN_PUNCTUATION,
    LV_TEXT_WORD_PROCESS_CLOSE_PUNCTUATION,
    LV_TEXT_WORD_PROCESS_RETURN,
    LV_TEXT_WORD_PROCESS_NEWLINE,
    LV_TEXT_WORD_PROCESS_SPACE,
    LV_TEXT_WORD_PROCESS_TAB,
    LV_TEXT_WORD_PROCESS_QUOTATION,
    LV_TEXT_WORD_PROCESS_UNKNOWN,
} lv_text_word_process_word_type_t;

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t brk;
} lv_text_position_t;

typedef struct {
    lv_text_position_t pos;
    lv_text_word_process_word_type_t type;
    uint32_t real_width;
    uint32_t ideal_width;
} lv_text_word_process_word_info_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_iter_t * lv_text_char_process_iter_create(const char * txt);

void lv_text_char_process_iter_destroy(lv_iter_t * iter);

lv_iter_t * lv_text_word_process_iter_create(const char * txt, const lv_font_t * font, int32_t letter_space,
                                             int32_t remaining_width, uint8_t flag);

void lv_text_word_process_iter_destroy(lv_iter_t * iter);

/*************************
 *    GLOBAL VARIABLES
 *************************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TEXT_WORD_PROCESS_H*/
