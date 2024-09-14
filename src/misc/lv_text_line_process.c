/**
* @file lv_text_line_process.c
*
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_text_line_process.h"
#include "lv_iter.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    const char * text;
    const lv_font_t * font;

    lv_text_line_process_line_info_t line_info_prev;
    bool has_prev_line_info;
    uint32_t max_width;
    uint32_t letter_space;
    uint8_t tab_width;
    bool long_break;
} lv_text_line_process_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_result_t line_iter_next_cb(void * instance, void * context, void * elem);

/**********************
 *  GLOBAL VARIABLES
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

lv_iter_t * lv_text_line_process_iter_create(const char * txt, const lv_font_t * font,
                                             uint16_t max_width, uint32_t letter_space, uint8_t tab_width, bool long_break)
{
    lv_iter_t * iter = lv_iter_create((void *)txt, sizeof(lv_text_line_process_line_info_t), sizeof(lv_text_line_process_t),
                                      line_iter_next_cb);

    if(iter == NULL) return NULL;

    lv_text_line_process_t * ctx = lv_iter_get_context(iter);
    ctx->text = txt;
    ctx->font = font;

    ctx->max_width = max_width;
    ctx->letter_space = letter_space;
    ctx->tab_width = tab_width;
    ctx->long_break = long_break;

    return iter;
}

void lv_text_line_process_iter_destroy(lv_iter_t * iter)
{
    lv_iter_destroy(iter);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_result_t line_iter_next_cb(void * instance, void * context, void * elem)
{
    LV_UNUSED(instance);

    lv_text_line_process_t * ctx = (lv_text_line_process_t *)context;

    lv_text_line_process_line_info_t line_info = {
        .pos = {
            .start = ctx->has_prev_line_info ? ctx->line_info_prev.pos.brk : 0,
            .end = 0,
            .brk = 0,
        },
        .line_height = 0,
        .line_spacing = 0,
        .real_width = 0,
        .ideal_width = 0,
    };

    lv_iter_t * word_iter = lv_text_word_process_iter_create(&ctx->text[line_info.pos.start], ctx->font,
                                                             ctx->letter_space, ctx->max_width, 0);
    if(word_iter == NULL) return LV_RESULT_INVALID;

    lv_iter_make_peekable(word_iter, 2);

    uint32_t end;
    uint32_t brk;
    bool is_line_leading = true;
    bool is_word_breakable = false;
    uint32_t real_width = 0;
    uint32_t ideal_width = 0;

    while(true) {
        lv_text_word_process_word_info_t word;
        lv_result_t res = lv_iter_peek(word_iter, &word);

        if(res == LV_RESULT_INVALID) {
            lv_text_word_process_iter_destroy(word_iter);
            return LV_RESULT_INVALID;
        }

        real_width += word.real_width;
        ideal_width += word.ideal_width;

        if(word.type == LV_TEXT_WORD_PROCESS_NEWLINE || word.type == LV_TEXT_WORD_PROCESS_RETURN) {
            end = word.pos.end;
            brk = word.pos.end;
            lv_iter_next(word_iter, NULL);
            break;
        }

        if(is_line_leading == true && ctx->long_break == true && word.pos.brk != UINT32_MAX &&
           !(word.type == LV_TEXT_WORD_PROCESS_RETURN || word.type == LV_TEXT_WORD_PROCESS_NEWLINE)) {
            end = word.pos.end;
            brk = word.pos.brk;
            break;
        }

        if(word.type == LV_TEXT_WORD_PROCESS_OPEN_PUNCTUATION || word.type == LV_TEXT_WORD_PROCESS_QUOTATION) {
            is_word_breakable = true;
            lv_iter_peek_advance(word_iter);
            lv_text_word_process_word_info_t word_next;
            res = lv_iter_peek(word_iter, &word_next);
            if(res == LV_RESULT_OK && word_next.pos.brk != UINT32_MAX && word_next.pos.brk != word_next.pos.end) {
                if(is_line_leading == true) {
                    continue;
                }

                end = word.pos.start;
                brk = word.pos.start;

                real_width -= word.real_width;
                ideal_width -= word.ideal_width;
                break;
            }
        }

        lv_iter_next(word_iter, NULL);

        lv_text_word_process_word_info_t word_next;
        res = lv_iter_peek(word_iter, &word_next);
        if(res == LV_RESULT_INVALID) {
            end = word.pos.end;
            brk = word.pos.end;
            break;
        }

        if(word_next.pos.brk != UINT32_MAX && word_next.pos.brk != word_next.pos.end) {
            end = word.pos.end;
            brk = word.pos.end;

            if(word_next.type == LV_TEXT_WORD_PROCESS_RETURN || word_next.type == LV_TEXT_WORD_PROCESS_NEWLINE) {
                lv_iter_next(word_iter, NULL);
                end++;
                brk++;
            }
            else if((word.type != LV_TEXT_WORD_PROCESS_CLOSE_PUNCTUATION || word.type != LV_TEXT_WORD_PROCESS_QUOTATION) &&
                    (word_next.type == LV_TEXT_WORD_PROCESS_CLOSE_PUNCTUATION || word_next.type == LV_TEXT_WORD_PROCESS_QUOTATION
                     || word_next.type == LV_TEXT_WORD_PROCESS_SPACE)) {
                if(is_line_leading || is_word_breakable) {
                    end = word_next.pos.end;
                    brk = word_next.pos.brk;
                }
                else {
                    end = word.pos.start;
                    brk = word.pos.start;
                }

                real_width -= word.real_width;
                ideal_width -= word.ideal_width;
            }
            break;
        }
        if(word.type == LV_TEXT_WORD_PROCESS_CJK
           || word.type == LV_TEXT_WORD_PROCESS_LATIN
           || word.type == LV_TEXT_WORD_PROCESS_NUMBER) {
            is_word_breakable = false;
        }

        is_line_leading = false;
    }

    lv_text_word_process_iter_destroy(word_iter);

    line_info.pos.end = line_info.pos.start + end;
    line_info.pos.brk = line_info.pos.start + brk;
    line_info.real_width = real_width;
    line_info.ideal_width = ideal_width;

    ctx->line_info_prev = line_info;
    ctx->has_prev_line_info = true;

    *(lv_text_line_process_line_info_t *)elem = line_info;

    return LV_RESULT_OK;
}
