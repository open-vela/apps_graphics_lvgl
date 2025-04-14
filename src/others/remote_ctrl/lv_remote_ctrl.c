/**
 * @file lv_remote_ctrl.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_remote_ctrl.h"

#if LV_USE_REMOTE_CTRL

#include "lv_remote_ctrl_private.h"
#include "../../stdlib/lv_string.h"
#include "../../misc/lv_assert.h"

/*********************
 *      DEFINES
 *********************/

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

/**********************
*  STATIC VARIABLES
**********************/

#define LV_REMOTE_CTRL_CLASS_DEF(NAME) extern const lv_remote_ctrl_class_t lv_remote_ctrl_##NAME##_class;
#include "lv_remote_class_defs.inc"
#undef LV_REMOTE_CTRL_CLASS_DEF

static const lv_remote_ctrl_class_t * class_table[] = {
#define LV_REMOTE_CTRL_CLASS_DEF(NAME) &lv_remote_ctrl_##NAME##_class,
#include "lv_remote_class_defs.inc"
#undef LV_REMOTE_CTRL_CLASS_DEF
};

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    void * ctx;
    const lv_remote_ctrl_class_t * cls;
} lv_remote_ctrl_sub_ctx_t;

struct _lv_remote_ctrl_ctx_t {
    lv_remote_ctrl_sub_ctx_t sub_ctx[ARRAY_SIZE(class_table)];
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_remote_ctrl_ctx_t * lv_remote_ctrl_create(void)
{
    lv_remote_ctrl_ctx_t * ctx = lv_malloc_zeroed(sizeof(lv_remote_ctrl_ctx_t));
    LV_ASSERT_MALLOC(ctx);

    for(size_t i = 0; i < ARRAY_SIZE(ctx->sub_ctx); i++) {
        lv_remote_ctrl_sub_ctx_t * sub_ctx = &ctx->sub_ctx[i];
        sub_ctx->cls = class_table[i];

        if(sub_ctx->cls->instance_size > 0) {
            sub_ctx->ctx = lv_malloc_zeroed(sub_ctx->cls->instance_size);
            LV_ASSERT_MALLOC(sub_ctx->ctx);
        }

        if(sub_ctx->cls->constructor_cb) {
            sub_ctx->cls->constructor_cb(sub_ctx->ctx);
        }
    }

    return ctx;
}

void lv_remote_ctrl_destroy(lv_remote_ctrl_ctx_t * ctx)
{
    LV_ASSERT_NULL(ctx);

    for(size_t i = 0; i < ARRAY_SIZE(ctx->sub_ctx); i++) {
        if(ctx->sub_ctx[i].cls->destructor_cb) {
            ctx->sub_ctx[i].cls->destructor_cb(ctx->sub_ctx[i].ctx);
        }

        if(ctx->sub_ctx[i].ctx) {
            lv_free(ctx->sub_ctx[i].ctx);
            ctx->sub_ctx[i].ctx = NULL;
        }
    }

    lv_free(ctx);
}

lv_result_t lv_remote_ctrl_execute(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_args_t * args)
{
    LV_ASSERT_NULL(ctx);
    LV_ASSERT_NULL(args);
    LV_ASSERT(args->argc <= LV_REMOTE_CTRL_ARGC_MAX);

    const char * cmd = &args->argv_buf[0];
    if(!cmd[0]) {
        LV_LOG_WARN("Command is empty");
        return LV_RESULT_INVALID;
    }

    for(size_t i = 0; i < ARRAY_SIZE(ctx->sub_ctx); i++) {
        lv_remote_ctrl_sub_ctx_t * sub_ctx = &ctx->sub_ctx[i];
        if(lv_strcmp(cmd, sub_ctx->cls->name) != 0) {
            continue;
        }

        const char * argv[LV_REMOTE_CTRL_ARGC_MAX];
        for(int cnt = 0; cnt < args->argc; cnt++) {
            /* Convert to argv format */
            LV_ASSERT(args->argv_offsets[cnt] < sizeof(args->argv_buf));
            argv[cnt] = &args->argv_buf[args->argv_offsets[cnt]];
        }

        if(!sub_ctx->cls->execute_cb) {
            LV_LOG_ERROR("Command '%s' no execute callback", cmd);
            return LV_RESULT_INVALID;
        }

        return sub_ctx->cls->execute_cb(sub_ctx->ctx, args->argc, argv);
    }

    LV_LOG_WARN("Command '%s' not found", cmd);
    return LV_RESULT_INVALID;
}

void lv_remote_ctrl_show_help(const char * cmd_name, lv_remote_ctrl_print_func_t print_func)
{
    LV_ASSERT_NULL(print_func);
    print_func("Usage: %s <command> <subcommand> [parameters]\n", cmd_name);
    print_func("Available commands:\n");

    for(size_t i = 0; i < ARRAY_SIZE(class_table); i++) {
        if(!class_table[i]->show_help_cb) {
            continue;
        }

        class_table[i]->show_help_cb(print_func);
    }
}

lv_result_t lv_remote_ctrl_args_init(lv_remote_ctrl_args_t * args, int argc, const char * argv[])
{
    LV_ASSERT_NULL(args);
    LV_ASSERT_NULL(argv);
    lv_memzero(args, sizeof(lv_remote_ctrl_args_t));

    if(argc < 1 || argc > LV_REMOTE_CTRL_ARGC_MAX) {
        LV_LOG_WARN("Invalid argument count: %d (must be between 1 and %d)",
                    argc, LV_REMOTE_CTRL_ARGC_MAX);
        return LV_RESULT_INVALID;
    }

    size_t offset = 0;
    const size_t max_size = sizeof(args->argv_buf);

    for(int i = 0; i < argc; i++) {
        if(argv[i] == NULL) {
            LV_LOG_WARN("Argument %d is NULL", i);
            return LV_RESULT_INVALID;
        }

        const size_t remain = max_size - offset;
        const size_t len = lv_strlen(argv[i]) + 1;

        if(len >= remain) {
            LV_LOG_WARN("Arguments(%s) too long: %zu (remain %zu bytes)", argv[i], len, remain);
            return LV_RESULT_INVALID;
        }

        char * dst = &args->argv_buf[offset];
        lv_strcpy(dst, argv[i]);
        args->argv_offsets[i] = offset;
        offset += len;
    }

    args->argc = argc;
    return LV_RESULT_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_REMOTE_CTRL*/
