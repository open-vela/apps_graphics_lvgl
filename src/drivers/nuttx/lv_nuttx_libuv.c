/**
 * @file lv_nuttx_libuv.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_nuttx_libuv.h"
#include <stdlib.h>

#include "../../../lvgl.h"
#include "../../lvgl_private.h"

#if LV_USE_NUTTX

#if LV_USE_NUTTX_LIBUV
#include <uv.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    int fd;
    bool polling;
    uv_poll_t fb_poll;
} lv_nuttx_uv_fb_ctx_t;

typedef struct {
    int fd;
    uv_poll_t vsync_poll;
    int32_t vsync_req_count;
    lv_timer_t * vsync_timer;
} lv_nuttx_uv_vsync_ctx_t;

typedef struct {
    int fd;
    uv_poll_t input_poll;
    lv_indev_t * indev;
} lv_nuttx_uv_input_ctx_t;

typedef struct {
#if LV_USE_REMOTE_CTRL
    uv_pipe_t server_pipe;
    lv_remote_ctrl_ctx_t * remote_ctrl_ctx;
#endif
} lv_nuttx_uv_control_ctx_t;

typedef struct {
    uv_timer_t uv_timer;
    lv_nuttx_uv_fb_ctx_t fb_ctx;
    lv_nuttx_uv_input_ctx_t input_ctx;
    lv_nuttx_uv_input_ctx_t uinput_ctx;
    lv_nuttx_uv_input_ctx_t mouse_ctx;
    lv_nuttx_uv_control_ctx_t control_ctx;
    lv_nuttx_uv_vsync_ctx_t vsync_ctx;
    lv_nuttx_uv_init_t inited;
    int32_t ref_count;
} lv_nuttx_uv_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int  lv_nuttx_uv_timer_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
static void lv_nuttx_uv_timer_deinit(lv_nuttx_uv_ctx_t * uv_ctx);

static int  _lv_nuttx_uv_vsync_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
static void _lv_nuttx_uv_vsync_deinit(lv_nuttx_uv_ctx_t * uv_ctx);

static int  lv_nuttx_uv_fb_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
static void lv_nuttx_uv_fb_deinit(lv_nuttx_uv_ctx_t * uv_ctx);

static int lv_nuttx_uv_input_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
static void lv_nuttx_uv_input_deinit(lv_nuttx_uv_ctx_t * uv_ctx);

static int lv_nuttx_uv_uinput_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
static void lv_nuttx_uv_uinput_deinit(lv_nuttx_uv_ctx_t * uv_ctx);

static int lv_nuttx_uv_control_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
static void lv_nuttx_uv_control_deinit(lv_nuttx_uv_ctx_t * uv_ctx);

#ifdef CONFIG_LV_USE_NUTTX_MOUSE
    static void lv_nuttx_uv_mouse_poll_cb(uv_poll_t * handle, int status, int events);
    static int lv_nuttx_uv_mouse_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx);
    static void lv_nuttx_uv_mouse_deinit(lv_nuttx_uv_ctx_t * uv_ctx);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_nuttx_uv_init(lv_nuttx_uv_t * uv_info)
{
    return lv_nuttx_uv_init_partial(uv_info, LV_NUTTX_UV_INIT_ALL);
}

void * lv_nuttx_uv_init_partial(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_init_t partial)
{
    lv_nuttx_uv_ctx_t * uv_ctx;
    int ret;

    uv_ctx = lv_malloc_zeroed(sizeof(lv_nuttx_uv_ctx_t));
    LV_ASSERT_MALLOC(uv_ctx);
    if(uv_ctx == NULL) return NULL;

    if((partial & LV_NUTTX_UV_INIT_TIMER) && (ret = lv_nuttx_uv_timer_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_timer_init fail : %d", ret);
        goto err_out;
    }

    if((partial & LV_NUTTX_UV_INIT_FB) && (ret = lv_nuttx_uv_fb_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_fb_init fail : %d", ret);
        goto err_out;
    }

    if((partial & LV_NUTTX_UV_INIT_INPUT) && (ret = lv_nuttx_uv_input_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_input_init fail : %d", ret);
        goto err_out;
    }

    if((partial & LV_NUTTX_UV_INIT_UINPUT) && (ret = lv_nuttx_uv_uinput_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_uinput_init fail : %d", ret);
        goto err_out;
    }

    if((partial & LV_NUTTX_UV_INIT_CONTROL) && (ret = lv_nuttx_uv_control_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_control_init fail : %d", ret);
        goto err_out;
    }

    if((partial & LV_NUTTX_UV_INIT_VSYNC) && (ret = _lv_nuttx_uv_vsync_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_vsync_init fail : %d", ret);
        goto err_out;
    }

#ifdef CONFIG_LV_USE_NUTTX_MOUSE
    if((partial & LV_NUTTX_UV_INIT_MOUSE) && (ret = lv_nuttx_uv_mouse_init(uv_info, uv_ctx)) < 0) {
        LV_LOG_ERROR("lv_nuttx_uv_mouse_init fail : %d", ret);
        goto err_out;
    }
#endif
    uv_ctx->inited = partial;

    return uv_ctx;

err_out:
    lv_free(uv_ctx);
    return NULL;
}

void lv_nuttx_uv_deinit(void ** data)
{
    lv_nuttx_uv_ctx_t * uv_ctx = *data;

    if(uv_ctx == NULL) return;
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_VSYNC) {
        _lv_nuttx_uv_vsync_deinit(uv_ctx);
    }
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_CONTROL) {
        lv_nuttx_uv_control_deinit(uv_ctx);
    }
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_INPUT) {
        lv_nuttx_uv_input_deinit(uv_ctx);
    }
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_UINPUT) {
        lv_nuttx_uv_uinput_deinit(uv_ctx);
    }
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_FB) {
        lv_nuttx_uv_fb_deinit(uv_ctx);
    }
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_TIMER) {
        lv_nuttx_uv_timer_deinit(uv_ctx);
    }
#ifdef CONFIG_LV_USE_NUTTX_MOUSE
    if(uv_ctx->inited & LV_NUTTX_UV_INIT_MOUSE) {
        lv_nuttx_uv_mouse_deinit(uv_ctx);
    }
#endif
    *data = NULL;
    LV_LOG_USER("Done");
}

void * lv_nuttx_uv_vsync_init(lv_nuttx_uv_vsync_t * vsync_info)
{
    lv_nuttx_uv_t uv_info;
    uv_info.loop = vsync_info->loop;
    uv_info.disp = vsync_info->disp;
    return lv_nuttx_uv_init_partial(&uv_info, LV_NUTTX_UV_INIT_VSYNC);
}

void lv_nuttx_uv_vsync_deinit(void ** data)
{
    lv_nuttx_uv_deinit(data);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_nuttx_uv_timer_cb(uv_timer_t * handle)
{
    uint32_t sleep_ms;

    sleep_ms = lv_timer_handler();

    if(sleep_ms == LV_NO_TIMER_READY) {
        uv_timer_stop(handle);
        return;
    }

    /* Prevent busy loops. */

    if(sleep_ms == 0) {
        sleep_ms = 1;
    }

    LV_LOG_TRACE("sleep_ms = %" LV_PRIu32, sleep_ms);
    uv_timer_start(handle, lv_nuttx_uv_timer_cb, sleep_ms, 0);
}

