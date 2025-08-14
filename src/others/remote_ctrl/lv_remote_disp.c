/**
 * @file lv_remote_disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl_private.h"

#if LV_USE_REMOTE_CTRL

#include "../../libs/argparse/argparse.h"
#include "../../display/lv_display.h"
#include "../../display/lv_display_private.h"

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
    print_func("disp - control the display\n");
}

static void constructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void destructor_cb(void * ctx)
{
    LV_UNUSED(ctx);
}

static void dump_disp_info(lv_display_t * disp)
{
    LV_LOG_USER("display: %p", (void *)disp);
    LV_LOG_USER("  hor: %d", (int)lv_display_get_horizontal_resolution(disp));
    LV_LOG_USER("  ver: %d", (int)lv_display_get_vertical_resolution(disp));
    LV_LOG_USER("  cf: %d", (int)lv_display_get_color_format(disp));
    LV_LOG_USER("  dpi: %d", (int)lv_display_get_dpi(disp));
    LV_LOG_USER("  rotation: %d", (int)lv_display_get_rotation(disp));
    LV_LOG_USER("  timer: %p", (void *)lv_display_get_refr_timer(disp));

    for(uint32_t i = 0; i < disp->screen_cnt; i++) {
        LV_LOG_USER("  screens[%d]: %p", (int)i, (void *)disp->screens[i]);
    }

    LV_LOG_USER("  act_scr: %p", (void *)disp->act_scr);
    LV_LOG_USER("  prev_scr: %p", (void *)disp->prev_scr);
    LV_LOG_USER("  scr_to_load: %p", (void *)disp->scr_to_load);
    LV_LOG_USER("  bottom_layer: %p", (void *)disp->bottom_layer);
    LV_LOG_USER("  top_layer: %p", (void *)disp->top_layer);
    LV_LOG_USER("  sys_layer: %p", (void *)disp->sys_layer);

    LV_LOG_USER("  buf_1: %p", (void *)disp->buf_1);
    LV_LOG_USER("  buf_2: %p", (void *)disp->buf_2);
    LV_LOG_USER("  buf_act: %p", (void *)disp->buf_act);
}

static lv_result_t execute_cb(void * ctx, int argc, const char * argv[])
{
    LV_UNUSED(ctx);
    int rotation = -1;
    int dump_disp = 0;
    lv_display_t * disp = NULL;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_HEX('d', "disp", &disp, "set display by address", NULL, 0, 0),
        OPT_INTEGER('r', "rotation", &rotation, "set display rotation, options: 0, 90, 180, 270", NULL, 0, 0),
        OPT_BOOLEAN(0, "dump", &dump_disp, "dump display information", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    if(argparse_parse(&argparse, argc, argv) > 0) {
        LV_LOG_WARN("argparse failed");
        return LV_RESULT_INVALID;
    }

    if(dump_disp) {
        if(disp) {
            dump_disp_info(disp);
        }
        else {
            lv_display_t * d = lv_display_get_next(NULL);
            while(d) {
                dump_disp_info(d);
                d = lv_display_get_next(d);
            }
        }
    }

    if(rotation >= 0) {
        lv_display_rotation_t rot;
        switch(rotation) {
            case 0:
                rot = LV_DISPLAY_ROTATION_0;
                break;
            case 90:
                rot = LV_DISPLAY_ROTATION_90;
                break;
            case 180:
                rot = LV_DISPLAY_ROTATION_180;
                break;
            case 270:
                rot = LV_DISPLAY_ROTATION_270;
                break;
            default:
                LV_LOG_WARN("Invalid rotation value: %d", rotation);
                return LV_RESULT_INVALID;
        }

        lv_display_set_rotation(disp, rot);
        LV_LOG_USER("set display rotation: %d", rotation);
    }

    return LV_RESULT_OK;
}

LV_REMOTE_CTRL_CLASS_EXPORT(disp, 0)

#endif /*LV_USE_REMOTE_CTRL*/
