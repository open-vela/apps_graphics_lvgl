/**
 * @file lv_nuttx_libuv.h
 *
 */

#ifndef LV_NUTTX_LIBUV_H
#define LV_NUTTX_LIBUV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../display/lv_display.h"
#include "../../indev/lv_indev.h"

#if LV_USE_NUTTX

#if LV_USE_NUTTX_LIBUV

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_NUTTX_UV_INIT_NONE = 0,
    LV_NUTTX_UV_INIT_TIMER = 1 << 0,
    LV_NUTTX_UV_INIT_FB = 1 << 1,
    LV_NUTTX_UV_INIT_INPUT = 1 << 2,
    LV_NUTTX_UV_INIT_UINPUT = 1 << 3,
    LV_NUTTX_UV_INIT_MOUSE = 1 << 4,
    LV_NUTTX_UV_INIT_CONTROL = 1 << 5,
    LV_NUTTX_UV_INIT_VSYNC = 1 << 6,
    LV_NUTTX_UV_INIT_ALL = LV_NUTTX_UV_INIT_TIMER | LV_NUTTX_UV_INIT_FB | LV_NUTTX_UV_INIT_INPUT |
                           LV_NUTTX_UV_INIT_UINPUT | LV_NUTTX_UV_INIT_MOUSE | LV_NUTTX_UV_INIT_CONTROL |
                           LV_NUTTX_UV_INIT_VSYNC,
} lv_nuttx_uv_init_t;

typedef struct {
    void * loop;
    lv_display_t * disp;
    lv_indev_t * indev;
    lv_indev_t * uindev;
    lv_indev_t * mouse_indev;
} lv_nuttx_uv_t;

typedef struct {
    void * loop;
    lv_display_t * disp;
} lv_nuttx_uv_vsync_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the uv_loop using the provided configuration information.
 * @param uv_info Pointer to the lv_nuttx_uv_t structure to be initialized.
 */
void * lv_nuttx_uv_init(lv_nuttx_uv_t * uv_info);

/**
 * Initialize the uv_loop using the provided configuration information.
 * @param uv_info Pointer to the lv_nuttx_uv_t structure to be initialized.
 * @param partial The partial mode to initialize the uv_loop.
 */
void * lv_nuttx_uv_init_partial(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_init_t partial);

/**
 * Deinitialize the uv_loop configuration for NuttX porting layer.
 * @param data Pointer to user data.
 */
void lv_nuttx_uv_deinit(void ** data);

/**
 * @deprecated Use lv_nuttx_uv_init_partial() and lv_nuttx_uv_deinit() instead.
 */
void * lv_nuttx_uv_vsync_init(lv_nuttx_uv_vsync_t * uv_vsync);

/**
 * @deprecated Use lv_nuttx_uv_init_partial() and lv_nuttx_uv_deinit() instead.
 */
void lv_nuttx_uv_vsync_deinit(void ** data);
/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_NUTTX_LIBUV*/

#endif /*LV_USE_NUTTX*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_NUTTX_LIBUV_H*/