static void lv_nuttx_uv_timer_resume(void * data)
{
    uv_timer_t * timer = (uv_timer_t *)data;
    if(timer)
        uv_timer_start(timer, lv_nuttx_uv_timer_cb, 0, 0);
}

static int lv_nuttx_uv_timer_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
    uv_loop_t * loop = uv_info->loop;

    LV_ASSERT_NULL(uv_ctx);
    LV_ASSERT_NULL(loop);

    uv_ctx->uv_timer.data = uv_ctx;
    uv_timer_init(loop, &uv_ctx->uv_timer);
    uv_ctx->ref_count++;
    uv_timer_start(&uv_ctx->uv_timer, lv_nuttx_uv_timer_cb, 1, 1);

    lv_timer_handler_set_resume_cb(lv_nuttx_uv_timer_resume, &uv_ctx->uv_timer);
    return 0;
}

static void lv_nuttx_uv_deinit_cb(uv_handle_t * handle)
{
    lv_nuttx_uv_ctx_t * uv_ctx = handle->data;
    if(--uv_ctx->ref_count <= 0) {
        LV_LOG_USER("Done");
        lv_free(uv_ctx);
    }
}

static void lv_nuttx_uv_timer_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
    lv_timer_handler_set_resume_cb(NULL, NULL);
    uv_close((uv_handle_t *)&uv_ctx->uv_timer, lv_nuttx_uv_deinit_cb);
    LV_LOG_USER("Done");
}

