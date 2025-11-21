/**
 * @file lv_remote_hello.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../libs/argparse/argparse.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    int value;
} hello_ctx_t;

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
    print_func("Hello world example.\n");
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
    hello_ctx_t * hello_ctx = ctx;

    int value = -1;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_INTEGER('v', "value", &value, "set the value", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(value >= 0) {
        hello_ctx->value = value;
        LV_LOG_USER("value set to %d", value);
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(hello, sizeof(hello_ctx_t))

#endif /*LV_USE_REMOTE_CTRL*/
