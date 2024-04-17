/**
 * @file lv_gifdec.h
 *
 */

#ifndef LV_GIFDEC_H
#define LV_GIFDEC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"

#if LV_USE_GIF
#if !LV_USE_CUSTOM_GIF

/*********************
 *      DEFINES
 *********************/

#ifndef LV_GIFDEC_OPEN
#define LV_GIFDEC_OPEN(obj) lv_gifdec_open(obj)
#endif

#ifndef LV_GIFDEC_START
#define LV_GIFDEC_START(ctx, src) lv_gifdec_start(ctx, src)
#endif

#ifndef LV_GIFDEC_PAUSE
#define LV_GIFDEC_PAUSE(ctx) lv_gifdec_pause(ctx)
#endif

#ifndef LV_GIFDEC_RESUME
#define LV_GIFDEC_RESUME(ctx) lv_gifdec_resume(ctx)
#endif

#ifndef LV_GIFDEC_RESTART
#define LV_GIFDEC_RESTART(ctx) lv_gifdec_restart(ctx)
#endif

#ifndef LV_GIFDEC_CLOSE
#define LV_GIFDEC_CLOSE(ctx) lv_gifdec_close(ctx)
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void * lv_gifdec_open(void * obj);
void lv_gifdec_start(void * ctx, const void * src);
void lv_gifdec_pause(void * ctx);
void lv_gifdec_resume(void * ctx);
void lv_gifdec_restart(void * ctx);
void lv_gifdec_close(void * ctx);

#endif /*!LV_USE_CUSTOM_GIF*/
#endif /*LV_USE_GIF*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GIFDEC_H*/