static void lv_nuttx_uv_do_vsync(void)
{
    lv_display_t * d;
    d = lv_display_get_next(NULL);
    while(d) {
        lv_display_send_vsync_event(d, NULL);
        d = lv_display_get_next(d);
    }
}

static void lv_nuttx_uv_vsync_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    lv_nuttx_uv_do_vsync();
}

static void lv_nuttx_uv_vsync_poll_cb(uv_poll_t * handle, int status, int events)
{
    lv_nuttx_uv_vsync_ctx_t * vsync_ctx = (lv_nuttx_uv_vsync_ctx_t *)(handle->data);

    LV_UNUSED(status);
    LV_UNUSED(events);

    lv_nuttx_uv_do_vsync();

    lv_timer_resume(vsync_ctx->vsync_timer);
}

static void lv_nuttx_uv_disp_poll_cb(uv_poll_t * handle, int status, int events)
{
    lv_nuttx_uv_fb_ctx_t * fb_ctx = &((lv_nuttx_uv_ctx_t *)(handle->data))->fb_ctx;

    LV_UNUSED(status);
    LV_UNUSED(events);
    uv_poll_stop(handle);
    _lv_display_refr_timer(NULL);
    fb_ctx->polling = false;
}

static void lv_nuttx_uv_disp_refr_req_cb(lv_event_t * e)
{
    lv_nuttx_uv_fb_ctx_t * fb_ctx = lv_event_get_user_data(e);

    if(fb_ctx->polling) {
        return;
    }
    fb_ctx->polling = true;
    uv_poll_start(&fb_ctx->fb_poll, UV_WRITABLE, lv_nuttx_uv_disp_poll_cb);
}

static void lv_nuttx_uv_disp_vsync_request_cb(lv_event_t * e)
{
    lv_nuttx_uv_vsync_ctx_t * vsync_ctx = lv_event_get_user_data(e);
    void * param = lv_event_get_param(e);

    if(param) {
        if(vsync_ctx->vsync_req_count < 0) return;

        if(vsync_ctx->vsync_req_count == 0) {
            LV_LOG_INFO("enabled");
            if(vsync_ctx->fd > 0) {
                uv_poll_start(&vsync_ctx->vsync_poll, UV_PRIORITIZED, lv_nuttx_uv_vsync_poll_cb);
            }
            lv_timer_resume(vsync_ctx->vsync_timer);
        }
        vsync_ctx->vsync_req_count++;
    }
    else {
        if(vsync_ctx->vsync_req_count == 0) return;

        vsync_ctx->vsync_req_count--;
        if(vsync_ctx->vsync_req_count == 0) {
            LV_LOG_INFO("disabled");
            if(vsync_ctx->fd > 0) {
                uv_poll_stop(&vsync_ctx->vsync_poll);
            }
            lv_timer_pause(vsync_ctx->vsync_timer);
        }
    }
}

static int _lv_nuttx_uv_vsync_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
    LV_ASSERT_NULL(uv_info);
    LV_ASSERT_NULL(uv_ctx);

    lv_nuttx_uv_vsync_ctx_t * vsync_ctx = &uv_ctx->vsync_ctx;
    vsync_ctx->vsync_timer = lv_timer_create(lv_nuttx_uv_vsync_timer_cb,
                                             LV_NUTTX_VSYNC_TIMER_PERIOD, NULL);
    if(vsync_ctx->vsync_timer == NULL) {
        LV_LOG_ERROR("lv_nuttx_uv_vsync_init fail");
        return -ENOMEM;
    }
    lv_timer_pause(vsync_ctx->vsync_timer);

    uv_loop_t * loop = uv_info->loop;

    if(loop != NULL) {
        vsync_ctx->fd = *(int *)(lv_intptr_t)lv_display_get_driver_data(uv_info->disp);
        vsync_ctx->vsync_poll.data = vsync_ctx;
        uv_poll_init(loop, &vsync_ctx->vsync_poll, vsync_ctx->fd);
        uv_ctx->ref_count++;
    }

    LV_LOG_USER("lvgl vsync start OK");
    lv_display_add_event_cb(uv_info->disp, lv_nuttx_uv_disp_vsync_request_cb, LV_EVENT_VSYNC_REQUEST, vsync_ctx);
    return 0;
}

