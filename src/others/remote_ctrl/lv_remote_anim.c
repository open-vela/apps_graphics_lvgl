/**
 * @file lv_remote_anim.c
 */

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

/*********************
 *      DEFINES
 *********************/

#include "../../libs/argparse/argparse.h"
#include "../../misc/lv_anim.h"
#include "../../core/lv_global.h"

/**********************
 *      TYPEDEFS
 **********************/

#define state LV_GLOBAL_DEFAULT()->anim_state
#define anim_ll_p &(state.anim_ll)

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

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

static void dump_anim_info(lv_anim_t * anim)
{
    LV_LOG_USER("anim: %p", (void *)anim);
    LV_LOG_USER("  var: %p", anim->var);
    LV_LOG_USER("  exec_cb: %p", (void *)(lv_uintptr_t)anim->exec_cb);
    LV_LOG_USER("  start: %d, end: %d", (int)anim->start_value, (int)anim->end_value);
    LV_LOG_USER("  duration: %dms", (int)anim->duration);
    LV_LOG_USER("  repeat_cnt: %d", (int)anim->repeat_cnt);
}

static void show_help_cb(lv_remote_ctrl_print_func_t print_func)
{
    print_func("anim - control the animation module\n");
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    LV_UNUSED(ctx);
    lv_anim_t * anim = NULL;
    int period = -1;
    int refresh = 0;
    int dump_anim = 0;
    int del_anim = 0;
    int del_all_anim = 0;
    int duration = -1;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_HEX('a', "anim", &anim, "Animation object to control", NULL, 0, 0),
        OPT_INTEGER(0, "period", &period, "Set animation timer period in milliseconds", NULL, 0, 0),
        OPT_BOOLEAN(0, "refresh", &refresh, "Force refresh all animations", NULL, 0, 0),
        OPT_BOOLEAN(0, "dump", &dump_anim, "Dump animation information", NULL, 0, 0),
        OPT_BOOLEAN(0, "del", &del_anim, "Delete animation object", NULL, 0, 0),
        OPT_BOOLEAN(0, "del-all", &del_all_anim, "Delete all animation objects", NULL, 0, 0),
        OPT_INTEGER(0, "duration", &duration, "Set animation duration in milliseconds", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(period >= 0) {
        lv_timer_t * timer = lv_anim_get_timer();
        if(!timer) {
            LV_LOG_WARN("No animation timer found");
            return LV_RESULT_INVALID;
        }

        lv_timer_set_period(timer, period);
        LV_LOG_USER("Set animation timer period: %dms", period);
        return LV_RESULT_OK;
    }

    if(dump_anim) {
        if(anim) {
            dump_anim_info(anim);
        }
        else {
            lv_anim_t * a;
            _LV_LL_READ(anim_ll_p, a) {
                dump_anim_info(a);
            }
        }
        return LV_RESULT_OK;
    }

    if(refresh) {
        lv_anim_refr_now();
        LV_LOG_USER("Force refreshed all animations");
        return LV_RESULT_OK;
    }

    if(del_all_anim) {
        lv_anim_delete_all();
        LV_LOG_USER("Deleted all animation objects");
        return LV_RESULT_OK;
    }

    if(!anim) {
        LV_LOG_WARN("No animation object found");
        return LV_RESULT_INVALID;
    }

    if(duration >= 0) {
        lv_anim_set_duration(anim, duration);
        LV_LOG_USER("Set animation duration: %dms", duration);
    }

    if(del_anim) {
        bool ret = lv_anim_delete(anim->var, anim->exec_cb);
        LV_LOG_USER("Deleted animation object: %s", ret ? "success" : "failed");
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(anim, 0)

#endif /*LV_USE_REMOTE_CTRL*/
