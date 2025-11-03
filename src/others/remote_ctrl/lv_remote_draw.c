/**
 * @file lv_remote_draw.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../libs/argparse/argparse.h"
#include "../../draw/lv_draw.h"
#include "../../core/lv_global.h"

/*********************
 *      DEFINES
 *********************/

#define _draw_info LV_GLOBAL_DEFAULT()->draw_info

#define TASK_MASK_ALL 0xFFFFFFFF

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_draw_unit_t hook_unit;
    lv_draw_unit_t * ori_unit;
    lv_draw_unit_t * prev_unit;
    lv_draw_task_type_t act_task_type;
    uint32_t dispatch_task_mask;
    uint32_t evaluate_task_mask;
    int32_t (*ori_dispatch_cb)(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
    int32_t (*ori_evaluate_cb)(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
} draw_ctx_t;

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
    print_func("draw - Drawing event hooks and logging.\n");
}

static void constructor_cb(void * ctx)
{
    draw_ctx_t * draw_ctx = ctx;
    draw_ctx->dispatch_task_mask = TASK_MASK_ALL;
    draw_ctx->evaluate_task_mask = TASK_MASK_ALL;
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static int32_t dispatch_hook_cb(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    draw_ctx_t * ctx = (draw_ctx_t *)draw_unit;

    LV_LOG_USER("task type: %d", ctx->act_task_type);

    if(!(ctx->dispatch_task_mask & (1 << ctx->act_task_type))) {
        LV_LOG_USER("mask: %" LV_PRIu32 ", intercepted", ctx->dispatch_task_mask);
        return -1;
    }

    int32_t ret = ctx->ori_dispatch_cb(ctx->ori_unit, layer);
    LV_LOG_USER("ori_dispatch_cb: %p, ret: %d", (void *)(lv_uintptr_t)ctx->ori_dispatch_cb, (int)ret);
    return ret;
}

static int32_t evaluate_hook_cb(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    draw_ctx_t * ctx = (draw_ctx_t *)draw_unit;
    ctx->act_task_type = task->type;

    LV_LOG_USER("task type: %d", ctx->act_task_type);

    if(!(ctx->evaluate_task_mask & (1 << ctx->act_task_type))) {
        LV_LOG_USER("mask: %" LV_PRIu32 ", intercepted", ctx->evaluate_task_mask);
        return 0;
    }

    if(task->draw_dsc) {
        lv_draw_dsc_base_t * dsc = task->draw_dsc;
        LV_LOG_USER("obj: %p, part: %08x", (void *)dsc->obj, (int)dsc->part);
    }

    int32_t ret = ctx->ori_evaluate_cb(ctx->ori_unit, task);
    LV_LOG_USER("ori_evaluate_cb: %p, ret: %d", (void *)(lv_uintptr_t)ctx->ori_evaluate_cb, (int)ret);
    LV_LOG_USER("preference_score: %d, preferred_draw_unit_id: %d", task->preference_score, task->preferred_draw_unit_id);
    return ret;
}

static void dump_draw_unit_info(lv_draw_unit_t * unit)
{
    LV_LOG_USER("unit: %p", (void *)unit);
    LV_LOG_USER("  next: %p", (void *)unit->next);
    LV_LOG_USER("  target_layer: %p", (void *)unit->target_layer);
    LV_LOG_USER("  clip_area: %p", (void *)unit->clip_area);
    LV_LOG_USER("  name: %s", unit->name);
    LV_LOG_USER("  dispatch_cb: %p", (void *)(lv_uintptr_t)unit->dispatch_cb);
    LV_LOG_USER("  evaluate_cb: %p", (void *)(lv_uintptr_t)unit->evaluate_cb);
    LV_LOG_USER("  delete_cb: %p", (void *)(lv_uintptr_t)unit->delete_cb);
}

static int task_mask_cmd_cb(struct argparse * self, const struct argparse_option * option)
{
    LV_UNUSED(self);
    uint32_t mask = *(unsigned long *)option->value;
    LV_LOG_USER("mask: %08x", (int)mask);
    *(uint32_t *)option->data = mask;
    return 0;
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    draw_ctx_t * draw_ctx = ctx;
    LV_UNUSED(draw_ctx);

    lv_draw_unit_t * unit = NULL;
    int dump_unit = 0;
    int hook = -1;
    unsigned long dispatch_task_mask = TASK_MASK_ALL;
    unsigned long evaluate_task_mask = TASK_MASK_ALL;
    int send_event_code = -1;
    const char * unit_name = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_HEX(0, "unit", &unit, "The drawing unit address", NULL, 0, 0),
        OPT_BOOLEAN(0, "dump", &dump_unit, "Dump the drawing unit to the console", NULL, 0, 0),
        OPT_INTEGER(0, "hook", &hook, "hook dispatch and evaluate callbacks", NULL, 0, 0),
        OPT_HEX(0, "dispatch-task-mask", &dispatch_task_mask, "The dispatch task mask for hooking", task_mask_cmd_cb, (intptr_t)&draw_ctx->dispatch_task_mask, 0),
        OPT_HEX(0, "evaluate-task-mask", &evaluate_task_mask, "The evaluate task mask for hooking", task_mask_cmd_cb, (intptr_t)&draw_ctx->evaluate_task_mask, 0),
        OPT_INTEGER(0, "send-event", &send_event_code, "Send an event to the drawing unit", NULL, 0, 0),
        OPT_STRING(0, "unit-name", &unit_name, "The name of the drawing unit", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(dump_unit) {
        if(unit) {
            dump_draw_unit_info(unit);
        }
        else {
            lv_draw_unit_t * u = _draw_info.unit_head;
            while(u) {
                dump_draw_unit_info(u);
                u = u->next;
            }
        }
        return LV_RESULT_OK;
    }

    if(hook == 0) {
        if(!draw_ctx->ori_unit) {
            LV_LOG_WARN("No hook installed");
            return LV_RESULT_INVALID;
        }

        if(draw_ctx->prev_unit) {
            draw_ctx->prev_unit->next = draw_ctx->ori_unit;
        }
        else {
            _draw_info.unit_head = draw_ctx->ori_unit;
        }

        LV_LOG_USER("Unit unhooked: %s", draw_ctx->ori_unit->name);
        lv_memzero(&draw_ctx->hook_unit, sizeof(draw_ctx->hook_unit));
        draw_ctx->ori_unit = NULL;
        draw_ctx->prev_unit = NULL;
        return LV_RESULT_OK;
    }

    if(send_event_code >= 0) {
        if(send_event_code >= _LV_EVENT_LAST) {
            LV_LOG_WARN("Invalid event code: %d, out of range: [0, %d)", send_event_code, _LV_EVENT_LAST);
            return LV_RESULT_INVALID;
        }

        LV_LOG_USER("Sending event: %d(%s), to %s",
                    send_event_code, lv_event_get_code_name((lv_event_code_t)send_event_code),
                    unit_name ? unit_name : "all units");
        lv_draw_unit_send_event(unit_name, (lv_event_code_t)send_event_code, NULL);
        return LV_RESULT_OK;
    }

    if(!unit) {
        LV_LOG_WARN("No unit specified");
        return LV_RESULT_INVALID;
    }

    lv_draw_unit_t * u = _draw_info.unit_head;
    lv_draw_unit_t * u_prev = NULL;
    while(u) {
        if(u == unit) {
            break;
        }
        u_prev = u;
        u = u->next;
    }

    if(!u) {
        LV_LOG_WARN("Unit: %p not found", (void *)unit);
        return LV_RESULT_INVALID;
    }

    if(hook > 0) {
        if(draw_ctx->ori_unit) {
            LV_LOG_WARN("Hook already installed: %p", (void *)draw_ctx->ori_unit);
            return LV_RESULT_INVALID;
        }

        draw_ctx->hook_unit = *unit;
        draw_ctx->hook_unit.dispatch_cb = dispatch_hook_cb;
        draw_ctx->hook_unit.evaluate_cb = evaluate_hook_cb;
        draw_ctx->ori_dispatch_cb = unit->dispatch_cb;
        draw_ctx->ori_evaluate_cb = unit->evaluate_cb;
        draw_ctx->ori_unit = unit;
        draw_ctx->prev_unit = u_prev;

        if(u_prev) {
            u_prev->next = &draw_ctx->hook_unit;
        }
        else {
            _draw_info.unit_head = &draw_ctx->hook_unit;
        }

        LV_LOG_USER("Unit hooked: %s", unit->name);
        return LV_RESULT_OK;
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(draw, sizeof(draw_ctx_t))

#endif /*LV_USE_REMOTE_CTRL*/