static void _lv_nuttx_uv_vsync_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
    lv_nuttx_uv_vsync_ctx_t * vsync_ctx = &uv_ctx->vsync_ctx;
    if(vsync_ctx->vsync_timer) {
        lv_timer_del(vsync_ctx->vsync_timer);
        vsync_ctx->vsync_timer = NULL;
    }
    if(vsync_ctx->fd > 0) {
        uv_close((uv_handle_t *)&vsync_ctx->vsync_poll, lv_nuttx_uv_deinit_cb);
    }
}

static int lv_nuttx_uv_fb_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
    uv_loop_t * loop = uv_info->loop;
    lv_display_t * disp = uv_info->disp;

    LV_ASSERT_NULL(uv_ctx);
    LV_ASSERT_NULL(disp);
    LV_ASSERT_NULL(loop);

    lv_nuttx_uv_fb_ctx_t * fb_ctx = &uv_ctx->fb_ctx;
    fb_ctx->fd = *(int *)lv_display_get_driver_data(disp);

    if(fb_ctx->fd <= 0) {
        LV_LOG_USER("skip uv fb init.");
        return 0;
    }

    if(!lv_display_get_refr_timer(disp)) {
        LV_LOG_ERROR("disp refr_timer is NULL");
        return -EINVAL;
    }

    /* Remove default refr timer. */
    lv_display_delete_refr_timer(disp);

    fb_ctx->fb_poll.data = uv_ctx;
    uv_poll_init(loop, &fb_ctx->fb_poll, fb_ctx->fd);
    uv_ctx->ref_count++;
    uv_poll_start(&fb_ctx->fb_poll, UV_WRITABLE, lv_nuttx_uv_disp_poll_cb);

    LV_LOG_USER("lvgl fb loop start OK");

    /* Register for the invalidate area event */

    lv_display_add_event_cb(disp, lv_nuttx_uv_disp_refr_req_cb, LV_EVENT_REFR_REQUEST, fb_ctx);

    return 0;
}

static void lv_nuttx_uv_fb_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
    /* should remove event */
    lv_nuttx_uv_fb_ctx_t * fb_ctx = &uv_ctx->fb_ctx;
    if(fb_ctx->fd > 0) {
        uv_close((uv_handle_t *)&fb_ctx->fb_poll, lv_nuttx_uv_deinit_cb);
    }
    LV_LOG_USER("Done");
}

static void lv_nuttx_uv_input_poll_cb(uv_poll_t * handle, int status, int events)
{
    lv_indev_t * indev = ((lv_nuttx_uv_ctx_t *)(handle->data))->input_ctx.indev;

    if(status < 0) {
        LV_LOG_WARN("input poll error: %s ", uv_strerror(status));
        return;
    }

    if(events & UV_READABLE) {
        lv_indev_read(indev);
    }
}

static int lv_nuttx_uv_input_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
    uv_loop_t * loop = uv_info->loop;
    lv_indev_t * indev = uv_info->indev;

    if(indev == NULL) {
        LV_LOG_USER("skip uv input init.");
        return 0;
    }

    LV_ASSERT_NULL(uv_ctx);
    LV_ASSERT_NULL(loop);

    if(lv_indev_get_mode(indev) == LV_INDEV_MODE_EVENT) {
        LV_LOG_ERROR("input device has been running in event-driven mode");
        return -EINVAL;
    }

    lv_nuttx_uv_input_ctx_t * input_ctx = &uv_ctx->input_ctx;
    input_ctx->fd = *(int *)lv_indev_get_driver_data(indev);
    if(input_ctx->fd <= 0) {
        LV_LOG_ERROR("can't get valid input fd");
        return 0;
    }

    input_ctx->indev = indev;
    lv_indev_set_mode(indev, LV_INDEV_MODE_EVENT);

    input_ctx->input_poll.data = uv_ctx;
    uv_poll_init(loop, &input_ctx->input_poll, input_ctx->fd);
    uv_ctx->ref_count++;
    uv_poll_start(&input_ctx->input_poll, UV_READABLE, lv_nuttx_uv_input_poll_cb);

    LV_LOG_USER("lvgl input loop start OK");

    return 0;
}

static void lv_nuttx_uv_input_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
    lv_nuttx_uv_input_ctx_t * input_ctx = &uv_ctx->input_ctx;
    if(input_ctx->fd > 0) {
        uv_close((uv_handle_t *)&input_ctx->input_poll, lv_nuttx_uv_deinit_cb);
    }
    LV_LOG_USER("Done");
}

