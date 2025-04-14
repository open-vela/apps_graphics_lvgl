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

#include <stddef.h>

#include "../../misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    enum {
        LV_REMOTE_CTRL_CMD_NONE,
        /* Sysmon commands begin */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_CREATE, /**< Create sysmon performance monitor */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_DESTROY, /**< Destroy sysmon performance monitor */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_START, /**< Start sysmon performance monitor */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_STOP, /**< Stop sysmon performance monitor */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_RESET, /**< Reset sysmon performance monitor */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_DATA, /**< Get sysmon performance monitor data */
        LV_REMOTE_CTRL_CMD_SYSMON_PERF_TRACE, /**< Write sysmon performance monitor data to file */
        LV_REMOTE_CTRL_CMD_SYSMON_MIN = LV_REMOTE_CTRL_CMD_SYSMON_PERF_CREATE,
        LV_REMOTE_CTRL_CMD_SYSMON_MAX = LV_REMOTE_CTRL_CMD_SYSMON_PERF_TRACE,
        /* Sysmon commands end */
    } cmd;
    union {
        struct {
            size_t max_events;
            size_t max_scrolls;
        } sysmon_perf_create;
    } cfg;
} lv_remote_ctrl_cmd_t;

typedef void (*lv_remote_ctrl_print_func_t)(const char * format, ...);

struct _lv_remote_ctrl_ctx_t;
typedef struct _lv_remote_ctrl_ctx_t lv_remote_ctrl_ctx_t;

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
 * Show help for remote control commands
 * @param cmd_name Name of the command to show help for
 * @param print_func Pointer to print function
 */
void lv_remote_ctrl_show_help(const char * cmd_name, lv_remote_ctrl_print_func_t print_func);

/**
 * Parse a remote control command
 * @param cmd Pointer to remote control command to fill
 * @param info Pointer to command arguments
 * @param size Size of command arguments
 * @return LV_RESULT_OK if the command is parsed successfully, LV_RESULT_INVALID otherwise
 */
lv_result_t lv_remote_ctrl_cmd_parse(lv_remote_ctrl_cmd_t * cmd, char * info[], int size);

/**
 * Execute a remote control command
 * @param ctx Pointer to remote control context
 * @param cmd Pointer to remote control command to execute
 * @return LV_RESULT_OK if the command is executed successfully, LV_RESULT_INVALID otherwise
 */
lv_result_t lv_remote_ctrl_cmd_execute(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_cmd_t * cmd);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_REMOTE_CTRL*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_REMOTE_CTRL_H*/
