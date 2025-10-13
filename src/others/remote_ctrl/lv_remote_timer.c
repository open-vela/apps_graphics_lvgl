/**
 * @file lv_remote_timer.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../libs/argparse/argparse.h"
#include "../../misc/lv_timer.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_timer_t * ori_timer;
    lv_timer_cb_t ori_timer_cb;
    void * ori_user_data;
} timer_ctx_t;

/**********************
 *  STATIC PROTOTYPES
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

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void show_help_cb(lv_remote_ctrl_print_func_t print_func)
{
    print_func("timer - Timer start and stop and period control\n");
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void timer_hook_cb(lv_timer_t * timer)
{
    timer_ctx_t * ctx = lv_timer_get_user_data(timer);

    /* change user_data to original user_data */
    timer->user_data = ctx->ori_user_data;

    uint32_t start = lv_tick_get();
    ctx->ori_timer_cb(timer);
    LV_LOG_USER("timer_cb: %p, cost: %" LV_PRIu32 " ms", (void *)(lv_uintptr_t)ctx->ori_timer_cb, lv_tick_elaps(start));

    /* restore user_data */
    timer->user_data = ctx;
}

static void dump_timer_info(lv_timer_t * timer)
{
    LV_LOG_USER("timer: %p", (void *)timer);
    LV_LOG_USER("  period: %" LV_PRIu32 " ms", timer->period);
    LV_LOG_USER("  last_run: %" LV_PRIu32 " ms", timer->last_run);
    LV_LOG_USER("  timer_cb: %p", (void *)(lv_uintptr_t)timer->timer_cb);
    LV_LOG_USER("  user_data: %p", timer->user_data);
    LV_LOG_USER("  repeat_count: %" LV_PRIu32, timer->repeat_count);
    LV_LOG_USER("  pause: %d", timer->paused);
    LV_LOG_USER("  auto delete: %d", timer->auto_delete);
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    timer_ctx_t * timer_ctx = ctx;

    int delete_timer = 0;
    int period = -1;
    int pause = 0;
    int resume = 0;
    int ready = 0;
    int reset = 0;
    int enable = 0;
    int hook = -1;
    int dump_timer = 0;
    lv_timer_t * timer = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_HEX('t', "timer", &timer, "set timer by address", NULL, 0, 0),
        OPT_BOOLEAN(0, "dump", &dump_timer, "dump indev information", NULL, 0, 0),
        OPT_INTEGER(0, "period", &period, "set timer period in milliseconds", NULL, 0, 0),
        OPT_BOOLEAN(0, "pause", &pause, "pause or unpause timer", NULL, 0, 0),
        OPT_BOOLEAN(0, "resume", &resume, "resume timer", NULL, 0, 0),
        OPT_BOOLEAN(0, "ready", &ready, "ready timer", NULL, 0, 0),
        OPT_BOOLEAN(0, "reset", &reset, "reset timer", NULL, 0, 0),
        OPT_INTEGER(0, "enable", &enable, "enable or disable all timer. -1: disable all, 1: enable all", NULL, 0, 0),
        OPT_INTEGER(0, "hook", &hook, "hool timer callback", NULL, 0, 0),
        OPT_BOOLEAN(0, "del", &delete_timer, "delete timer", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(dump_timer) {
        if(timer) {
            dump_timer_info(timer);
        }
        else {
            lv_timer_t * t = lv_timer_get_next(NULL);
            while(t) {
                dump_timer_info(t);
                t = lv_timer_get_next(t);
            }
        }

        return LV_RESULT_OK;
    }

    if(enable) {
        lv_timer_enable(enable > 0 ? true : false);
        return LV_RESULT_OK;
    }

    if(hook == 0) {
        if(!timer_ctx->ori_timer) {
            LV_LOG_WARN("timer hook not set");
            return LV_RESULT_INVALID;
        }

        lv_timer_set_cb(timer_ctx->ori_timer, timer_ctx->ori_timer_cb);
        lv_timer_set_user_data(timer_ctx->ori_timer, timer_ctx->ori_user_data);
        timer_ctx->ori_timer = NULL;
        timer_ctx->ori_timer_cb = NULL;
        timer_ctx->ori_user_data = NULL;
        LV_LOG_USER("timer unhooked");
        return LV_RESULT_OK;
    }

    if(!timer) {
        LV_LOG_WARN("No timer specified");
        return LV_RESULT_INVALID;
    }

    if(period >= 0) {
        lv_timer_set_period(timer, period);
        LV_LOG_USER("timer period: %dms", period);
    }

    if(pause) {
        lv_timer_pause(timer);
        LV_LOG_USER("timer pause");
    }

    if(ready) {
        lv_timer_ready(timer);
        LV_LOG_USER("timer ready");
    }

    if(reset) {
        lv_timer_reset(timer);
        LV_LOG_USER("timer reset");
    }

    if(resume) {
        lv_timer_resume(timer);
        LV_LOG_USER("timer resumed");
    }

    if(hook > 0) {
        if(timer_ctx->ori_timer_cb) {
            LV_LOG_WARN("timer hook: %p already set", (void *)(lv_uintptr_t)timer_ctx->ori_timer_cb);
            return LV_RESULT_INVALID;
        }

        timer_ctx->ori_timer = timer;
        timer_ctx->ori_timer_cb = timer->timer_cb;
        timer_ctx->ori_user_data = timer->user_data;

        lv_timer_set_cb(timer, timer_hook_cb);
        lv_timer_set_user_data(timer, timer_ctx);
        LV_LOG_USER("timer hooked: %p, user_data: %p", (void *)(lv_uintptr_t)timer_ctx->ori_timer_cb, timer_ctx->ori_user_data);
        return LV_RESULT_OK;
    }

    if(delete_timer) {
        if(timer == timer_ctx->ori_timer) {
            LV_LOG_USER("clearing timer hooked: %p, user_data: %p", (void *)(lv_uintptr_t)timer_ctx->ori_timer_cb,
                        timer_ctx->ori_user_data);
            timer_ctx->ori_timer = NULL;
            timer_ctx->ori_timer_cb = NULL;
            timer_ctx->ori_user_data = NULL;
        }

        lv_timer_delete(timer);
        LV_LOG_USER("timer deleted");
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(timer, sizeof(timer_ctx_t))

#endif /*LV_USE_REMOTE_CTRL*/