static void lv_nuttx_uv_uinput_poll_cb(uv_poll_t * handle, int status, int events)
{
    lv_indev_t * indev = ((lv_nuttx_uv_ctx_t *)(handle->data))->uinput_ctx.indev;

    if(status < 0) {
        LV_LOG_WARN("uinput poll error: %s ", uv_strerror(status));
        return;
    }

    if(events & UV_READABLE) {
        lv_indev_read(indev);
    }
}

static int lv_nuttx_uv_uinput_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
    uv_loop_t * loop = uv_info->loop;
    lv_indev_t * uindev = uv_info->uindev;

    if(uindev == NULL) {
        LV_LOG_USER("skip uv uinput init.");
        return 0;
    }

    LV_ASSERT_NULL(uv_ctx);
    LV_ASSERT_NULL(loop);

    if(lv_indev_get_mode(uindev) == LV_INDEV_MODE_EVENT) {
        LV_LOG_ERROR("uinput device has been running in event-driven mode");
        return -EINVAL;
    }

    lv_nuttx_uv_input_ctx_t * uinput_ctx = &uv_ctx->uinput_ctx;
    uinput_ctx->fd = *(int *)lv_indev_get_driver_data(uindev);
    if(uinput_ctx->fd <= 0) {
        LV_LOG_ERROR("can't get valid uinput fd");
        return 0;
    }

    uinput_ctx->indev = uindev;
    lv_indev_set_mode(uindev, LV_INDEV_MODE_EVENT);

    uinput_ctx->input_poll.data = uv_ctx;
    uv_poll_init(loop, &uinput_ctx->input_poll, uinput_ctx->fd);
    uv_ctx->ref_count++;
    uv_poll_start(&uinput_ctx->input_poll, UV_READABLE, lv_nuttx_uv_uinput_poll_cb);

    LV_LOG_USER("lvgl uinput loop start OK");

    return 0;
}

static void lv_nuttx_uv_uinput_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
    lv_nuttx_uv_input_ctx_t * uinput_ctx = &uv_ctx->uinput_ctx;
    if(uinput_ctx->fd > 0) {
        uv_close((uv_handle_t *)&uinput_ctx->input_poll, lv_nuttx_uv_deinit_cb);
    }
    LV_LOG_USER("Done");
}

#ifdef CONFIG_LV_USE_NUTTX_MOUSE

static void lv_nuttx_uv_mouse_poll_cb(uv_poll_t * handle, int status, int events)
{
    lv_indev_t * indev = ((lv_nuttx_uv_ctx_t *)(handle->data))->mouse_ctx.indev;

    if(status < 0) {
        LV_LOG_WARN("mouse poll error: %s ", uv_strerror(status));
        return;
    }

    if(events & UV_READABLE) {
        lv_indev_read(indev);
    }
}

static int lv_nuttx_uv_mouse_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
    uv_loop_t * loop = uv_info->loop;
    lv_indev_t * mouse = uv_info->mouse_indev;

    if(mouse == NULL) {
        LV_LOG_USER("skip uv mouse init.");
        return 0;
    }

    LV_ASSERT_NULL(uv_ctx);
    LV_ASSERT_NULL(loop);

    if(lv_indev_get_mode(mouse) == LV_INDEV_MODE_EVENT) {
        LV_LOG_ERROR("mouse device has been running in event-driven mode");
        return -EINVAL;
    }

    lv_nuttx_uv_input_ctx_t * mouse_ctx = &uv_ctx->mouse_ctx;
    mouse_ctx->fd = *(int *)lv_indev_get_driver_data(mouse);
    if(mouse_ctx->fd <= 0) {
        LV_LOG_ERROR("can't get valid mouse fd");
        return 0;
    }

    mouse_ctx->indev = mouse;
    lv_indev_set_mode(mouse, LV_INDEV_MODE_EVENT);

    mouse_ctx->input_poll.data = uv_ctx;
    uv_poll_init(loop, &mouse_ctx->input_poll, mouse_ctx->fd);
    uv_ctx->ref_count++;
    uv_poll_start(&mouse_ctx->input_poll, UV_READABLE, lv_nuttx_uv_mouse_poll_cb);

    LV_LOG_USER("lvgl mouse loop start OK");

    return 0;
}

static void lv_nuttx_uv_mouse_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
    lv_nuttx_uv_input_ctx_t * mouse_ctx = &uv_ctx->mouse_ctx;
    if(mouse_ctx->fd > 0) {
        uv_close((uv_handle_t *)&mouse_ctx->input_poll, lv_nuttx_uv_deinit_cb);
    }
    LV_LOG_USER("Done");
}

