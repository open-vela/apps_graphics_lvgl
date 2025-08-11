/**
 * @file lv_remote_templ.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

/*********************
 *      DEFINES
 *********************/

typedef struct {
    int dummy;
} templ_ctx_t;

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
    print_func("This is a template remote control application.\n");
}

static void constructor_cb(void * ctx)
{
    templ_ctx_t * templ_ctx = ctx;
    LV_UNUSED(templ_ctx);
}

static void destructor_cb(void * ctx)
{
    templ_ctx_t * templ_ctx = ctx;
    LV_UNUSED(templ_ctx);
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    templ_ctx_t * templ_ctx = ctx;
    LV_UNUSED(templ_ctx);

    for(int i = 0; i < argc; i++) {
        LV_LOG_USER("arg %d: %s", i, argv[i]);
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(templ, sizeof(templ_ctx_t))

#endif /*LV_USE_REMOTE_CTRL*/
