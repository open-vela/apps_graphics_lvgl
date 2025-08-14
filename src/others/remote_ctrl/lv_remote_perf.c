/**
 * @file lv_remote_perf.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL && defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR

#include "../../misc/lv_circle_buf.h"
#include "../sysmon/lv_sysmon.h"
#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    PERF_CMD_NONE,
    PERF_CMD_CREATE, /**< Create sysmon performance monitor */
    PERF_CMD_DESTROY, /**< Destroy sysmon performance monitor */
    PERF_CMD_START, /**< Start sysmon performance monitor */
    PERF_CMD_STOP, /**< Stop sysmon performance monitor */
    PERF_CMD_RESET, /**< Reset sysmon performance monitor */
    PERF_CMD_DATA, /**< Get sysmon performance monitor data */
    PERF_CMD_TRACE, /**< Write sysmon performance monitor data to file */
    PERF_CMD_CSV, /**< Write sysmon performance monitor data to CSV file */
} perf_cmd_t;

typedef struct {
    perf_cmd_t cmd;
    union {
        struct {
            size_t max_events;
            size_t max_scrolls;
            const char * tag;
        } perf_create;
        struct {
            bool immediate;
        } perf_start;
        struct {
            const char * file_name;
        } perf_csv;
    } sysmon;
} perf_cfg_t;

typedef struct {
    lv_sysmon_perf_t * instance;
    char tag[128];
} perf_ctx_t;

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
    char buf[128];

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

static void show_help_cb(lv_remote_ctrl_print_func_t print_func)
{
    print_func("  perf                                    Performance monitor\n");
    print_func("Subcommands:\n");
    print_func("  create <tag> <max_events> <max_scrolls> Create performance monitor\n");
    print_func("  destroy                                 Destroy monitor\n");
    print_func("  start [immediate]                       Start monitoring, immediate is 1 or 0, default 0 (delay start until the first render finished)\n");
    print_func("  stop                                    Stop monitoring\n");
    print_func("  reset                                   Reset monitoring data\n");
    print_func("  data                                    Get monitoring data\n");
    print_func("  trace                                   Generate trace data\n");
    print_func("  csv <file_name>                         Generate CSV data to file (append)\n");
}

