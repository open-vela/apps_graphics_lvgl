#if LV_BUILD_TEST
#include "lv_test_init.h"
#include "../unity/unity.h"
#include "lv_test_indev.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define HOR_RES 800
#define VER_RES 480

static void hal_init(void);
static void dummy_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p);

uint8_t* last_flushed_buf;
lv_indev_t* lv_test_mouse_indev = NULL;
lv_indev_t* lv_test_keypad_indev = NULL;
lv_indev_t* lv_test_encoder_indev = NULL;
lv_display_t* lv_test_disp = NULL;

void lv_test_init(void)
{
    lv_init();
    hal_init();
}

void lv_test_deinit(void)
{
    lv_mem_deinit();
    lv_deinit();
    lv_test_disp = NULL;
    lv_test_mouse_indev = NULL;
    lv_test_keypad_indev = NULL;
    lv_test_encoder_indev = NULL;
}

static void hal_init(void)
{
    static lv_color32_t test_fb[(HOR_RES + LV_DRAW_BUF_STRIDE_ALIGN - 1) * VER_RES + LV_DRAW_BUF_ALIGN];
    lv_memzero(test_fb, sizeof(test_fb));

    if (lv_test_disp == NULL) {
        lv_test_disp = lv_display_create(HOR_RES, VER_RES);
        lv_display_set_buffers(lv_test_disp, lv_draw_buf_align(test_fb, LV_COLOR_FORMAT_ARGB8888), NULL, HOR_RES * VER_RES * 4,
            LV_DISPLAY_RENDER_MODE_DIRECT);
        lv_display_set_flush_cb(lv_test_disp, dummy_flush_cb);
    }

    if (lv_test_mouse_indev == NULL) {
        lv_test_mouse_indev = lv_indev_create();
        lv_indev_set_type(lv_test_mouse_indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(lv_test_mouse_indev, lv_test_mouse_read_cb);
    }

    if (lv_test_keypad_indev == NULL) {
        lv_test_keypad_indev = lv_indev_create();
        lv_indev_set_type(lv_test_keypad_indev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(lv_test_keypad_indev, lv_test_keypad_read_cb);
    }

    if (lv_test_encoder_indev == NULL) {
        lv_test_encoder_indev = lv_indev_create();
        lv_indev_set_type(lv_test_encoder_indev, LV_INDEV_TYPE_ENCODER);
        lv_indev_set_read_cb(lv_test_encoder_indev, lv_test_encoder_read_cb);
    }
}

void hal_init_colorformat(lv_color_format_t colorformat)
{
    static lv_color32_t test_fb[(HOR_RES + LV_DRAW_BUF_STRIDE_ALIGN - 1) * VER_RES + LV_DRAW_BUF_ALIGN];
    lv_memzero(test_fb, sizeof(test_fb));

    if (lv_test_disp != NULL) {
        lv_display_set_buffers(lv_test_disp, lv_draw_buf_align(test_fb, colorformat), NULL, HOR_RES * VER_RES * 4,
            LV_DISPLAY_RENDER_MODE_DIRECT);
    }
}

static void dummy_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p)
{
    LV_UNUSED(area);
    LV_UNUSED(color_p);
    last_flushed_buf = color_p;
    lv_display_flush_ready(disp);
}

void lv_test_assert_fail(void)
{
    /*Handle error on test*/
}

#endif
