/**
 * @file lv_gif.h
 *
 */

#ifndef LV_GIF_H
#define LV_GIF_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_GIF

#include "lv_gif_decoder.h"
#include "../../widgets/image/lv_image.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_image_t img;
    void * decoder_ctx;
} lv_gif_t;

LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_gif_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a gif object
 * @param parent    pointer to an object, it will be the parent of the new gif.
 * @return          pointer to the gif obj
 */
lv_obj_t * lv_gif_create(lv_obj_t * parent);

/**
 * Set the gif data to display on the object
 * @param obj       pointer to a gif object
 * @param src       1) pointer to an ::lv_image_dsc_t descriptor (which contains gif raw data) or
 *                  2) path to a gif file (e.g. "S:/dir/anim.gif")
 */
void lv_gif_set_src(lv_obj_t * obj, const void * src);

/**
 * Restart a gif animation.
 * @param obj pointer to a gif obj
 */
void lv_gif_restart(lv_obj_t * obj);

/**
 * Pause a gif animation.
 * @param obj pointer to a gif obj
 */
void lv_gif_pause(lv_obj_t * obj);

/**
 * Resume a gif animation.
 * @param obj pointer to a gif obj
 */
void lv_gif_resume(lv_obj_t * obj);

/**
 * Init Gif library
 */
void lv_gif_init(void);

/*
 * Register a GIF decoder
 * @param decoder pointer to a GIF decoder struct
 */
void lv_gif_decoder_register(lv_gif_decoder_t * decoder);

/**
 * Deinit Gif library
 */
void lv_gif_deinit(void);
/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_GIF*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GIF_H*/
