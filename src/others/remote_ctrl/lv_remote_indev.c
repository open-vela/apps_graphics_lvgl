/**
 * @file lv_remote_indev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../libs/argparse/argparse.h"
#include "../../indev/lv_indev.h"
#include "../../core/lv_obj.h"

/*********************
 *      DEFINES
 *********************/

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
    print_func("indev - control the indev\n");
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void indev_set_cursor(lv_indev_t * indev, int32_t size)
{
    if(!indev) indev = lv_indev_active();
    if(!indev) {
        LV_LOG_WARN("No active indev");
        return;
    }

    lv_obj_t * cursor_obj = lv_indev_get_cursor(indev);
    if(size <= 0) {
        if(cursor_obj) {
            LV_LOG_USER("remove cursor obj: %p", (void *)cursor_obj);
            lv_indev_set_cursor(indev, NULL);
            lv_obj_delete(cursor_obj);
        }
        return;
    }

    if(cursor_obj == NULL) {
        cursor_obj = lv_obj_create(lv_layer_sys());
        LV_LOG_USER("create cursor obj: %p", (void *)cursor_obj);
        lv_obj_remove_style_all(cursor_obj);
        lv_obj_set_style_radius(cursor_obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(cursor_obj, LV_OPA_50, 0);
        lv_obj_set_style_bg_color(cursor_obj, lv_color_black(), 0);
        lv_obj_set_style_border_width(cursor_obj, 2, 0);
        lv_obj_set_style_border_color(cursor_obj, lv_palette_main(LV_PALETTE_GREY), 0);
    }
    lv_obj_set_size(cursor_obj, size, size);
    lv_obj_set_style_translate_x(cursor_obj, -size / 2, 0);
    lv_obj_set_style_translate_y(cursor_obj, -size / 2, 0);
    lv_indev_set_cursor(indev, cursor_obj);
}

static void dump_indev_info(lv_indev_t * indev)
{
    LV_LOG_USER("indev: %p", (void *)indev);
    LV_LOG_USER("  type: %d", lv_indev_get_type(indev));
    LV_LOG_USER("  state: %d", lv_indev_get_state(indev));
    LV_LOG_USER("  group: %p", (void *)lv_indev_get_group(indev));
    LV_LOG_USER("  display: %p", (void *)lv_indev_get_display(indev));
    LV_LOG_USER("  act_obj: %p", (void *)lv_indev_get_active_obj());
    LV_LOG_USER("  timer: %p", (void *)lv_indev_get_read_timer(indev));
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    LV_UNUSED(ctx);
    int enable = -1;
    int cursor_size = -1;
    int dump_indev = 0;
    lv_indev_t * indev = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_HEX('i', "indev", &indev, "set indev by address", NULL, 0, 0),
        OPT_INTEGER(0, "enable", &enable, "enable or disable profiler", NULL, 0, 0),
        OPT_INTEGER(0, "cursor-size", &cursor_size, "set cursor size in pixels", NULL, 0, 0),
        OPT_BOOLEAN(0, "dump", &dump_indev, "dump indev information", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(dump_indev) {
        if(indev) {
            dump_indev_info(indev);
        }
        else {
            lv_indev_t * i = lv_indev_get_next(NULL);
            while(i) {
                dump_indev_info(i);
                i = lv_indev_get_next(i);
            }
        }
    }

    if(enable >= 0) {
        lv_indev_enable(indev, enable > 0 ? true : false);
    }

    if(cursor_size >= 0) {
        indev_set_cursor(indev, cursor_size);
        LV_LOG_USER("set cursor size: %d", cursor_size);
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(indev, 0)

#endif /*LV_USE_REMOTE_CTRL*/
