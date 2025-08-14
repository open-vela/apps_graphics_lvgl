/**
 * @file lv_remote_profiler.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL && LV_USE_PROFILER && LV_USE_PROFILER_BUILTIN

#include "../../misc/lv_profiler_builtin.h"
#include "../../libs/argparse/argparse.h"

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
    print_func("profiler - built-in profiler control.\n");
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static int profiler_enable_cmd_cb(struct argparse * self,
                                  const struct argparse_option * option)
{
    LV_UNUSED(self);
    int enable = *(int *)option->value;
    lv_profiler_builtin_set_enable(enable ? true : false);
    LV_LOG_USER(enable ? "profiler enabled" : "profiler disabled");
    return 0;
}

static int profiler_flush_cmd_cb(struct argparse * self, const struct argparse_option * option)
{
    LV_UNUSED(self);
    LV_UNUSED(option);
    lv_profiler_builtin_flush();
    return 0;
}

static int profiler_reset_cmd_cb(struct argparse * self, const struct argparse_option * option)
{
    LV_UNUSED(self);
    LV_UNUSED(option);
    lv_profiler_builtin_reset();
    return 0;
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    LV_UNUSED(ctx);
    int enable = 0;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_INTEGER(0, "enable", &enable, "enable or disable profiler", profiler_enable_cmd_cb, 0, 0),
        OPT_BOOLEAN(0, "flush", NULL, "flush profiler results", profiler_flush_cmd_cb, 0, 0),
        OPT_BOOLEAN(0, "reset", NULL, "reset and clear profiler results", profiler_reset_cmd_cb, 0, 0),
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

LV_REMOTE_CTRL_CLASS_EXPORT(profiler, 0)

#endif /*LV_USE_REMOTE_CTRL && LV_USE_PROFILER && LV_USE_PROFILER_BUILTIN*/
