/**
 * @file lv_gif_decoder.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"
#include "lv_gif.h"
#if LV_USE_GIF

#include "gifdec.h"

/*********************
 *      DEFINES
 *********************/
/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    gd_GIF * gif;
    lv_image_dsc_t imgdsc;
    lv_timer_t * timer;
    void * obj;
    uint32_t last_call;
} lv_gif_def_decoder_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void def_gif_decoder_frame_cb(lv_timer_t * t);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * def_gif_decoder_open(void * obj)
{
    lv_gif_def_decoder_t * decoder_ctx = lv_malloc_zeroed(sizeof(lv_gif_def_decoder_t));

    LV_ASSERT_MALLOC(decoder_ctx);

    decoder_ctx->timer = lv_timer_create(def_gif_decoder_frame_cb, 10, decoder_ctx);
    lv_timer_pause(decoder_ctx->timer);
    decoder_ctx->obj = obj;

    return decoder_ctx;
}

void def_gif_decoder_start(void * ctx, const void * src)
{
    lv_gif_def_decoder_t * decoder_ctx = (lv_gif_def_decoder_t *)ctx;

    /*Close previous gif if any*/
    if(decoder_ctx->gif) {
        lv_image_cache_drop(lv_image_get_src(decoder_ctx->obj));

        gd_close_gif(decoder_ctx->gif);
        decoder_ctx->gif = NULL;
        decoder_ctx->imgdsc.data = NULL;
    }

    if(lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t * img_dsc = src;
        decoder_ctx->gif = gd_open_gif_data(img_dsc->data);
    }
    else if(lv_image_src_get_type(src) == LV_IMAGE_SRC_FILE) {
        decoder_ctx->gif = gd_open_gif_file(src);
    }
    if(decoder_ctx->gif == NULL) {
        LV_LOG_WARN("Couldn't load the source");
        return;
    }

    decoder_ctx->imgdsc.data = decoder_ctx->gif->canvas;
    decoder_ctx->imgdsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    decoder_ctx->imgdsc.header.h = decoder_ctx->gif->height;
    decoder_ctx->imgdsc.header.w = decoder_ctx->gif->width;
    decoder_ctx->last_call = lv_tick_get();

    lv_image_set_src(decoder_ctx->obj, &decoder_ctx->imgdsc);

    lv_timer_resume(decoder_ctx->timer);
    lv_timer_reset(decoder_ctx->timer);

    def_gif_decoder_frame_cb(decoder_ctx->timer);
}

void def_gif_decoder_pause(void * ctx)
{
    lv_gif_def_decoder_t * decoder_ctx = (lv_gif_def_decoder_t *)ctx;
    lv_timer_pause(decoder_ctx->timer);
}

void def_gif_decoder_resume(void * ctx)
{
    lv_gif_def_decoder_t * decoder_ctx = (lv_gif_def_decoder_t *)ctx;

    if(decoder_ctx->gif == NULL) {
        LV_LOG_WARN("Gif resource not loaded correctly");
        return;
    }

    lv_timer_resume(decoder_ctx->timer);
}

void def_gif_decoder_restart(void * ctx)
{
    lv_gif_def_decoder_t * decoder_ctx = (lv_gif_def_decoder_t *)ctx;

    if(decoder_ctx->gif == NULL) {
        LV_LOG_WARN("Gif resource not loaded correctly");
        return;
    }

    gd_rewind(decoder_ctx->gif);
    lv_timer_resume(decoder_ctx->timer);
    lv_timer_reset(decoder_ctx->timer);
}

void def_gif_decoder_close(void * ctx)
{
    lv_gif_def_decoder_t * decoder_ctx = (lv_gif_def_decoder_t *)ctx;

    lv_image_cache_drop(lv_image_get_src(decoder_ctx->obj));

    if(decoder_ctx->gif)
        gd_close_gif(decoder_ctx->gif);
    lv_timer_delete(decoder_ctx->timer);
    lv_free(decoder_ctx);
}

void def_gif_decoder_frame_cb(lv_timer_t * t)
{
    lv_gif_def_decoder_t * decoder_ctx = (lv_gif_def_decoder_t *)t->user_data;

    uint32_t elaps = lv_tick_elaps(decoder_ctx->last_call);
    if(elaps < decoder_ctx->gif->gce.delay * 10) return;

    decoder_ctx->last_call = lv_tick_get();

    int has_next = gd_get_frame(decoder_ctx->gif);
    if(has_next == 0) {
        /*It was the last repeat*/
        lv_result_t res = lv_obj_send_event(decoder_ctx->obj, LV_EVENT_READY, NULL);
        lv_timer_pause(t);
        if(res != LV_FS_RES_OK) return;
    }

    gd_render_frame(decoder_ctx->gif, (uint8_t *)decoder_ctx->imgdsc.data);

    lv_image_cache_drop(lv_image_get_src(decoder_ctx->obj));
    lv_obj_invalidate(decoder_ctx->obj);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_GIF*/
