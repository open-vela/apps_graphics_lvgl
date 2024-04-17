/**
 * @file lv_gif_decoder.h
 *
 */

#ifndef LV_GIF_DECODER_H
#define LV_GIF_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"

#if LV_USE_GIF

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    void * (*decoder_open)(void * obj);
    void (*decoder_start)(void * ctx, const void * src);
    void (*decoder_pause)(void * ctx);
    void (*decoder_resume)(void * ctx);
    void (*decoder_restart)(void * ctx);
    void (*decoder_close)(void * ctx);
} lv_gif_decoder_t;

void * def_gif_decoder_open(void * obj);
void def_gif_decoder_start(void * ctx, const void * src);
void def_gif_decoder_pause(void * ctx);
void def_gif_decoder_resume(void * ctx);
void def_gif_decoder_restart(void * ctx);
void def_gif_decoder_close(void * ctx);

#endif /*LV_USE_GIF*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GIF_DECODER_H*/
