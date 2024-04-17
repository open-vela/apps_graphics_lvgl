/**
 * @file lv_gifdec.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_GIF
#if !LV_USE_CUSTOM_GIF

#include "lv_gifdec.h"
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
} lv_gifdec_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_gifdec_frame_cb(lv_timer_t * t);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_gifdec_open(void * obj)
{
    lv_gifdec_t * dec_ctx = lv_malloc_zeroed(sizeof(lv_gifdec_t));

    LV_ASSERT_MALLOC(dec_ctx);

    dec_ctx->timer = lv_timer_create(lv_gifdec_frame_cb, 10, dec_ctx);
    lv_timer_pause(dec_ctx->timer);
    dec_ctx->obj = obj;

    return dec_ctx;
}

void lv_gifdec_start(void * ctx, const void * src)
{
    lv_gifdec_t * dec_ctx = (lv_gifdec_t *)ctx;

    /*Close previous gif if any*/
    if(dec_ctx->gif) {
        lv_image_cache_drop(lv_image_get_src(dec_ctx->obj));

        gd_close_gif(dec_ctx->gif);
        dec_ctx->gif = NULL;
        dec_ctx->imgdsc.data = NULL;
    }

    if(lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t * img_dsc = src;
        dec_ctx->gif = gd_open_gif_data(img_dsc->data);
    }
    else if(lv_image_src_get_type(src) == LV_IMAGE_SRC_FILE) {
        dec_ctx->gif = gd_open_gif_file(src);
    }
    if(dec_ctx->gif == NULL) {
        LV_LOG_WARN("Couldn't load the source");
        return;
    }

    dec_ctx->imgdsc.data = dec_ctx->gif->canvas;
    dec_ctx->imgdsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    dec_ctx->imgdsc.header.h = dec_ctx->gif->height;
    dec_ctx->imgdsc.header.w = dec_ctx->gif->width;
    dec_ctx->last_call = lv_tick_get();

    lv_image_set_src(dec_ctx->obj, &dec_ctx->imgdsc);

    lv_timer_resume(dec_ctx->timer);
    lv_timer_reset(dec_ctx->timer);

    lv_gifdec_frame_cb(dec_ctx->timer);
}

void lv_gifdec_pause(void * ctx)
{
    lv_gifdec_t * dec_ctx = (lv_gifdec_t *)ctx;
    lv_timer_pause(dec_ctx->timer);
}

void lv_gifdec_resume(void * ctx)
{
    lv_gifdec_t * dec_ctx = (lv_gifdec_t *)ctx;

    if(dec_ctx->gif == NULL) {
        LV_LOG_WARN("Gif resource not loaded correctly");
        return;
    }

    lv_timer_resume(dec_ctx->timer);
}

void lv_gifdec_restart(void * ctx)
{
    lv_gifdec_t * dec_ctx = (lv_gifdec_t *)ctx;

    if(dec_ctx->gif == NULL) {
        LV_LOG_WARN("Gif resource not loaded correctly");
        return;
    }

    gd_rewind(dec_ctx->gif);
    lv_timer_resume(dec_ctx->timer);
    lv_timer_reset(dec_ctx->timer);
}

void lv_gifdec_close(void * ctx)
{
    lv_gifdec_t * dec_ctx = (lv_gifdec_t *)ctx;

    lv_image_cache_drop(lv_image_get_src(dec_ctx->obj));

    if(dec_ctx->gif)
        gd_close_gif(dec_ctx->gif);
    lv_timer_delete(dec_ctx->timer);
    lv_free(dec_ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_gifdec_frame_cb(lv_timer_t * t)
{
    lv_gifdec_t * dec_ctx = (lv_gifdec_t *)t->user_data;

    uint32_t elaps = lv_tick_elaps(dec_ctx->last_call);
    if(elaps < dec_ctx->gif->gce.delay * 10) return;

    dec_ctx->last_call = lv_tick_get();

    int has_next = gd_get_frame(dec_ctx->gif);
    if(has_next == 0) {
        /*It was the last repeat*/
        lv_result_t res = lv_obj_send_event(dec_ctx->obj, LV_EVENT_READY, NULL);
        lv_timer_pause(t);
        if(res != LV_FS_RES_OK) return;
    }

    gd_render_frame(dec_ctx->gif, (uint8_t *)dec_ctx->imgdsc.data);

    lv_image_cache_drop(lv_image_get_src(dec_ctx->obj));
    lv_obj_invalidate(dec_ctx->obj);
}

#endif /*#!LV_USE_CUSTOM_GIF*/
#endif /*LV_USE_GIF*/
