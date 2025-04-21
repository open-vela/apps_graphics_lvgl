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
    char perf_tag[LV_REMOTE_CTRL_CMD_STR_LEN];
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
    print_func("  perf                                    Performance monitor\n");
    print_func("Subcommands for perf:\n");
    print_func("  create <tag> <max_events> <max_scrolls> Create performance monitor\n");
    print_func("  destroy                                 Destroy monitor\n");
    print_func("  start [immediate]                       Start monitoring, immediate is 1 or 0, default 0 (delay start until the first render finished)\n");
    print_func("  stop                                    Stop monitoring\n");
    print_func("  reset                                   Reset monitoring data\n");
    print_func("  data                                    Get monitoring data\n");
    print_func("  trace                                   Generate trace data\n");
    print_func("  csv <file_name>                         Generate CSV data to file (append)\n");
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
        if(size < 4) {
            return LV_RESULT_INVALID;
        }

        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_CREATE;
        lv_strncpy(cmd->cfg.sysmon_perf_create.tag, info[1], sizeof(cmd->cfg.sysmon_perf_create.tag) - 1);
        cmd->cfg.sysmon_perf_create.tag[sizeof(cmd->cfg.sysmon_perf_create.tag) - 1] = '\0';
        cmd->cfg.sysmon_perf_create.max_events = atoi(info[2]);
        cmd->cfg.sysmon_perf_create.max_scrolls = atoi(info[3]);
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
    else if(lv_strcmp(subcommand, "csv") == 0) {
        cmd->cmd = LV_REMOTE_CTRL_CMD_SYSMON_PERF_CSV;
        lv_strncpy(cmd->cfg.sysmon_perf_csv.file_name, info[1], sizeof(cmd->cfg.sysmon_perf_csv.file_name) - 1);
        cmd->cfg.sysmon_perf_csv.file_name[sizeof(cmd->cfg.sysmon_perf_csv.file_name) - 1] = '\0';
    }
    else {
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR
static void lv_remote_ctrl_sysmon_perf_print_head(lv_fs_file_t * file)
{
    static const char * head =
        "tag,id,start_ms,duration_ms,fps_redraw,fps_refr,refr_cnt,redraw_cnt,refr_avg_ms,render_avg_ms,flush_avg_ms\n";
    if(file) {
        uint32_t pos;
        if(lv_fs_tell(file, &pos) != LV_FS_RES_OK) {
            LV_LOG_ERROR("Failed to get file position");
            return;
        }
        if(pos == 0) {
            lv_fs_write(file, head, lv_strlen(head), NULL);
        }
    }
    else {
        LV_LOG("%s", head);
    }
}

static void lv_remote_ctrl_sysmon_perf_print_line(lv_fs_file_t * file, const lv_sysmon_perf_info_t * info,
                                                  const char * tag, const char * id)
{
    static const char * fmt = "%s,%s,%" LV_PRIu32 ",%" LV_PRIu32
                              ",%" LV_PRFv32(".2f") ",%" LV_PRFv32(".2f") ",%" LV_PRIu32 ",%" LV_PRIu32
                              ",%" LV_PRFv32(".2f") ",%" LV_PRFv32(".2f") ",%" LV_PRFv32(".2f")"\n";
    char buf[LV_REMOTE_CTRL_CMD_STR_LEN];

    lv_snprintf(buf, sizeof(buf), fmt, tag, id, info->measured.perf_start, info->calculated.duration,
                info->calculated.fps, info->calculated.fps_refr, info->measured.refr_cnt, info->measured.render_cnt,
                info->calculated.refr_avg_time, info->calculated.render_avg_time, info->calculated.flush_avg_time);
    if(file) {
        lv_fs_write(file, buf, lv_strlen(buf), NULL);
    }
    else {
        LV_LOG("%s", buf);
    }
}

static void lv_remote_ctrl_sysmon_handler(lv_remote_ctrl_ctx_t * ctx, const lv_remote_ctrl_cmd_t * cmd)
{
    const lv_sysmon_perf_data_t * data = NULL;
    lv_fs_file_t * file = NULL;
    lv_fs_file_t csv;

    switch(cmd->cmd) {
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_CREATE:
            if(ctx->perf) {
                LV_LOG_WARN("Sysmon perf has already been created, replace it");
                lv_sysmon_perf_destroy(ctx->perf);
            }
            lv_strncpy(ctx->perf_tag, cmd->cfg.sysmon_perf_create.tag, sizeof(ctx->perf_tag) - 1);
            ctx->perf = lv_sysmon_perf_create(ctx->perf_tag, cmd->cfg.sysmon_perf_create.max_events,
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
        case LV_REMOTE_CTRL_CMD_SYSMON_PERF_CSV:
            data = lv_sysmon_perf_get_data(ctx->perf);
            if(lv_fs_open(&csv, cmd->cfg.sysmon_perf_csv.file_name, LV_FS_MODE_WR | LV_FS_MODE_RD) != LV_FS_RES_OK) {
                LV_LOG_ERROR("Failed to open file %s", cmd->cfg.sysmon_perf_csv.file_name);
                return;
            }
            if(lv_fs_seek(&csv, 0, LV_FS_SEEK_END) != LV_FS_RES_OK) {
                LV_LOG_ERROR("Failed to seek to end of file %s", cmd->cfg.sysmon_perf_csv.file_name);
                lv_fs_close(&csv);
                return;
            }
            file = &csv;
            break;
        default:
            break;
    }

    if(data) {
        lv_remote_ctrl_sysmon_perf_print_head(file);
        lv_remote_ctrl_sysmon_perf_print_line(file, &data->overall, ctx->perf_tag, "overall");
        if(data->scrolls) {
            uint32_t size = lv_circle_buf_size(data->scrolls);
            lv_sysmon_perf_info_t info;
            char id[16];
            for(uint32_t i = 0; i < size; i++) {
                lv_snprintf(id, sizeof(id), "scroll-%" LV_PRIu32, i);
                lv_circle_buf_peek_at(data->scrolls, i, &info);
                lv_remote_ctrl_sysmon_perf_print_line(file, &info, ctx->perf_tag, id);
            }
        }
    }
    if(file) {
        lv_fs_close(file);
    }
}
#endif /*LV_USE_PERF_MONITOR*/

#endif /*LV_USE_REMOTE_CTRL*/
