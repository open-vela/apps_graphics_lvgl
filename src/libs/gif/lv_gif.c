/**
 * @file lv_gifenc.c
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
#define MY_CLASS (&lv_gif_class)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_gif_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_gif_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_gif_class = {
    .constructor_cb = lv_gif_constructor,
    .destructor_cb = lv_gif_destructor,
    .instance_size = sizeof(lv_gif_t),
    .base_class = &lv_image_class,
    .name = "gif",
};

#define gif_decoder LV_GLOBAL_DEFAULT()->gif_decoder

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_gif_init(void)
{
    if(gif_decoder == NULL) {
        gif_decoder = lv_malloc_zeroed(sizeof(lv_gif_decoder_t));
    }

    gif_decoder->decoder_open = def_gif_decoder_open;
    gif_decoder->decoder_start = def_gif_decoder_start;
    gif_decoder->decoder_pause = def_gif_decoder_pause;
    gif_decoder->decoder_resume = def_gif_decoder_resume;
    gif_decoder->decoder_restart = def_gif_decoder_restart;
    gif_decoder->decoder_close = def_gif_decoder_close;
}

void lv_gif_decoder_register(lv_gif_decoder_t * decoder)
{
    if(decoder) {
        lv_memcpy(gif_decoder, decoder, sizeof(lv_gif_decoder_t));
    }
}

lv_obj_t * lv_gif_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_gif_set_src(lv_obj_t * obj, const void * src)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    gif_decoder->decoder_start(gif_obj->decoder_ctx, src);
}

void lv_gif_restart(lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    gif_decoder->decoder_restart(gif_obj->decoder_ctx);
}

void lv_gif_pause(lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    gif_decoder->decoder_pause(gif_obj->decoder_ctx);
}

void lv_gif_resume(lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    gif_decoder->decoder_resume(gif_obj->decoder_ctx);
}

void lv_gif_deinit(void)
{
    if(gif_decoder) {
        lv_free(gif_decoder);
        gif_decoder = NULL;
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_gif_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_UNUSED(class_p);
    gif_obj->decoder_ctx = gif_decoder->decoder_open(obj);
}

static void lv_gif_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_UNUSED(class_p);

    gif_decoder->decoder_close(gif_obj->decoder_ctx);
}

#endif /*LV_USE_GIF*/
