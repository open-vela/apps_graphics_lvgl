/**
 * @file lv_remote_snapshot.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../display/lv_display.h"
#include "../../misc/lv_utils.h"
#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/

#define SNAPSHOT_BORDER_SIZE 3

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    SNAPSHOT_CMD_NONE,
    SNAPSHOT_CMD_TAKE,
    SNAPSHOT_CMD_SAVE,
} snapshot_cmd_t;

typedef struct {
    snapshot_cmd_t cmd;
    const char * file_name;
    size_t cnt;
    size_t idx;
    uint16_t offset;
    bool by_x;
} snapshot_cfg_t;

typedef struct {
    lv_display_t * disp;
    lv_draw_buf_t * buf;
    snapshot_cfg_t cfg;
} snapshot_ctx_t;

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

static void on_disp_flush_start(lv_event_t * e)
{
    snapshot_ctx_t * ctx = lv_event_get_user_data(e);
    if(!ctx->buf) {
        return;
    }

    lv_display_t * disp = lv_event_get_target(e);
    if(!lv_display_flush_is_last(disp)) {
        return;
    }

    lv_draw_buf_t * buf = lv_display_get_buf_active(disp);
    if(!buf) {
        LV_LOG_ERROR("No active buffer");
        return;
    }

    if(ctx->cfg.cnt < 1) {
        return;
    }

    if(ctx->cfg.by_x) {
        uint16_t w = buf->header.w / ctx->cfg.cnt;
        uint16_t x = ctx->cfg.idx * w;
        lv_area_t dest_area = {x, 0, x + w - 1 - SNAPSHOT_BORDER_SIZE, buf->header.h - 1};
        lv_area_t src_area = {ctx->cfg.offset, 0, ctx->cfg.offset + w - 1 - SNAPSHOT_BORDER_SIZE, buf->header.h - 1};
        lv_draw_buf_copy(ctx->buf, &dest_area, buf, &src_area);
    }
    else {
        uint16_t h = buf->header.h / ctx->cfg.cnt;
        uint16_t y = ctx->cfg.idx * h;
        lv_area_t dest_area = {0, y, buf->header.w - 1, y + h - 1 - SNAPSHOT_BORDER_SIZE};
        lv_area_t src_area = {0, ctx->cfg.offset, buf->header.w - 1, ctx->cfg.offset + h - 1 - SNAPSHOT_BORDER_SIZE};
        lv_draw_buf_copy(ctx->buf, &dest_area, buf, &src_area);
    }

    if(++ctx->cfg.idx >= ctx->cfg.cnt) {
        ctx->cfg.idx = 0;
    }
}

static void show_help_cb(lv_remote_ctrl_print_func_t print_func)
{
    print_func("  snapshot                                Take snapshots\n");
    print_func("Subcommands:\n");
    print_func("  take <count> [by_x] [offset]            Take <count> snapshots into one image, whether the image is joint horizontally or vertically depends on the <by_x> parameter\n");
    print_func("  save <file_name>                        Save snapshots to file\n");
}

static bool arg_parse(snapshot_cfg_t * cfg, int argc, const char * argv[])
{
    if(argc < 2) {
        LV_LOG_ERROR("Missing subcommand");
        return false;
    }

    lv_memzero(cfg, sizeof(snapshot_cfg_t));

    const char * subcommand = argv[1];
    if(lv_strcmp(subcommand, "take") == 0) {
        if(argc < 3) {
            LV_LOG_ERROR("Missing count");
            return false;
        }

        cfg->cmd = SNAPSHOT_CMD_TAKE;
        const int cnt = atoi(argv[2]);
        if(cnt < 1) {
            LV_LOG_ERROR("Invalid count: %d", cnt);
            return false;
        }

        cfg->cnt = cnt;
        if(argc > 3) {
            cfg->by_x = atoi(argv[3]) != 0;
        }

        if(argc > 4) {
            const int offset = atoi(argv[4]);
            if(offset < 0) {
                LV_LOG_ERROR("Invalid offset: %d", offset);
                return false;
            }

            cfg->offset = offset;
        }

        return true;
    }

    if(lv_strcmp(subcommand, "save") == 0) {
        if(argc < 3) {
            LV_LOG_ERROR("Missing file name");
            return false;
        }

        cfg->cmd = SNAPSHOT_CMD_SAVE;
        cfg->file_name = argv[2];

        return true;
    }

    LV_LOG_ERROR("Invalid subcommand: %s", subcommand);
    return false;
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    snapshot_ctx_t * snapshot_ctx = ctx;

    if(snapshot_ctx->disp) {
        lv_display_remove_event_cb_with_user_data(snapshot_ctx->disp, on_disp_flush_start, snapshot_ctx);
        snapshot_ctx->disp = NULL;
    }

    if(snapshot_ctx->buf) {
        lv_draw_buf_destroy(snapshot_ctx->buf);
        snapshot_ctx->buf = NULL;
    }
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    snapshot_ctx_t * snapshot_ctx = ctx;

    snapshot_cfg_t cfg;
    if(!arg_parse(&cfg, argc, argv)) {
        return LV_RESULT_INVALID;
    }

    snapshot_ctx->cfg = cfg;

    if(!snapshot_ctx->disp) {
        snapshot_ctx->disp = lv_display_get_default();
        if(!snapshot_ctx->disp) {
            LV_LOG_ERROR("No display found");
            return LV_RESULT_INVALID;
        }
        lv_display_add_event_cb(snapshot_ctx->disp, on_disp_flush_start, LV_EVENT_FLUSH_START, snapshot_ctx);
    }

    const lv_draw_buf_t * buf = lv_display_get_buf_active(snapshot_ctx->disp);
    if(!buf) {
        LV_LOG_ERROR("No active buffer");
        return LV_RESULT_INVALID;
    }

    switch(snapshot_ctx->cfg.cmd) {
        case SNAPSHOT_CMD_TAKE: {
                if(snapshot_ctx->buf) {
                    LV_LOG_WARN("Snapshot buf: %p is already in progress, please save to file", (void *)snapshot_ctx->buf);
                    return LV_RESULT_OK;
                }

                snapshot_ctx->buf = lv_draw_buf_create(buf->header.w, buf->header.h, buf->header.cf, buf->header.stride);
                if(!snapshot_ctx->buf) {
                    LV_LOG_ERROR("Failed to create snapshot buffer");
                    return LV_RESULT_INVALID;
                }
                lv_draw_buf_clear(snapshot_ctx->buf, NULL);
            }
            break;

        case SNAPSHOT_CMD_SAVE: {
                if(!snapshot_ctx->buf) {
                    LV_LOG_USER("Direct save active buffer to file: %s", snapshot_ctx->cfg.file_name);
                    return lv_draw_buf_save_to_file(buf, snapshot_ctx->cfg.file_name);
                }

                lv_result_t res = lv_draw_buf_save_to_file(snapshot_ctx->buf, snapshot_ctx->cfg.file_name);
                if(res != LV_RESULT_OK) {
                    LV_LOG_ERROR("Failed to save snapshot to file: %d", res);
                    return res;
                }

                lv_draw_buf_destroy(snapshot_ctx->buf);
                snapshot_ctx->buf = NULL;
            }
            break;

        default:
            LV_LOG_WARN("Invalid command: %d", snapshot_ctx->cfg.cmd);
            return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(snapshot, sizeof(snapshot_ctx_t))

#endif /*LV_USE_REMOTE_CTRL*/
