/**
 * @file lv_remote_refr.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../libs/argparse/argparse.h"
#include "../../core/lv_refr.h"

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

static void show_help_cb(lv_remote_ctrl_print_func_t print_func)
{
    print_func("refr - display refresh control\n");
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static int refr_debug_cmd_cb(struct argparse * self,
                             const struct argparse_option * option)
{
    LV_UNUSED(self);
    int debug_en = *(int *)option->value;
    lv_refr_enable_debug_mode(debug_en ? true : false);
    LV_LOG_USER("debug enable: %d", debug_en);
    return 0;
}

static int refr_now_cmd_cb(struct argparse * self,
                           const struct argparse_option * option)
{
    LV_UNUSED(self);
    LV_UNUSED(option);
    lv_refr_now(NULL);
    LV_LOG_USER("refresh now");
    return 0;
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    LV_UNUSED(ctx);
    int debug_en = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_INTEGER('d', "debug", &debug_en, "enable debug", refr_debug_cmd_cb, 0, 0),
        OPT_BOOLEAN(0, "now", NULL, "refresh now", refr_now_cmd_cb, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(refr, 0)

#endif /*LV_USE_REMOTE_CTRL*/
