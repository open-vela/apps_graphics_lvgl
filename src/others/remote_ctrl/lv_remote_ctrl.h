/**
 * @file lv_remote_ctrl.h
 *
 */
#ifndef LV_REMOTE_CTRL_H
#define LV_REMOTE_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lv_conf_internal.h"

#if LV_USE_REMOTE_CTRL

#if !LV_USE_ARGPARSE
#error "lv_remote_ctrl requires lv_conf.h: LV_USE_ARGPARSE 1"
#endif

#include "../../misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

#define LV_REMOTE_CTRL_ARGC_MAX 16
#define LV_REMOTE_CTRL_ARGV_BUF_LEN 512

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_remote_ctrl_ctx_t;
typedef struct _lv_remote_ctrl_ctx_t lv_remote_ctrl_ctx_t;

typedef void (*lv_remote_ctrl_print_func_t)(const char * format, ...);

typedef struct {
    int argc;                                       /**< Number of command arguments */
    uint16_t argv_offsets[LV_REMOTE_CTRL_ARGC_MAX]; /**< Offsets from argv_buf start */
    char argv_buf[LV_REMOTE_CTRL_ARGV_BUF_LEN];     /**< Buffer to store command arguments */
} lv_remote_ctrl_args_t;

typedef lv_remote_ctrl_args_t lv_remote_ctrl_cmd_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a remote control context
 * @return Pointer to remote control context
 */
lv_remote_ctrl_ctx_t * lv_remote_ctrl_create(void);

/**
 * Destroy a remote control context
 * @param ctx Pointer to remote control context to destroy
 */
void lv_remote_ctrl_destroy(lv_remote_ctrl_ctx_t * ctx);

/**
 * Execute a remote control command
 * @param ctx Pointer to remote control context
 * @param args Pointer to command arguments
 * @return LV_RESULT_OK if the command is executed successfully, LV_RESULT_INVALID otherwise
 */
lv_result_t lv_remote_ctrl_execute(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_args_t * args);

/**
 * Show help for remote control commands
 * @param cmd_name Name of the command to show help for
 * @param print_func Pointer to print function
 */
void lv_remote_ctrl_show_help(const char * cmd_name, lv_remote_ctrl_print_func_t print_func);

/**
 * Initialize command arguments
 * @param args Pointer to command arguments to initialize
 * @param argc Number of command arguments
 * @param argv Array of command arguments
 * @return LV_RESULT_OK if the command arguments are initialized successfully, LV_RESULT_INVALID otherwise
 */
lv_result_t lv_remote_ctrl_args_init(lv_remote_ctrl_args_t * args, int argc, const char * argv[]);

static inline lv_result_t lv_remote_ctrl_cmd_parse(lv_remote_ctrl_args_t * args, char * argv[], int argc)
{
    return lv_remote_ctrl_args_init(args, argc, (const char **)argv);
}

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_REMOTE_CTRL*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_REMOTE_CTRL_H*/
