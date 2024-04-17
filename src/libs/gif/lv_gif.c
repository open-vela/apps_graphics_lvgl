/**
 * @file lv_gif.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_GIF
#include "lv_gif.h"
#if LV_USE_CUSTOM_GIF
    #include LV_CUSTOM_GIF_INCLUDE
#else
    #include "lv_gifdec.h"
#endif

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

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_gif_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_gif_set_src(lv_obj_t * obj, const void * src)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_GIFDEC_START(gif_obj->dec_ctx, src);
}

void lv_gif_restart(lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_GIFDEC_RESTART(gif_obj->dec_ctx);
}

void lv_gif_pause(lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_GIFDEC_PAUSE(gif_obj->dec_ctx);
}

void lv_gif_resume(lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_GIFDEC_RESUME(gif_obj->dec_ctx);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_gif_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_UNUSED(class_p);
    gif_obj->dec_ctx = LV_GIFDEC_OPEN(obj);
}

static void lv_gif_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_gif_t * gif_obj = (lv_gif_t *)obj;
    LV_UNUSED(class_p);

    LV_GIFDEC_CLOSE(gif_obj->dec_ctx);
}

#endif /*LV_USE_GIF*/
