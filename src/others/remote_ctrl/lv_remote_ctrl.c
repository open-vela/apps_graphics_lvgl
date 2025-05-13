/**
 * @file lv_remote_ctrl.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_remote_ctrl.h"

#if LV_USE_REMOTE_CTRL

#include <stdlib.h>

#include "../../misc/lv_circle_buf.h"
#include "../../misc/lv_log.h"
#include "../../stdlib/lv_string.h"
#include "../sysmon/lv_sysmon.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_remote_ctrl_ctx_t {
#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR
    lv_sysmon_perf_t * perf;
#endif
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_result_t lv_remote_ctrl_fill_perf_cmd(lv_remote_ctrl_cmd_t * cmd, char * info[], int size);
static void lv_remote_ctrl_sysmon_handler(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_cmd_t * cmd);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_remote_ctrl_ctx_t * lv_remote_ctrl_create(void)
{
    lv_remote_ctrl_ctx_t * ctx = lv_malloc_zeroed(sizeof(lv_remote_ctrl_ctx_t));
    LV_ASSERT_MALLOC(ctx);
    return ctx;
}

void lv_remote_ctrl_destroy(lv_remote_ctrl_ctx_t * ctx)
{
#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR
    if(ctx->perf) {
        lv_sysmon_perf_destroy(ctx->perf);
        ctx->perf = NULL;
    }
#endif
    lv_free(ctx);
}

void lv_remote_ctrl_show_help(const char * cmd_name, lv_remote_ctrl_print_func_t print_func)
{
    print_func("Usage: %s <command> <subcommand> [parameters]\n", cmd_name);
    print_func("Commands:\n");
    print_func("  perf                              Performance monitor\n");
    print_func("Subcommands for perf:\n");
    print_func("  create <max_events> <max_scrolls> Create performance monitor\n");
    print_func("  destroy                           Destroy monitor\n");
    print_func("  start [immediate]                 Start monitoring, immediate is 1 or 0, default 0 (delay start until the first render finished)\n");
    print_func("  stop                              Stop monitoring\n");
    print_func("  reset                             Reset monitoring data\n");
    print_func("  data                              Get monitoring data\n");
    print_func("  trace                             Generate trace data\n");
}

lv_result_t lv_remote_ctrl_cmd_parse(lv_remote_ctrl_cmd_t * cmd, char * info[], int size)
{
    const char * command;
    if(size < 1) {
        return LV_RESULT_INVALID;
    }

    command = info[0];

    if(lv_strcmp(command, "perf") == 0) {
        return lv_remote_ctrl_fill_perf_cmd(cmd, info + 1, size - 1);
    }

    return LV_RESULT_INVALID;
}

lv_result_t lv_remote_ctrl_cmd_execute(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_cmd_t * cmd)
{
#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR
    if(cmd->cmd >= LV_REMOTE_CTRL_CMD_SYSMON_MIN && cmd->cmd <= LV_REMOTE_CTRL_CMD_SYSMON_MAX) {
        lv_remote_ctrl_sysmon_handler(ctx, cmd);
    }
#endif /*LV_USE_PERF_MONITOR*/
    return LV_RESULT_OK;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_result_t lv_remote_ctrl_fill_perf_cmd(lv_remote_ctrl_cmd_t * cmd, char * info[], int size)
{
    const char * subcommand;
    if(size < 1) {
        return LV_RESULT_INVALID;
    }

    subcommand = info[0];
    if(lv_strcmp(subcommand, "create") == 0) {
        if(size < 3) {
            return LV_RESULT_INVALID;
        }

        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_CREATE;
        cmd->cfg.sysmon_perf_create.max_events = atoi(info[1]);
        cmd->cfg.sysmon_perf_create.max_scrolls = atoi(info[2]);
    }
    else if(lv_strcmp(subcommand, "destroy") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_DESTROY;
    }
    else if(lv_strcmp(subcommand, "start") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_START;
        if(size > 1) {
            cmd->cfg.sysmon_perf_start.immediate = atoi(info[1]) != 0;
        }
        else {
            cmd->cfg.sysmon_perf_start.immediate = false;
        }
    }
    else if(lv_strcmp(subcommand, "stop") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_STOP;
    }
    else if(lv_strcmp(subcommand, "reset") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_RESET;
    }
    else if(lv_strcmp(subcommand, "data") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_DATA;
    }
    else if(lv_strcmp(subcommand, "trace") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_TRACE;
    }
    else {
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR
static void lv_remote_ctrl_sysmon_perf_print(const lv_sysmon_perf_info_t * info, int id)
{
    LV_LOG("[%d] start %" LV_PRIu32 " duration %" LV_PRIu32 "ms, "
           "%" LV_PRFv32(".2f") " FPS (refr_cnt: %" LV_PRIu32 " | redraw_cnt: %" LV_PRIu32"), "
           "refr %" LV_PRFv32(".2f") "ms (render %" LV_PRFv32(".2f") "ms | flush %" LV_PRFv32(".2f") "ms), "
           "CPU %" LV_PRIu32 "%%\n",
           id, info->measured.perf_start, info->calculated.duration,
           info->calculated.fps, info->measured.refr_cnt, info->measured.render_cnt,
           info->calculated.refr_avg_time, info->calculated.render_avg_time, info->calculated.flush_avg_time,
           info->calculated.cpu);
}

static void lv_remote_ctrl_sysmon_handler(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_cmd_t * cmd)
{
    const lv_sysmon_perf_data_t * data = NULL;
    switch(cmd->cmd) {
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_CREATE:
            if(ctx->perf) {
                LV_LOG_WARN("Sysmon perf has already been created, replace it");
                lv_sysmon_perf_destroy(ctx->perf);
            }
            ctx->perf = lv_sysmon_perf_create("lv_remote_ctrl", cmd->cfg.sysmon_perf_create.max_events,
                                              cmd->cfg.sysmon_perf_create.max_scrolls);
            break;
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_DESTROY:
            if(ctx->perf) {
                lv_sysmon_perf_destroy(ctx->perf);
                ctx->perf = NULL;
            }
            break;
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_START:
            if(lv_sysmon_perf_start(ctx->perf, cmd->cfg.sysmon_perf_start.immediate) == LV_RESULT_INVALID) {
                LV_LOG_WARN("Sysmon perf is not created or already started");
            }
            break;
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_STOP:
            data = lv_sysmon_perf_stop(ctx->perf);
            break;
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_RESET:
            lv_sysmon_perf_reset_data(ctx->perf, LV_SYSMON_PERF_TYPE_ALL);
            break;
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_DATA:
            data = lv_sysmon_perf_get_data(ctx->perf);
            break;
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_TRACE:
            lv_sysmon_perf_generate_trace(ctx->perf);
            break;
        default:
            break;
    }

    if(data) {
        LV_LOG("Perf data:");
        lv_remote_ctrl_sysmon_perf_print(&data->overall, 0);
        if(data->scrolls) {
            uint32_t size = lv_circle_buf_size(data->scrolls);
            lv_sysmon_perf_info_t info;
            LV_LOG("Scrolls:");
            for(uint32_t i = 0; i < size; i++) {
                lv_circle_buf_peek_at(data->scrolls, i, &info);
                lv_remote_ctrl_sysmon_perf_print(&info, i);
            }
            LV_LOG("End of scrolls");
        }
    }
}
#endif /*LV_USE_PERF_MONITOR*/

#endif /*LV_USE_REMOTE_CTRL*/