#endif /*LV_USE_NUTTX_MOUSE*/

#if LV_USE_REMOTE_CTRL
static void lv_nuttx_uv_control_client_deinit_cb(uv_handle_t * client)
{
    lv_free(client);
}

static void lv_nuttx_uv_control_client_alloc_cb(uv_handle_t * client, size_t size, uv_buf_t * buf)
{
    buf->base = lv_malloc(size);
    buf->len = size;
}

static void lv_nuttx_uv_control_client_read_cb(uv_stream_t * client, ssize_t nread, const uv_buf_t * buf)
{
    if(nread == sizeof(lv_remote_ctrl_args_t)) {
        lv_nuttx_uv_ctx_t * uv_ctx = uv_handle_get_data((uv_handle_t *)client);
        const lv_remote_ctrl_args_t * args = (const lv_remote_ctrl_args_t *)buf->base;
        lv_remote_ctrl_execute(uv_ctx->control_ctx.remote_ctrl_ctx, args);
    }
    else if(nread < 0) {
        if(nread != UV_EOF) {
            LV_LOG_ERROR("uv_read failed: %s", uv_strerror(nread));
        }
        uv_close((uv_handle_t *)client, lv_nuttx_uv_control_client_deinit_cb);
    }
    else {
        LV_LOG_WARN("unexpected data received: %zd", nread);
    }

    lv_free(buf->base);
}

static void lv_nuttx_uv_control_server_accept_cb(uv_stream_t * server, int status)
{
    lv_nuttx_uv_ctx_t * uv_ctx = uv_handle_get_data((uv_handle_t *)server);
    int ret;

    if(status < 0) {
        LV_LOG_ERROR("uv_accept failed: %s", uv_strerror(status));
        return;
    }

    uv_pipe_t * client = lv_malloc(sizeof(uv_pipe_t));
    LV_ASSERT_MALLOC(client);

    if((ret = uv_pipe_init(server->loop, client, 0)) < 0) {
        LV_LOG_ERROR("uv_pipe_init failed: %s", uv_strerror(ret));
        return;
    }

    if((ret = uv_accept(server, (uv_stream_t *)client)) < 0) {
        LV_LOG_ERROR("uv_accept failed: %s", uv_strerror(ret));
        return;
    }

    uv_handle_set_data((uv_handle_t *)client, uv_ctx);
    uv_read_start((uv_stream_t *)client, lv_nuttx_uv_control_client_alloc_cb, lv_nuttx_uv_control_client_read_cb);
}
#endif

static int lv_nuttx_uv_control_init(lv_nuttx_uv_t * uv_info, lv_nuttx_uv_ctx_t * uv_ctx)
{
#if LV_USE_REMOTE_CTRL
    uv_loop_t * loop = uv_info->loop;
    uv_pipe_t * server = &uv_ctx->control_ctx.server_pipe;
    int ret;

    uv_ctx->control_ctx.remote_ctrl_ctx = lv_remote_ctrl_create();

    if((ret = uv_pipe_init(loop, server, 0)) < 0) {
        LV_LOG_ERROR("uv_pipe_init failed: %s", uv_strerror(ret));
        return 0;
    }

    uv_handle_set_data((uv_handle_t *)server, uv_ctx);
    uv_ctx->ref_count++;

    if((ret = uv_pipe_bind(server, LV_NUTTX_CONTROL_PIPE_NAME)) < 0) {
        LV_LOG_ERROR("uv_pipe_bind failed: %s", uv_strerror(ret));
        return 0;
    }

    if((ret = uv_listen((uv_stream_t *)server, 8, lv_nuttx_uv_control_server_accept_cb)) < 0) {
        LV_LOG_ERROR("uv_listen failed: %s", uv_strerror(ret));
        return 0;
    }

    LV_LOG_USER("lvgl control server start OK");
#endif

    return 0;
}

static void lv_nuttx_uv_control_deinit(lv_nuttx_uv_ctx_t * uv_ctx)
{
#if LV_USE_REMOTE_CTRL
    lv_remote_ctrl_destroy(uv_ctx->control_ctx.remote_ctrl_ctx);

    uv_close((uv_handle_t *)&uv_ctx->control_ctx.server_pipe, lv_nuttx_uv_deinit_cb);

    LV_LOG_USER("Done");
#endif
}

#endif /*LV_USE_NUTTX_LIBUV*/

#endif /*LV_USE_NUTTX*/