static bool arg_parse(perf_cfg_t * cfg, int argc, const char * argv[])
{
    int size = argc - 1;
    const char ** info = argv + 1;

    lv_memzero(cfg, sizeof(perf_cfg_t));

    if(size < 1) {
        LV_LOG_WARN("No subcommand");
        return false;
    }

    const char * subcommand = info[0];
    if(lv_strcmp(subcommand, "create") == 0) {
        if(size < 4) {
            LV_LOG_WARN("Invalid arguments for create");
            return false;
        }

        cfg->cmd = PERF_CMD_CREATE;
        cfg->sysmon.perf_create.tag = info[1];
        cfg->sysmon.perf_create.max_events = atoi(info[2]);
        cfg->sysmon.perf_create.max_scrolls = atoi(info[3]);
        return true;
    }

    if(lv_strcmp(subcommand, "destroy") == 0) {
        cfg->cmd = PERF_CMD_DESTROY;
        return true;
    }

    if(lv_strcmp(subcommand, "start") == 0) {
        cfg->cmd = PERF_CMD_START;
        if(size > 1) {
            cfg->sysmon.perf_start.immediate = atoi(info[1]) != 0;
        }
        else {
            cfg->sysmon.perf_start.immediate = false;
        }

        return true;
    }

    if(lv_strcmp(subcommand, "stop") == 0) {
        cfg->cmd = PERF_CMD_STOP;
        return true;
    }

    if(lv_strcmp(subcommand, "reset") == 0) {
        cfg->cmd = PERF_CMD_RESET;
        return true;
    }

    if(lv_strcmp(subcommand, "data") == 0) {
        cfg->cmd = PERF_CMD_DATA;
        return true;
    }

    if(lv_strcmp(subcommand, "trace") == 0) {
        cfg->cmd = PERF_CMD_TRACE;
        return true;
    }

    if(lv_strcmp(subcommand, "csv") == 0) {
        if(size < 2) {
            LV_LOG_WARN("Invalid arguments for csv");
            return false;
        }

        cfg->cmd = PERF_CMD_CSV;
        cfg->sysmon.perf_csv.file_name = info[1];
        return true;
    }

    return false;
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    perf_ctx_t * perf_ctx = ctx;
    if(perf_ctx->instance) {
        lv_sysmon_perf_destroy(perf_ctx->instance);
        perf_ctx->instance = NULL;
    }
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    perf_cfg_t cfg;
    if(!arg_parse(&cfg, argc, argv)) {
        return LV_RESULT_INVALID;
    }

    perf_ctx_t * perf_ctx = ctx;

    const lv_sysmon_perf_data_t * data = NULL;
    lv_fs_file_t * file = NULL;
    lv_fs_file_t csv;

    switch(cfg.cmd) {
        case PERF_CMD_CREATE:
            if(perf_ctx->instance) {
                LV_LOG_WARN("Sysmon perf has already been created, replace it");
                lv_sysmon_perf_destroy(perf_ctx->instance);
            }
            lv_memzero(perf_ctx->tag, sizeof(perf_ctx->tag));
            lv_strncpy(perf_ctx->tag, cfg.sysmon.perf_create.tag, sizeof(perf_ctx->tag) - 1);
            perf_ctx->instance = lv_sysmon_perf_create(perf_ctx->tag, cfg.sysmon.perf_create.max_events,
                                                       cfg.sysmon.perf_create.max_scrolls);
            break;
        case PERF_CMD_DESTROY:
            if(perf_ctx->instance) {
                lv_sysmon_perf_destroy(perf_ctx->instance);
                perf_ctx->instance = NULL;
            }
            break;
        case PERF_CMD_START:
            if(lv_sysmon_perf_start(perf_ctx->instance, cfg.sysmon.perf_start.immediate) == LV_RESULT_INVALID) {
                LV_LOG_WARN("Sysmon perf is not created or already started");
            }
            break;
        case PERF_CMD_STOP:
            data = lv_sysmon_perf_stop(perf_ctx->instance);
            break;
        case PERF_CMD_RESET:
            lv_sysmon_perf_reset_data(perf_ctx->instance, LV_SYSMON_PERF_TYPE_ALL);
            break;
        case PERF_CMD_DATA:
            data = lv_sysmon_perf_get_data(perf_ctx->instance);
            break;
        case PERF_CMD_TRACE:
            lv_sysmon_perf_generate_trace(perf_ctx->instance);
            break;
        case PERF_CMD_CSV:
            data = lv_sysmon_perf_get_data(perf_ctx->instance);
            if(lv_fs_open(&csv, cfg.sysmon.perf_csv.file_name, LV_FS_MODE_WR | LV_FS_MODE_RD) != LV_FS_RES_OK) {
                LV_LOG_ERROR("Failed to open file %s", cfg.sysmon.perf_csv.file_name);
                return LV_RESULT_INVALID;
            }
            if(lv_fs_seek(&csv, 0, LV_FS_SEEK_END) != LV_FS_RES_OK) {
                LV_LOG_ERROR("Failed to seek to end of file %s", cfg.sysmon.perf_csv.file_name);
                lv_fs_close(&csv);
                return LV_RESULT_INVALID;
            }
            file = &csv;
            break;

        default:
            LV_LOG_WARN("Invalid command: %d", cfg.cmd);
            return LV_RESULT_INVALID;
    }

    if(data) {
        lv_remote_ctrl_sysmon_perf_print_head(file);
        lv_remote_ctrl_sysmon_perf_print_line(file, &data->overall, perf_ctx->tag, "overall");
        if(data->scrolls) {
            uint32_t size = lv_circle_buf_size(data->scrolls);
            lv_sysmon_perf_info_t info;
            char id[16];
            for(uint32_t i = 0; i < size; i++) {
                lv_snprintf(id, sizeof(id), "scroll-%" LV_PRIu32, i);
                lv_circle_buf_peek_at(data->scrolls, i, &info);
                lv_remote_ctrl_sysmon_perf_print_line(file, &info, perf_ctx->tag, id);
            }
        }
    }
    if(file) {
        lv_fs_close(file);
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(perf, sizeof(perf_ctx_t))

#endif /*LV_USE_REMOTE_CTRL && LV_USE_PERF_MONITOR*/
