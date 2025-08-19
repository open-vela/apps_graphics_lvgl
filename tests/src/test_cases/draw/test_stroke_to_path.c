
#if LV_BUILD_TEST
#include "../lvgl.h"
#include "lv_test_helpers.h"
#include <string.h>

#include "unity/unity.h"

#if (LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE)

static void canvas_draw(const char * name, void (*draw_cb)(lv_layer_t *))
{
    LV_UNUSED(name);
    lv_obj_t * canvas = lv_canvas_create(lv_screen_active());
    lv_draw_buf_t * draw_buf = lv_draw_buf_create(640, 480, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    TEST_ASSERT_NOT_NULL(draw_buf);
    lv_canvas_set_draw_buf(canvas, draw_buf);
    lv_canvas_fill_bg(canvas, lv_color_make(0xff, 0xff, 0xff), 255);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    draw_cb(&layer);

    lv_canvas_finish_layer(canvas, &layer);

#ifndef NON_AMD64_BUILD
    char fn_buf[64];
    lv_snprintf(fn_buf, sizeof(fn_buf), "draw/stroke_to_path_%s.lp64.png", name);
    TEST_ASSERT_EQUAL_SCREENSHOT(fn_buf);
#else
    char fn_buf[64];
    lv_snprintf(fn_buf, sizeof(fn_buf), "draw/stroke_to_path_%s.lp32.png", name);
    TEST_ASSERT_EQUAL_SCREENSHOT(fn_buf);
#endif

    lv_image_cache_drop(draw_buf);
    lv_draw_buf_destroy(draw_buf);
    lv_obj_del(canvas);
}

static void draw_lines(lv_layer_t * layer)
{
    lv_vector_dsc_t * dsc = lv_vector_dsc_create(layer);

    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(dsc, lv_color_white());
    lv_vector_clear_area(dsc, &rect);

    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    lv_fpoint_t pts[] = {{30, 30}, {160, 160}};
    lv_vector_path_move_to(path, &pts[0]);
    lv_vector_path_line_to(path, &pts[1]);

    lv_vector_dsc_set_stroke_color(dsc, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_color(dsc, lv_color_make(0x00, 0x00, 0xff));

    lv_vector_path_t * path2 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    lv_vector_draw_dsc_t * current_dsc = lv_vector_dsc_get_current_dsc(dsc);
    lv_vector_stroke_dsc_t * td = &current_dsc->stroke_dsc;

    lv_vector_dsc_set_stroke_width(dsc, 5.0f);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 8.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_SQUARE);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 10.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_ROUND);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);

    lv_vector_dsc_translate(dsc, 40, 0);
    float dash[] = {16, 12};
    lv_vector_dsc_set_stroke_dash(dsc, dash, 2);
    lv_vector_dsc_set_stroke_width(dsc, 5.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_BUTT);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    lv_vector_dsc_translate(dsc, 40, 0);
    float dash1[] = {8, 20, 2, 12};
    lv_vector_dsc_set_stroke_dash(dsc, dash1, 4);
    lv_vector_dsc_set_stroke_width(dsc, 8.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_SQUARE);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    lv_vector_dsc_translate(dsc, 40, 0);
    float dash2[] = {16, 20, 10, 25};
    lv_vector_dsc_set_stroke_dash(dsc, dash2, 4);
    lv_vector_dsc_set_stroke_width(dsc, 10.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_ROUND);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    lv_vector_dsc_identity(dsc);
    lv_vector_dsc_translate(dsc, 0, 180);
    lv_vector_path_clear(path);

    lv_fpoint_t pts2[] = {{120, 30}, {30, 160}, {80, 190}};
    lv_vector_path_move_to(path, &pts2[0]);
    lv_vector_path_line_to(path, &pts2[1]);
    lv_vector_path_line_to(path, &pts2[2]);

    lv_vector_dsc_set_stroke_color(dsc, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_color(dsc, lv_color_make(0xff, 0x00, 0x00));

    lv_vector_dsc_set_stroke_width(dsc, 10.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_BUTT);
    lv_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_BEVEL);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 8.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_SQUARE);
    lv_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_MITER);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 10.0f);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_ROUND);
    lv_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_ROUND);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 5.0f);
    lv_vector_dsc_set_stroke_dash(dsc, dash, 2);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_BUTT);
    lv_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_BEVEL);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 8.0f);
    lv_vector_dsc_set_stroke_dash(dsc, dash1, 4);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_SQUARE);
    lv_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_MITER);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    lv_vector_dsc_translate(dsc, 40, 0);
    lv_vector_dsc_set_stroke_width(dsc, 10.0f);
    lv_vector_dsc_set_stroke_dash(dsc, dash2, 4);
    lv_vector_dsc_set_stroke_cap(dsc, LV_VECTOR_STROKE_CAP_ROUND);
    lv_vector_dsc_set_stroke_join(dsc, LV_VECTOR_STROKE_JOIN_ROUND);
    lv_vector_stroke_to_path(path2, path, td);
    lv_vector_dsc_add_path(dsc, path2);
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    lv_draw_vector(dsc);
    lv_vector_path_delete(path);
    lv_vector_path_delete(path2);
    lv_vector_dsc_delete(dsc);
}

void test_stroke_to_path(void)
{
    canvas_draw("draw_lines", draw_lines);
}

#else

void test_stroke_to_path(void)
{
    ;
}
#endif
#endif
