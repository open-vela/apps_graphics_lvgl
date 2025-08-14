/**
 * @file lv_remote_obj.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../snapshot/lv_snapshot.h"
#include "../../misc/lv_utils.h"
#include "../../libs/argparse/argparse.h"
#include "../../core/lv_obj_tree.h"
#include "../../core/lv_obj.h"
#include "../../widgets/image/lv_image.h"
#include "../../widgets/label/lv_label.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_event_dsc_t * ori_event_dsc_ptr;
    lv_event_dsc_t ori_event_dsc;
    lv_event_dsc_t hook_event_dsc;
} obj_ctx_t;

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
    print_func("obj - object related commands\n");
}

static void obj_event_hook_cb(lv_event_t * e)
{
    obj_ctx_t * obj_ctx = lv_event_get_user_data(e);

    lv_event_code_t code = lv_event_get_code(e);
    void * target = lv_event_get_current_target(e);
    LV_LOG_USER("code: %d (%s), target: %p", code, lv_event_get_code_name(code), target);

    e->user_data = obj_ctx->ori_event_dsc.user_data;
    obj_ctx->ori_event_dsc.cb(e);
}

static void constructor_cb(void * ctx)
{
    obj_ctx_t * obj_ctx = ctx;
    obj_ctx->hook_event_dsc.cb = obj_event_hook_cb;
    obj_ctx->hook_event_dsc.user_data = ctx;
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static int style_value_color_cb(struct argparse * self, const struct argparse_option * option)
{
    LV_UNUSED(self);
    lv_style_value_t * value = (lv_style_value_t *)option->data;
    value->color = lv_color_hex(*(unsigned long *)option->value);
    return 0;
}

static void dump_event_dsc_info(const lv_event_dsc_t * dsc)
{
    if(!dsc) {
        return;
    }

    LV_LOG_USER("event_dsc: %p", (void *)dsc);
    LV_LOG_USER("  callback: %p", (void *)(lv_uintptr_t)dsc->cb);
    LV_LOG_USER("  user_data: %p", dsc->user_data);
    LV_LOG_USER("  filter: %08x", (int)dsc->filter);
}

static void dump_obj_info(lv_obj_t * obj)
{
    LV_LOG_USER("obj: %p", (void *)obj);
    LV_LOG_USER("  X: %d", (int)lv_obj_get_x(obj));
    LV_LOG_USER("  Y: %d", (int)lv_obj_get_y(obj));
    LV_LOG_USER("  W: %d", (int)lv_obj_get_width(obj));
    LV_LOG_USER("  H: %d", (int)lv_obj_get_height(obj));
    LV_LOG_USER("  R: %d", (int)lv_obj_get_style_radius(obj, 0));
    LV_LOG_USER("  flags: %08x", (int)obj->flags);
    LV_LOG_USER("  state: %04x", (int)lv_obj_get_state(obj));
    LV_LOG_USER("  class: %p", (void *)lv_obj_get_class(obj));

    uint32_t event_cnt = lv_obj_get_event_count(obj);
    for(uint32_t i = 0; i < event_cnt; i++) {
        const lv_event_dsc_t * dsc = lv_obj_get_event_dsc(obj, i);
        dump_event_dsc_info(dsc);
    }

    if(lv_obj_has_class(obj, &lv_image_class)) {
        const void * src = lv_image_get_src(obj);
        lv_image_src_t src_type = lv_image_src_get_type(src);
        switch(src_type) {
            case LV_IMAGE_SRC_FILE:
                LV_LOG_USER("image file path: %s", (const char *)src);
                break;
            case LV_IMAGE_SRC_VARIABLE:
                LV_LOG_USER("image variable: %p", src);
                break;
            case LV_IMAGE_SRC_SYMBOL:
                LV_LOG_TRACE("image symbol: %p", src);
                break;
            default:
                LV_LOG_WARN("unknown type");
                break;
        }

        return;
    }

    if(lv_obj_has_class(obj, &lv_label_class)) {
        const char * txt = lv_label_get_text(obj);
        LV_LOG_USER("label txt: %s", txt ? txt : "");
        return;
    }
}

static void obj_take_snapshot_to_file(lv_obj_t * obj, const char * file_path, lv_color_format_t cf)
{
#if LV_USE_SNAPSHOT
    if(!file_path) {
        LV_LOG_WARN("file path is not specified");
        return;
    }

    lv_draw_buf_t * buf = lv_snapshot_take(obj, cf);
    if(!buf) {
        LV_LOG_WARN("snapshot failed");
        return;
    }

    lv_result_t res = lv_draw_buf_save_to_file(buf, file_path);
    if(res != LV_RESULT_OK) {
        LV_LOG_WARN("snapshot save failed: %s", file_path);
    }

    lv_draw_buf_destroy(buf);
#else
    LV_UNUSED(obj);
    LV_UNUSED(file_path);
    LV_UNUSED(cf);
    LV_LOG_WARN("LV_USE_SNAPSHOT is not enabled");
#endif
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    obj_ctx_t * obj_ctx = ctx;

    lv_obj_t * obj = NULL;
    lv_obj_t * parent = lv_screen_active();
    int create_obj = 0;
    int del_obj = 0;
    int dump_tree = 0;
    int dump_info = 0;

    int add_state = 0;
    int remove_state = 0;
    unsigned long state = 0;

    int add_flag = 0;
    int remove_flag = 0;
    unsigned long flag = 0;

    int set_local_style = 0;
    int get_local_style = 0;
    int prop = LV_STYLE_PROP_INV;
    unsigned long selector = 0;

    lv_style_value_t style_value = { 0 };
    unsigned long color = 0;

    int invalidate_obj = 0;
    const char * snapshot_path = NULL;
    unsigned long snapshot_cf = LV_COLOR_FORMAT_ARGB8888;

    int hook_event = 0;
    lv_event_dsc_t * event_dsc = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_HEX(0, "obj", &obj, "object pointer", NULL, 0, 0),
        OPT_HEX(0, "parent", &parent, "parent object pointer", NULL, 0, 0),
        OPT_BOOLEAN(0, "create", &create_obj, "create object", NULL, 0, 0),
        OPT_BOOLEAN(0, "del", &del_obj, "delete object", NULL, 0, 0),

        OPT_BOOLEAN(0, "dump-tree", &dump_tree, "dump object tree", NULL, 0, 0),
        OPT_BOOLEAN(0, "dump", &dump_info, "dump object information", NULL, 0, 0),

        OPT_BOOLEAN(0, "add-state", &add_state, "add state to object", NULL, 0, 0),
        OPT_BOOLEAN(0, "remove-state", &remove_state, "remove object state", NULL, 0, 0),
        OPT_HEX(0, "state", &state, "object state (lv_state_t)", NULL, 0, 0),

        OPT_BOOLEAN(0, "add-flag", &add_flag, "add flag to object", NULL, 0, 0),
        OPT_BOOLEAN(0, "remove-flag", &remove_flag, "remove object flag", NULL, 0, 0),
        OPT_HEX(0, "flag", &flag, "object flag (lv_obj_flag_t)", NULL, 0, 0),

        OPT_BOOLEAN(0, "set-local-style", &set_local_style, "set local style for object", NULL, 0, 0),
        OPT_BOOLEAN(0, "get-local-style", &get_local_style, "get local style for object", NULL, 0, 0),
        OPT_INTEGER(0, "prop", &prop, "style property", NULL, 0, 0),
        OPT_HEX(0, "selector", &selector, "style selector", NULL, 0, 0),
        OPT_INTEGER(0, "style-value-num", &style_value.num, "style value (number)", NULL, 0, 0),
        OPT_HEX(0, "style-value-ptr", &style_value.ptr, "style value (pointer)", NULL, 0, 0),
        OPT_HEX(0, "style-value-color", &color, "style value (color)", style_value_color_cb, (intptr_t)&style_value, 0),

        OPT_BOOLEAN(0, "invalidate", &invalidate_obj, "invalidate object", NULL, 0, 0),

        OPT_STRING(0, "snapshot-path", &snapshot_path, "take snapshot to file", NULL, 0, 0),
        OPT_HEX(0, "snapshot-cf", &snapshot_cf, "snapshot color format", NULL, 0, 0),

        OPT_BOOLEAN(0, "hook-event", &hook_event, "hook obj event", NULL, 0, 0),
        OPT_HEX(0, "event-dsc", &event_dsc, "hook event descriptor pointer", NULL, 0, 0),

        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(dump_tree) {
        lv_obj_dump_tree(obj);
        return LV_RESULT_OK;
    }

    if(create_obj) {
        lv_obj_t * new_obj = lv_obj_create(parent);
        LV_LOG_USER("object created: %p", (void *)new_obj);
        return LV_RESULT_OK;
    }

    if(hook_event) {
        if(event_dsc) {
            if(obj_ctx->ori_event_dsc_ptr) {
                LV_LOG_WARN("event hook:%p already exists", (void *)obj_ctx->ori_event_dsc_ptr);
                return LV_RESULT_INVALID;
            }

            obj_ctx->hook_event_dsc.filter = event_dsc->filter;
            obj_ctx->ori_event_dsc = *event_dsc;
            obj_ctx->ori_event_dsc_ptr = event_dsc;
            LV_LOG_USER("event hook: %p added", (void *)event_dsc);
            dump_event_dsc_info(event_dsc);

            /* overwrite event descriptor */
            *event_dsc = obj_ctx->hook_event_dsc;
            return LV_RESULT_OK;
        }

        if(!obj_ctx->ori_event_dsc_ptr) {
            LV_LOG_WARN("event hook not exists");
            return LV_RESULT_INVALID;
        }

        /* restore event descriptor */
        *obj_ctx->ori_event_dsc_ptr = obj_ctx->ori_event_dsc;
        LV_LOG_USER("event hook: %p removed", (void *)obj_ctx->ori_event_dsc_ptr);
        obj_ctx->ori_event_dsc_ptr = NULL;
        return LV_RESULT_OK;
    }

    if(!obj) {
        LV_LOG_WARN("object pointer is not specified");
        return LV_RESULT_INVALID;
    }

    if(dump_info) {
        dump_obj_info(obj);
    }

    if(add_state) {
        lv_obj_add_state(obj, (lv_state_t)state);
        LV_LOG_USER("obj: %p, state added: %04x", (void *)obj, (int)state);
    }

    if(remove_state) {
        lv_obj_remove_state(obj, (lv_state_t)state);
        LV_LOG_USER("obj: %p, state removed: %04x", (void *)obj, (int)state);
    }

    if(add_flag) {
        lv_obj_add_flag(obj, (lv_obj_flag_t)flag);
        LV_LOG_USER("obj: %p, flag added: %08x", (void *)obj, (int)flag);
    }

    if(remove_flag) {
        lv_obj_remove_flag(obj, (lv_obj_flag_t)flag);
        LV_LOG_USER("obj: %p, flag removed: %08x", (void *)obj, (int)flag);
    }

    if(set_local_style || get_local_style) {
        if(prop <= LV_STYLE_PROP_INV) {
            LV_LOG_WARN("style property is not specified");
            return LV_RESULT_INVALID;
        }

        if(set_local_style) {
            lv_obj_set_local_style_prop(obj, (lv_style_prop_t)prop, style_value, (lv_style_selector_t)selector);
        }

        if(get_local_style) {
            lv_style_res_t res = lv_obj_get_local_style_prop(obj, (lv_style_prop_t)prop, &style_value,
                                                             (lv_style_selector_t)selector);
            if(res != LV_STYLE_RES_FOUND) {
                LV_LOG_WARN("style prop: %d get failed: %d", prop, (int)res);
                return LV_RESULT_INVALID;
            }
        }

        LV_LOG_USER("obj: %p, style prop: %d, selector: %08x, value: num: %d | ptr: %p | color: R%d G%d B%d",
                    (void *)obj,
                    prop,
                    (int)selector,
                    (int)style_value.num,
                    style_value.ptr,
                    style_value.color.red,
                    style_value.color.green,
                    style_value.color.blue);
    }

    if(invalidate_obj) {
        lv_obj_invalidate(obj);
        LV_LOG_USER("obj: %p, invalidated", (void *)obj);
    }

    if(snapshot_path) {
        LV_LOG_USER("taking snapshot to file: %s, cf: %02x", snapshot_path, (int)snapshot_cf);
        obj_take_snapshot_to_file(obj, snapshot_path, (lv_color_format_t)snapshot_cf);
    }

    if(del_obj) {
        LV_LOG_USER("object deleted: %p", (void *)obj);
        lv_obj_delete(obj);
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(obj, sizeof(obj_ctx_t))

#endif /*LV_USE_REMOTE_CTRL*/
