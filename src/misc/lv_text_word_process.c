/**
* @file lv_text_word_process.c
*
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_iter.h"
#include "lv_text_word_process.h"
#include "lv_text_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_iter_t * char_iter;
    const lv_font_t * font;
    int32_t letter_space;
    int32_t remaining_width;
    uint8_t flag;

    lv_text_word_process_word_info_t word_info_pre;
    bool has_word_info_pre;
} lv_text_word_process_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_text_word_process_word_type_t text_word_type_get(uint32_t ch);
static uint32_t len_unicode(uint32_t ch);

static lv_result_t char_iter_next_cb(void * instance, void * context, void * elem);

static lv_result_t word_iter_next_cb(void * instance, void * context, void * elem);

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

lv_iter_t * lv_text_char_process_iter_create(const char * txt)
{
    lv_iter_t * iter = lv_iter_create((void *)txt, sizeof(uint32_t), sizeof(uint32_t),
                                      char_iter_next_cb);

    if(iter == NULL) return NULL;

    return iter;
}

void lv_text_char_process_iter_destroy(lv_iter_t * iter)
{
    lv_iter_destroy(iter);
}

lv_iter_t * lv_text_word_process_iter_create(const char * txt, const lv_font_t * font, int32_t letter_space,
                                             int32_t remaining_width, uint8_t flag)
{
    lv_iter_t * iter = lv_iter_create((void *)txt, sizeof(lv_text_word_process_word_info_t), sizeof(lv_text_word_process_t),
                                      word_iter_next_cb);

    if(iter == NULL) return NULL;

    lv_text_word_process_t * ctx = lv_iter_get_context(iter);

    ctx->char_iter = lv_text_char_process_iter_create(txt);

    if(ctx->char_iter == NULL) {
        lv_iter_destroy(iter);
        return NULL;
    }

    ctx->font = font;
    ctx->letter_space = letter_space;
    ctx->remaining_width = remaining_width;
    ctx->flag = flag;

    lv_iter_make_peekable(ctx->char_iter, 2);

    return iter;
}

void lv_text_word_process_iter_destroy(lv_iter_t * iter)
{
    lv_text_word_process_t * ctx = lv_iter_get_context(iter);
    if(ctx->char_iter) lv_text_char_process_iter_destroy(ctx->char_iter);
    lv_iter_destroy(iter);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_result_t char_iter_next_cb(void * instance, void * context, void * elem)
{
    const char * txt = (const char *)instance;
    uint32_t * index = (uint32_t *)context;
    uint32_t * unicode = (uint32_t *)elem;

    if(*index == UINT32_MAX) return LV_RESULT_INVALID;

    uint32_t u = lv_text_encoded_next(txt, index);

    if(u == '\0') {
        *index = UINT32_MAX;
        return LV_RESULT_INVALID;
    }

    if(unicode) *unicode = u;

    return LV_RESULT_OK;
}

static lv_result_t word_iter_next_cb(void * instance, void * context, void * elem)
{
    LV_UNUSED(instance);

    lv_text_word_process_t * ctx = (lv_text_word_process_t *)context;
    lv_text_word_process_word_info_t * word_info = (lv_text_word_process_word_info_t *)elem;
    lv_iter_t * char_iter = ctx->char_iter;

    const uint32_t start = ctx->has_word_info_pre ? ctx->word_info_pre.pos.end : 0;

    uint32_t word_pos_end = start;
    lv_text_word_process_word_type_t word_type = LV_TEXT_WORD_PROCESS_UNKNOWN;
    int32_t word_width = 0;
    uint32_t brk_pos = ctx->has_word_info_pre ? ctx->word_info_pre.pos.brk : UINT32_MAX;
    int32_t real_width = 0;

    while(true) {
        uint32_t ch;
        lv_result_t res = lv_iter_peek(char_iter, &ch);

        if(res == LV_RESULT_INVALID) return LV_RESULT_INVALID;

        uint32_t char_len = len_unicode(ch);

        if(word_type == LV_TEXT_WORD_PROCESS_UNKNOWN) {
            word_type = text_word_type_get(ch);
        }

        lv_iter_next(char_iter, NULL);

        uint32_t ch_next;
        res = lv_iter_peek(ctx->char_iter, &ch_next);
        if(res == LV_RESULT_INVALID) {
            ch_next = '\0';
        }

        int32_t char_width = lv_font_get_glyph_width(ctx->font, ch, ch_next);
        int32_t char_width_next = lv_font_get_glyph_width(ctx->font, ch_next, 0);
        lv_text_word_process_word_type_t word_type_next = text_word_type_get(ch_next);

        word_pos_end += char_len;
        word_width += char_width;

        if(word_width + char_width_next > ctx->remaining_width) {
            if(brk_pos == UINT32_MAX) {
                brk_pos = word_pos_end;
                real_width = word_width;
            }
        }

        switch(word_type) {
            case LV_TEXT_WORD_PROCESS_LATIN:
                if(word_type_next == LV_TEXT_WORD_PROCESS_LATIN || word_type_next == LV_TEXT_WORD_PROCESS_NUMBER) continue;
                else break;
            case LV_TEXT_WORD_PROCESS_CJK:
                break;
            case LV_TEXT_WORD_PROCESS_HYPHEN:
                if(word_type_next == LV_TEXT_WORD_PROCESS_LATIN || word_type_next == LV_TEXT_WORD_PROCESS_NUMBER) continue;
                else break;
            case LV_TEXT_WORD_PROCESS_NUMBER:
                if(word_type_next == LV_TEXT_WORD_PROCESS_NUMBER) continue;
                else break;
            case LV_TEXT_WORD_PROCESS_OPEN_PUNCTUATION:
                if(word_type_next == LV_TEXT_WORD_PROCESS_OPEN_PUNCTUATION) continue;
                else break;
            case LV_TEXT_WORD_PROCESS_CLOSE_PUNCTUATION:
                if(word_type_next == LV_TEXT_WORD_PROCESS_CLOSE_PUNCTUATION) continue;
                else break;
            case LV_TEXT_WORD_PROCESS_QUOTATION:
                if(word_type_next == LV_TEXT_WORD_PROCESS_QUOTATION) continue;
                else break;
            case LV_TEXT_WORD_PROCESS_RETURN:
                break;
            case LV_TEXT_WORD_PROCESS_NEWLINE:
                brk_pos = word_pos_end - 1;
                break;
            case LV_TEXT_WORD_PROCESS_SPACE:
                break;
            case LV_TEXT_WORD_PROCESS_TAB:
                break;
            case LV_TEXT_WORD_PROCESS_UNKNOWN:
                break;
        }

        break;
    }

    if(real_width == 0) {
        real_width = word_width;
    }

    if(ctx->remaining_width >= real_width) {
        ctx->remaining_width -= real_width;
    }

    lv_text_word_process_word_info_t info = {
        .pos = {
            .start = start,
            .end = word_pos_end,
            .brk = brk_pos,
        },
        .type = word_type,
        .real_width = real_width,
        .ideal_width = word_width,
    };

    ctx->word_info_pre = info;
    ctx->has_word_info_pre = true;

    if(word_info) *word_info = info;

    return LV_RESULT_OK;
}

static bool is_latin(uint32_t ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static bool is_cjk(uint32_t ch)
{
    return ch >= 0x4e00 && ch <= 0x9fff;
}

static bool is_open_punctuation(uint32_t ch)
{
    const uint32_t open_punctuations[] = {
        0x28, // '('
        0x5b, // '['
        0x7b, // '{'
        0x3c, // '<'
        0xff08, // '（'
        0x300c, // '「'
        0x300e, // '『'
        0x3010, // '【'
        0x3014, // '〔'
        0x3008, // '〈'
        0x300a, // '《'
        0x2997, // '⦗'
        0x27e8, // '⟨'
        0x2018, // '‘'
        0x201c, // '“'
        0
    };
    for(int i = 0; open_punctuations[i]; i++) if(ch == open_punctuations[i]) return true;
    return false;
}

static bool is_close_punctuation(uint32_t ch)
{
    const uint32_t close_punctuations[] = {
        0x2e, // '.'
        0x2c, // ','
        0x3b, // ';'
        0x3a, // ':'
        0x21, // '!'
        0x3f, // '?'
        0x3002, // '。'
        0xff0c, // '，'
        0x3001, // '、'
        0xff1f, // '？'
        0xff01, // '！'
        0xff1a, // '：'
        0xff1b, // '；'
        0x29, // ')'
        0x5d, // ']'
        0x7d, // '}'
        0x3e, // '>'
        0xff09, // '）'
        0x300d, // '」'
        0x300f, // '』'
        0x3011, // '】'
        0x3015, // '〕'
        0x3009, // '〉'
        0x300b, // '》'
        0x2998, // '⦘'
        0x27e9, // '⟩'
        0x2019, // '’'
        0x201d, // '”'
        0x7c, // '|'
        0xb7, // '·'
        0x2f, // '/'
        0xff5c, // '｜'
        0x2014, // '—'
        0xff5e, // '～'
        0
    };
    for(int i = 0; close_punctuations[i]; i++) if(ch == close_punctuations[i]) return true;
    return false;
}

static bool is_quotation(uint32_t ch)
{
    const uint32_t is_quotation[] = {
        0x22,   // '"'
        0x27,   // '''
        0x275B, // '❛'
        0x275C, // '❜'
        0x275D, // '❝'
        0x275E, // '❞'
        0x2E00, // '⸀'
        0x2E01, // '⸁'
        0x2E06, // '⸆'
        0x2E07, // '⸇'
        0x2E08, // '⸈'
        0x2E0B, // '⸋'
        0
    };
    for(int i = 0; is_quotation[i]; i++) if(ch == is_quotation[i]) return true;
    return false;
}

static lv_text_word_process_word_type_t text_word_type_get(uint32_t ch)
{
    if(is_latin(ch)) return LV_TEXT_WORD_PROCESS_LATIN;
    else if(is_cjk(ch)) return LV_TEXT_WORD_PROCESS_CJK;
    else if(ch == '-') return LV_TEXT_WORD_PROCESS_HYPHEN;
    else if(ch >= '0' && ch <= '9') return LV_TEXT_WORD_PROCESS_NUMBER;
    else if(is_open_punctuation(ch)) return LV_TEXT_WORD_PROCESS_OPEN_PUNCTUATION;
    else if(is_close_punctuation(ch)) return LV_TEXT_WORD_PROCESS_CLOSE_PUNCTUATION;
    else if(is_quotation(ch)) return LV_TEXT_WORD_PROCESS_QUOTATION;
    else if(ch == '\n' || ch == '\r') return LV_TEXT_WORD_PROCESS_NEWLINE;
    else if(ch == ' ') return LV_TEXT_WORD_PROCESS_SPACE;
    else if(ch == '\t') return LV_TEXT_WORD_PROCESS_TAB;
    else return LV_TEXT_WORD_PROCESS_UNKNOWN;
}

static uint32_t len_unicode(uint32_t ch)
{
    if(ch < 0x80) return 1;
    else if(ch < 0x800) return 2;
    else if(ch < 0x10000)return 3;
    else return 4;
}
