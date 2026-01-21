#if LV_BUILD_TEST
#include "../lvgl.h"
#include "lv_test_helpers.h"
#include <string.h>

#include "unity/unity.h"

static lv_layer_t layer;
static lv_obj_t * canvas;
static lv_draw_buf_t * canvas_buf;

void setUp(void)
{
    canvas = lv_canvas_create(lv_scr_act());
    canvas_buf = lv_draw_buf_create(480, 480, LV_COLOR_FORMAT_ARGB8888, 0);
    TEST_ASSERT_NOT_NULL(canvas_buf);
    lv_canvas_set_draw_buf(canvas, canvas_buf);
    lv_canvas_fill_bg(canvas, lv_color_make(0xff, 0xff, 0xff), 255);
    lv_canvas_init_layer(canvas, &layer);
}

void tearDown(void)
{
    if(canvas_buf) {
        lv_image_cache_drop(canvas_buf);
        lv_draw_buf_destroy(canvas_buf);
        canvas_buf = NULL;
    }
    if(canvas) {
        lv_obj_del(canvas);
        canvas = NULL;
    }
}

#if LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE

#define SNAPSHOT_NAME(n) (#n)

#ifndef NON_AMD64_BUILD
    #define EXT_NAME ".lp64.png"
#else
    #define EXT_NAME ".lp32.png"
#endif

static void draw_snapshot(const char * name)
{
    char fn_buf[64];
    lv_snprintf(fn_buf, sizeof(fn_buf), "draw/vector_draw_%s" EXT_NAME, name);
    TEST_ASSERT_EQUAL_SCREENSHOT(fn_buf);
}

static void draw_vector(lv_vector_dsc_t * ctx)
{
    lv_image_cache_drop(canvas_buf);
    lv_canvas_set_draw_buf(canvas, canvas_buf);
    lv_canvas_fill_bg(canvas, lv_color_make(0xff, 0xff, 0xff), 255);
    lv_draw_vector(ctx);
    lv_canvas_finish_layer(canvas, &layer);
}

void test_draw_lines_group(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Test 1: Basic line Different line widths Dashed line
    lv_vector_path_clear(path);
    lv_fpoint_t pts1[] = {{50, 50}, {400, 50}};
    lv_vector_path_move_to(path, &pts1[0]);
    lv_vector_path_line_to(path, &pts1[1]);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_black());
    lv_vector_dsc_set_stroke_width(ctx, 1.0f);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path);

    lv_vector_path_clear(path);
    lv_fpoint_t pts2[] = {{50, 80}, {400, 80}};
    lv_vector_path_move_to(path, &pts2[0]);
    lv_vector_path_line_to(path, &pts2[1]);
    lv_vector_dsc_set_stroke_width(ctx, 5.0f);
    lv_vector_dsc_add_path(ctx, path);

    lv_vector_path_clear(path);
    lv_fpoint_t pts3[] = {{50, 120}, {400, 120}};
    lv_vector_path_move_to(path, &pts3[0]);
    lv_vector_path_line_to(path, &pts3[1]);
    float dashes[] = {10, 5};
    lv_vector_dsc_set_stroke_dash(ctx, dashes, 2);
    lv_vector_dsc_add_path(ctx, path);

    /* Test dash_count=3 */
    lv_vector_path_clear(path);
    lv_fpoint_t pts7[] = {{50, 140}, {400, 140}};
    lv_vector_path_move_to(path, &pts7[0]);
    lv_vector_path_line_to(path, &pts7[1]);
    float dashes3[] = {10, 5, 5};
    lv_vector_dsc_set_stroke_dash(ctx, dashes3, 3);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_black());
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(line_base));

    // Test 2: Line caps
    lv_vector_path_clear(path);
    lv_fpoint_t pts4[] = {{50, 100}, {400, 100}};
    lv_vector_path_move_to(path, &pts4[0]);
    lv_vector_path_line_to(path, &pts4[1]);
    lv_vector_dsc_set_stroke_width(ctx, 10.0f);
    lv_vector_dsc_set_stroke_cap(ctx, LV_VECTOR_STROKE_CAP_BUTT);
    lv_vector_dsc_set_stroke_dash(ctx, NULL, 0);
    lv_vector_dsc_add_path(ctx, path);

    lv_vector_path_clear(path);
    lv_fpoint_t pts5[] = {{50, 200}, {400, 200}};
    lv_vector_path_move_to(path, &pts5[0]);
    lv_vector_path_line_to(path, &pts5[1]);
    lv_vector_dsc_set_stroke_cap(ctx, LV_VECTOR_STROKE_CAP_SQUARE);
    lv_vector_dsc_set_stroke_dash(ctx, NULL, 0);
    lv_vector_dsc_add_path(ctx, path);

    lv_vector_path_clear(path);
    lv_fpoint_t pts6[] = {{50, 300}, {400, 300}};
    lv_vector_path_move_to(path, &pts6[0]);
    lv_vector_path_line_to(path, &pts6[1]);
    lv_vector_dsc_set_stroke_cap(ctx, LV_VECTOR_STROKE_CAP_ROUND);
    lv_vector_dsc_set_stroke_dash(ctx, NULL, 0);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(line_caps));

    // Test 3: Line joins
    lv_vector_path_clear(path);
    lv_vector_dsc_set_stroke_width(ctx, 10.0f);

    // Miter join
    lv_fpoint_t miter_pts[] = {{50, 200}, {100, 150}, {150, 250}, {200, 200}};
    lv_vector_path_move_to(path, &miter_pts[0]);
    lv_vector_path_line_to(path, &miter_pts[1]);
    lv_vector_path_line_to(path, &miter_pts[2]);
    lv_vector_path_line_to(path, &miter_pts[3]);
    lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_MITER);
    lv_vector_dsc_add_path(ctx, path);

    // Round join
    lv_vector_path_clear(path);
    lv_vector_dsc_set_stroke_width(ctx, 10.0f);

    lv_fpoint_t round_pts[] = {{50, 300}, {100, 250}, {150, 350}, {200, 300}};
    lv_vector_path_move_to(path, &round_pts[0]);
    lv_vector_path_line_to(path, &round_pts[1]);
    lv_vector_path_line_to(path, &round_pts[2]);
    lv_vector_path_line_to(path, &round_pts[3]);
    lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_ROUND);
    lv_vector_dsc_add_path(ctx, path);

    // Bevel join
    lv_vector_path_clear(path);
    lv_vector_dsc_set_stroke_width(ctx, 10.0f);

    lv_fpoint_t bevel_pts[] = {{50, 400}, {100, 350}, {150, 450}, {200, 400}};
    lv_vector_path_move_to(path, &bevel_pts[0]);
    lv_vector_path_line_to(path, &bevel_pts[1]);
    lv_vector_path_line_to(path, &bevel_pts[2]);
    lv_vector_path_line_to(path, &bevel_pts[3]);
    lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_BEVEL);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(line_joins));

    // Test 4: Miter limit comparison
    lv_vector_path_clear(path);
    lv_vector_dsc_identity(ctx);

    lv_fpoint_t miter_pts1[] = {{50, 200}, {100, 250}, {50, 300}};

    // Default miter limit (4.0)
    lv_vector_path_move_to(path, &miter_pts1[0]);
    lv_vector_path_line_to(path, &miter_pts1[1]);
    lv_vector_path_line_to(path, &miter_pts1[2]);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0xff, 0x00, 0x00)); // Red
    lv_vector_dsc_set_stroke_width(ctx, 10.0f);
    lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_MITER);
    lv_vector_dsc_set_stroke_miter_limit(ctx, 4.0);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_add_path(ctx, path);

    // Small miter limit (1.0)
    lv_vector_path_clear(path);
    lv_fpoint_t miter_pts2[] = {{150, 200}, {200, 250}, {150, 300}};
    lv_vector_path_move_to(path, &miter_pts2[0]);
    lv_vector_path_line_to(path, &miter_pts2[1]);
    lv_vector_path_line_to(path, &miter_pts2[2]);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0xff, 0x00)); // Green
    lv_vector_dsc_set_stroke_miter_limit(ctx, 1.0);
    lv_vector_dsc_add_path(ctx, path);

    // Large miter limit (10.0)
    lv_vector_path_clear(path);
    lv_fpoint_t miter_pts3[] = {{250, 200}, {300, 250}, {250, 300}};
    lv_vector_path_move_to(path, &miter_pts3[0]);
    lv_vector_path_line_to(path, &miter_pts3[1]);
    lv_vector_path_line_to(path, &miter_pts3[2]);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0x00, 0xff)); // Blue
    lv_vector_dsc_set_stroke_miter_limit(ctx, 10.0);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(line_miter_limit));

    // Test 5: Bezier
    // Test : Cubic Bezier curve
    lv_vector_dsc_identity(ctx);
    lv_vector_path_clear(path);

    lv_fpoint_t start = {50, 200};
    lv_fpoint_t ctrl1 = {100, 150};
    lv_fpoint_t ctrl2 = {150, 250};
    lv_fpoint_t end = {200, 200};
    lv_vector_path_move_to(path, &start);
    lv_vector_path_cubic_to(path, &ctrl1, &ctrl2, &end);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0x00, 0xff));
    lv_vector_dsc_set_stroke_width(ctx, 3.0f);
    lv_vector_dsc_add_path(ctx, path);

    // Test : Quadratic Bezier curve
    lv_vector_path_clear(path);
    lv_fpoint_t start2 = {50, 50};
    lv_fpoint_t control = {150, 50};
    lv_fpoint_t end2 = {150, 150};
    lv_vector_path_move_to(path, &start2);
    lv_vector_path_quad_to(path, &control, &end2);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0x80, 0x80));
    lv_vector_dsc_set_stroke_width(ctx, 3.0f);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(line_bezier));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_fill_stroke(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Test 1: Fill only
    lv_vector_path_clear(path);
    lv_area_t rect1 = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &rect1, 0, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path);

    // Test 2: Stroke only
    lv_vector_path_clear(path);
    lv_area_t rect2 = {200, 50, 300, 150};
    lv_vector_path_append_rect(path, &rect2, 0, 0);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0x00, 0xff));
    lv_vector_dsc_set_stroke_width(ctx, 5.0f);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_add_path(ctx, path);

    // Test 3: Fill and stroke
    lv_vector_path_clear(path);
    lv_area_t rect3 = {350, 50, 450, 150};
    lv_vector_path_append_rect(path, &rect3, 0, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0xff, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_add_path(ctx, path);

    // Test 4: Empty stroke path added to draw queue
    // Create a new empty path, set stroke-only, and add it to the draw queue.
    // This should not crash and should render nothing.
    lv_vector_path_clear(path);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0x00, 0x00));
    lv_vector_dsc_set_stroke_width(ctx, 5.0f);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(fill_and_stroke_base));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_stroke_gradients(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Common gradient stops
    lv_gradient_stop_t stops[2] = {
        {.color = lv_color_hex(0xFF0000), .opa = LV_OPA_COVER, .frac = 0},
        {.color = lv_color_hex(0x0000FF), .opa = LV_OPA_COVER, .frac = 255}
    };

    /* Test linear gradient with all spread types in one snapshot */
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);

    // Left: PAD spread
    lv_area_t linear_rect1 = {50, 50, 100, 150};
    lv_vector_path_append_rect(path, &linear_rect1, 10, 10);
    lv_vector_dsc_set_stroke_linear_gradient(ctx, 50, 50, 100, 100);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_stroke_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_PAD);
    lv_vector_dsc_set_stroke_width(ctx, 30.0f);
    lv_vector_dsc_add_path(ctx, path);

    // Middle: REPEAT spread
    lv_vector_path_clear(path);
    lv_area_t linear_rect2 = {160, 50, 210, 150};
    lv_vector_path_append_rect(path, &linear_rect2, 10, 10);
    lv_vector_dsc_set_stroke_linear_gradient(ctx, 160, 50, 210, 100);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_stroke_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_REPEAT);
    lv_vector_dsc_add_path(ctx, path);

    // Right: REFLECT spread
    lv_vector_path_clear(path);
    lv_area_t linear_rect3 = {270, 50, 320, 150};
    lv_vector_path_append_rect(path, &linear_rect3, 10, 10);
    lv_vector_dsc_set_stroke_linear_gradient(ctx, 270, 50, 320, 100);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_stroke_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_REFLECT);
    lv_vector_dsc_add_path(ctx, path);

    /* Test radial gradient with all spread types in one snapshot */
    // Clear previous drawing
    lv_vector_dsc_identity(ctx);
    lv_vector_path_clear(path);

    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    // Top: PAD spread
    lv_area_t radial_rect1 = {50, 200, 100, 250};
    lv_vector_path_append_rect(path, &radial_rect1, 10, 10);
    lv_vector_dsc_set_stroke_radial_gradient(ctx, 100, 250, 40);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_stroke_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_PAD);
    lv_vector_dsc_set_stroke_width(ctx, 30.0f);
    lv_vector_dsc_add_path(ctx, path);

    // Middle: REPEAT spread
    lv_vector_path_clear(path);
    lv_area_t radial_rect2 = {160, 200, 210, 250};
    lv_vector_path_append_rect(path, &radial_rect2, 10, 10);
    lv_vector_dsc_set_stroke_radial_gradient(ctx, 210, 250, 40);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_stroke_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_REPEAT);
    lv_vector_dsc_add_path(ctx, path);

    // Bottom: REFLECT spread
    lv_vector_path_clear(path);
    lv_area_t radial_rect3 = {270, 200, 320, 250};
    lv_vector_path_append_rect(path, &radial_rect3, 10, 10);
    lv_vector_dsc_set_stroke_radial_gradient(ctx, 320, 250, 40);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_stroke_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_REFLECT);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(stroke_gradients));

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_fill_gradients(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Test 1: Linear gradient
    lv_vector_path_clear(path);
    lv_area_t rect1 = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &rect1, 0, 0);

    lv_gradient_stop_t stops[2];
    stops[0].color = lv_color_hex(0xff0000);
    stops[0].opa = LV_OPA_COVER;
    stops[0].frac = 0;
    stops[1].color = lv_color_hex(0x0000ff);
    stops[1].opa = LV_OPA_COVER;
    stops[1].frac = 255;

    lv_vector_dsc_set_fill_linear_gradient(ctx, 50, 50, 150, 150);
    lv_vector_dsc_set_fill_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_set_fill_gradient_spread(ctx, LV_VECTOR_GRADIENT_SPREAD_PAD);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path);

    // Test 2: Radial gradient
    lv_vector_path_clear(path);
    lv_area_t rect2 = {200, 50, 300, 150};
    lv_vector_path_append_rect(path, &rect2, 0, 0);

    stops[0].color = lv_color_hex(0x00ff00);
    stops[1].color = lv_color_hex(0x0000ff);

    lv_vector_dsc_set_fill_radial_gradient(ctx, 250, 100, 50);
    lv_vector_dsc_set_fill_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_add_path(ctx, path);

    // Test 3: Gradient with transparency
    lv_vector_path_clear(path);
    lv_area_t rect3 = {350, 50, 450, 150};
    lv_vector_path_append_rect(path, &rect3, 0, 0);

    stops[0].color = lv_color_hex(0xff0000);
    stops[0].opa = LV_OPA_50;
    stops[1].color = lv_color_hex(0x0000ff);
    stops[1].opa = LV_OPA_50;

    lv_vector_dsc_set_fill_linear_gradient(ctx, 350, 50, 450, 150);
    lv_vector_dsc_set_fill_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(fill_gradient));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_fill_patterns(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Test 1: Basic pattern
    lv_vector_path_clear(path);
    lv_area_t rect1 = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &rect1, 0, 0);

    // Create checkerboard pattern
    lv_draw_image_dsc_t img_dsc;
    lv_draw_image_dsc_init(&img_dsc);
    LV_IMAGE_DECLARE(test_image_cogwheel_argb8888);
    img_dsc.header = test_image_cogwheel_argb8888.header;
    img_dsc.src = &test_image_cogwheel_argb8888;

    lv_vector_dsc_set_fill_image(ctx, &img_dsc);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path);

    // Test 2: Image fill with transform
    lv_vector_path_clear(path);
    lv_area_t rect2 = {200, 50, 300, 150};
    lv_vector_path_append_rect(path, &rect2, 0, 0);

    lv_matrix_t mt;
    lv_matrix_identity(&mt);
    lv_matrix_scale(&mt, 0.5f, 0.5f);
    lv_vector_dsc_set_fill_transform(ctx, &mt);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(fill_pattern_image));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_fill_units(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_draw_image_dsc_t img_dsc;
    lv_draw_image_dsc_init(&img_dsc);
    LV_IMAGE_DECLARE(test_image_cogwheel_argb8888);

    /* Test USERSPACE units */
    lv_vector_dsc_set_fill_units(ctx, LV_VECTOR_FILL_UNITS_USER_SPACE_ON_USE);

    /* Draw with USERSPACE units */
    lv_area_t rect1 = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &rect1, 0, 0);
    img_dsc.header = test_image_cogwheel_argb8888.header;
    img_dsc.src = &test_image_cogwheel_argb8888;
    lv_vector_dsc_set_fill_image(ctx, &img_dsc);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_add_path(ctx, path);

    /* Test BOUNDING_BOX units */
    lv_vector_path_clear(path);
    lv_vector_dsc_set_fill_units(ctx, LV_VECTOR_FILL_UNITS_OBJECT_BOUNDING_BOX);

    /* Draw with BOUNDING_BOX units */
    lv_area_t rect2 = {200, 50, 300, 150};
    lv_vector_path_append_rect(path, &rect2, 0, 0);
    lv_vector_dsc_set_fill_image(ctx, &img_dsc);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(fill_units));

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_fill_rounded_rect(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    /* Test 32-bit color setting */

    lv_vector_dsc_set_fill_color32(ctx, lv_color32_make(0x00, 0xff, 0x00, 0xff)); // RGBA format
    lv_vector_dsc_set_stroke_color32(ctx, lv_color32_make(0xff, 0x00, 0x00, 0xff));

    /* Test stroke transform */
    lv_matrix_t stroke_xform;
    lv_matrix_identity(&stroke_xform);
    lv_matrix_rotate(&stroke_xform, 45.0f);
    lv_vector_dsc_set_stroke_transform(ctx, &stroke_xform);

    /* Test path operations */
    lv_area_t rect = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &rect, 10, 10);
    lv_vector_path_is_empty(path);

    /* Draw and snapshot */
    lv_vector_dsc_set_stroke_width(ctx, 3.0f);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(fill_rounded_rect));

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_shapes_group(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Test 1: Rectangle
    lv_vector_path_clear(path);
    lv_area_t rect1 = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &rect1, 0, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path);

    // Test 2: Rounded rectangle
    lv_vector_path_clear(path);
    lv_area_t rect2 = {200, 50, 300, 150};
    lv_vector_path_append_rect(path, &rect2, 20, 20);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, path);

    // Test 3: Circle
    lv_vector_path_clear(path);
    lv_fpoint_t center = {100, 250};
    lv_vector_path_append_circle(path, &center, 50, 50);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0x00, 0xff));
    lv_vector_dsc_add_path(ctx, path);

    // Test 3: polygon
    lv_fpoint_t polygon[] = {
        {200, 200},
        {300, 200},
        {350, 250},
        {300, 300},
        {200, 300}
    };

    lv_vector_path_clear(path);
    lv_vector_path_move_to(path, &polygon[0]);
    for(int i = 1; i < 5; i++) {
        lv_vector_path_line_to(path, &polygon[i]);
    }
    lv_vector_path_close(path);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_black());
    lv_vector_dsc_set_stroke_width(ctx, 2.0f);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(shape_group));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_combined_shapes(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_t * path1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_t * path2 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Create first path (rectangle)
    lv_area_t rect1 = {50, 50, 150, 150};
    lv_vector_path_append_rect(path1, &rect1, 0, 0);

    // Create second path (circle)
    lv_fpoint_t center = {150, 150};
    lv_vector_path_append_circle(path2, &center, 100, 100);

    // Combine paths
    lv_vector_path_append_path(path1, path2);

    // Draw combined path
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0x80, 0x80));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path1);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(combined_paths));

    lv_vector_path_delete(path1);
    lv_vector_path_delete(path2);

    // Test 3: Arc
    lv_vector_path_clear(path);
    lv_fpoint_t center1 = {200, 150};
    lv_vector_path_append_arc(path, &center1, 50, 0, 270, false);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0xff, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_black());
    lv_vector_dsc_add_path(ctx, path);

    // Test 4: Combined path
    lv_vector_path_clear(path);
    lv_area_t rect2 = {350, 50, 450, 150};
    lv_vector_path_append_rect(path, &rect2, 0, 0);
    lv_fpoint_t circle_center = {400, 200};
    lv_vector_path_append_circle(path, &circle_center, 50, 50);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0xff, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_fill_rule(ctx, LV_VECTOR_FILL_EVENODD);
    lv_vector_dsc_add_path(ctx, path);

    /* Test pie chart (arc with pie=true) */
    lv_vector_path_clear(path);
    lv_fpoint_t pie_center = {80, 350};
    lv_vector_path_append_arc(path, &pie_center, 50, 30, 120, true);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_add_path(ctx, path);

    /* Test full circle pie (360 degrees) */
    lv_vector_path_clear(path);
    lv_fpoint_t pie_center2 = {200, 350};
    lv_vector_path_append_arc(path, &pie_center2, 60, 0, 360, true);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, path);

    /* Test combined rectangle and pie */
    lv_vector_path_clear(path);
    lv_area_t combined_rect = {350, 300, 450, 400};
    lv_vector_path_append_rect(path, &combined_rect, 0, 0);
    lv_fpoint_t combined_pie_center = {400, 350};
    lv_vector_path_append_arc(path, &combined_pie_center, 30, 45, 270, true);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0x00, 0xff));
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(shape_combined_group));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_complex_shapes(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Test : Combined paths with different fill rules
    lv_vector_path_clear(path);
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0x00, 0x80, 0x80));
    lv_vector_dsc_set_stroke_width(ctx, 3.0f);
    lv_area_t rect1 = {200, 50, 300, 150};
    lv_vector_path_append_rect(path, &rect1, 0, 0);
    lv_fpoint_t circle_center = {250, 150};
    lv_vector_path_append_circle(path, &circle_center, 50, 50);

    // Test even-odd rule
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x80, 0x00, 0x80));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_fill_rule(ctx, LV_VECTOR_FILL_EVENODD);
    lv_vector_dsc_add_path(ctx, path);

    // Test non-zero rule
    lv_vector_dsc_translate(ctx, 0, 200);
    lv_vector_dsc_set_fill_rule(ctx, LV_VECTOR_FILL_NONZERO);
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(fill_rule));

    // Test 5: Blend modes comparison
    lv_vector_path_clear(path);
    lv_vector_dsc_identity(ctx);

    // Base rectangle
    lv_area_t blend_rect1 = {100, 150, 200, 250};
    lv_vector_path_append_rect(path, &blend_rect1, 0, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_blend_mode(ctx, LV_VECTOR_BLEND_MULTIPLY);
    lv_vector_dsc_add_path(ctx, path);

    // SCREEN mode
    lv_vector_path_clear(path);
    lv_area_t blend_rect2 = {250, 150, 350, 250};
    lv_vector_path_append_rect(path, &blend_rect2, 0, 0);
    lv_vector_dsc_set_blend_mode(ctx, LV_VECTOR_BLEND_SCREEN);
    lv_vector_dsc_add_path(ctx, path);

    lv_vector_path_clear(path);
    lv_area_t blend_rect3 = {300, 150, 400, 250};
    lv_vector_path_append_rect(path, &blend_rect3, 0, 0);
    lv_vector_dsc_set_blend_mode(ctx, LV_VECTOR_BLEND_ADDITIVE);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(blend_mode));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_transforms(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Base shape (rectangle)
    lv_area_t rect1 = {50, 50, 100, 100};
    lv_vector_path_append_rect(path, &rect1, 0, 0);

    // Original shape
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_transform(ctx, &matrix);
    lv_vector_dsc_add_path(ctx, path);

    // Translated
    lv_matrix_t mt;
    lv_matrix_identity(&mt);
    lv_matrix_translate(&mt, 150, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0xff, 0x00));
    lv_vector_dsc_set_transform(ctx, &mt);
    lv_vector_dsc_add_path(ctx, path);

    // Rotated
    lv_matrix_identity(&mt);
    lv_matrix_translate(&mt, 300, 0);
    lv_matrix_rotate(&mt, 45);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0x00, 0xff));
    lv_vector_dsc_set_transform(ctx, &mt);
    lv_vector_dsc_add_path(ctx, path);

    // Scaled
    lv_matrix_identity(&mt);
    lv_matrix_translate(&mt, 0, 150);
    lv_matrix_scale(&mt, 1.5f, 0.5f);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0xff, 0x00));
    lv_vector_dsc_set_transform(ctx, &mt);
    lv_vector_dsc_add_path(ctx, path);

    // Combined transforms
    lv_matrix_identity(&mt);
    lv_matrix_translate(&mt, 150, 150);
    lv_matrix_rotate(&mt, 30);
    lv_matrix_scale(&mt, 1.2f, 1.2f);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0xff));
    lv_vector_dsc_set_transform(ctx, &mt);
    lv_vector_dsc_add_path(ctx, path);

    /* Verify bounds */
    lv_area_t bounds;
    lv_vector_path_get_bounding(path, &bounds);

    // Draw all transforms in one snapshot
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(transform_group));

    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_matrix_operations(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    // Clear background
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    // Base rectangle
    lv_area_t base_rect = {50, 50, 150, 150};
    lv_vector_path_append_rect(path, &base_rect, 0, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0x80, 0x80));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_add_path(ctx, path);

    // Scale transform
    lv_vector_path_clear(path);
    lv_vector_path_append_rect(path, &base_rect, 0, 0);
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_scale(ctx, 1.5f, 0.8f);
    lv_vector_dsc_translate(ctx, 200, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0x00));
    lv_vector_dsc_add_path(ctx, path);

    // Rotate transform
    lv_vector_path_clear(path);
    lv_vector_path_append_rect(path, &base_rect, 0, 0);
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_rotate(ctx, 45.0f);
    lv_vector_dsc_translate(ctx, 400, 0);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, path);

    // Translate transform
    lv_vector_path_clear(path);
    lv_vector_path_append_rect(path, &base_rect, 0, 0);
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_translate(ctx, 200, 200);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0x00, 0x00, 0xff));
    lv_vector_dsc_add_path(ctx, path);

    // Skew transform
    lv_vector_path_clear(path);
    lv_vector_path_append_rect(path, &base_rect, 0, 0);
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_skew(ctx, 15.0f, 10.0f);
    lv_vector_dsc_translate(ctx, 400, 200);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0x00, 0xff));
    lv_vector_dsc_add_path(ctx, path);

    // Combined transforms
    lv_vector_path_clear(path);
    lv_vector_path_append_rect(path, &base_rect, 0, 0);
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_translate(ctx, 300, 300);
    lv_vector_dsc_rotate(ctx, 30.0f);
    lv_vector_dsc_scale(ctx, 1.2f, 0.8f);
    lv_vector_dsc_skew(ctx, 10.0f, 5.0f);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, path);
    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(matrix));

    // Cleanup
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_error_handling_null_params(void)
{
    /* Create valid objects for further testing */
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_dsc_t * dsc = lv_vector_dsc_create(&layer);

    TEST_ASSERT_TRUE(lv_vector_path_is_empty(path));

    /* Test invalid path operations */
    lv_fpoint_t point = {0, 0};
    lv_area_t rect = {0, 0, 100, 100};

    lv_vector_path_line_to(path, &point);
    lv_vector_path_quad_to(path, &point, &point);
    lv_vector_path_cubic_to(path, &point, &point, &point);
    lv_vector_path_close(path);
    lv_vector_path_append_rect(path, &rect, 0, 0);
    lv_vector_path_append_circle(path, &point, 20, 20);

    /* Test invalid dsc operations */
    lv_vector_dsc_set_stroke_dash(dsc, NULL, 0);

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(dsc);
}

void test_error_handling_invalid_values(void)
{
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_dsc_t * dsc = lv_vector_dsc_create(&layer);

    /* Test invalid stroke width */
    lv_vector_dsc_set_stroke_width(dsc, -1.0f);
    lv_vector_dsc_set_stroke_width(dsc, 1000.0f);

    /* Test invalid dash pattern */

    float invalid_dash[] = {-1.0f, 0.0f};
    lv_vector_dsc_set_stroke_dash(dsc, invalid_dash, 2);

    /* Test invalid path points */

    lv_vector_path_move_to(path, &(lv_fpoint_t) {
        NAN, NAN
    });
    lv_vector_path_line_to(path, &(lv_fpoint_t) {
        INFINITY, INFINITY
    });

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(dsc);
}

void test_draw_fill_gradient_invalid_args(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    lv_gradient_stop_t stops[2] = {
        {.color = lv_color_hex(0xFF0000), .opa = LV_OPA_COVER, .frac = 0},
        {.color = lv_color_hex(0x0000FF), .opa = LV_OPA_COVER, .frac = 255}
    };

    lv_vector_dsc_identity(ctx);
    lv_vector_path_clear(path);
    lv_area_t linear_rect = {50, 50, 100, 150};
    lv_vector_path_append_rect(path, &linear_rect, 10, 10);

    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_50);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_TRANSP);

    /* Test start and end points are the same point */
    lv_vector_dsc_set_fill_linear_gradient(ctx, 50, 50, 50, 50);
    lv_vector_dsc_set_fill_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_add_path(ctx, path);

    /* Test gradient Radial is less than 0 */
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_translate(ctx, 100, 0);
    lv_vector_dsc_set_fill_radial_gradient(ctx, 250, 100, -50);
    lv_vector_dsc_set_fill_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot("fill_gradient_invalid_args");

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_stroke_gradient_invalid_args(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    lv_gradient_stop_t stops[2] = {
        {.color = lv_color_hex(0xFF0000), .opa = LV_OPA_COVER, .frac = 0},
        {.color = lv_color_hex(0x0000FF), .opa = LV_OPA_COVER, .frac = 255}
    };

    lv_vector_dsc_identity(ctx);
    lv_vector_path_clear(path);
    lv_area_t linear_rect = {50, 50, 100, 150};
    lv_vector_path_append_rect(path, &linear_rect, 10, 10);

    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_50);
    lv_vector_dsc_set_stroke_width(ctx, 10);

    /* Test linear gradient with start and end points are the same point */
    lv_vector_dsc_set_stroke_linear_gradient(ctx, 50, 50, 50, 50);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_add_path(ctx, path);

    /* Test radial gradient radius is less than 0 */
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_translate(ctx, 100, 0);
    lv_vector_dsc_set_stroke_radial_gradient(ctx, 250, 100, -50);
    lv_vector_dsc_set_stroke_gradient_color_stops(ctx, stops, 2);
    lv_vector_dsc_add_path(ctx, path);

    /* Test invalid stroke width */
    lv_vector_dsc_identity(ctx);
    lv_vector_dsc_translate(ctx, 200, 0);
    lv_vector_dsc_set_stroke_width(ctx, -10);
    lv_vector_dsc_add_path(ctx, path);

    draw_vector(ctx);
    draw_snapshot("stroke_gradient_invalid_args");

    /* Cleanup */
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

void test_draw_clipper_operations(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_t * path2 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_t * result = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    /* Clear canvas */
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    /* Create test paths with smaller size */
    lv_fpoint_t rect_points[] = {{200, 50}, {300, 50}, {300, 150}, {200, 150}};
    lv_vector_path_move_to(path1, &rect_points[0]);
    lv_vector_path_line_to(path1, &rect_points[1]);
    lv_vector_path_line_to(path1, &rect_points[2]);
    lv_vector_path_line_to(path1, &rect_points[3]);
    lv_vector_path_close(path1);

    lv_fpoint_t center = {270, 100};
    lv_vector_path_move_to(path2, &center);
    lv_vector_path_append_circle(path2, &center, 50, 50);

    /* Draw original paths */
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0, 0xff));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_50);
    lv_vector_dsc_add_path(ctx, path1);

    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0, 0));
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_50);
    lv_vector_dsc_add_path(ctx, path2);

    /* Draw clipped paths with offset */
    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_COVER);

    /* INTERSECT with offset */
    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_INTERSECT, result, path1, path2);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0xff, 0));
    lv_vector_dsc_translate(ctx, -200, 140);
    lv_vector_dsc_add_path(ctx, result);

    /* UNION with offset */
    lv_vector_path_clear(result);
    lv_vector_dsc_translate(ctx, 200, 0);
    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_UNION, result, path1, path2);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0xff, 0));
    lv_vector_dsc_add_path(ctx, result);

    /* XOR with offset */
    lv_vector_path_clear(result);
    lv_vector_dsc_translate(ctx, -200, 140);
    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_XOR, result, path1, path2);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0xff, 0));
    lv_vector_dsc_add_path(ctx, result);


    /* Test DIFF operation */
    lv_vector_path_clear(result);
    lv_vector_dsc_translate(ctx, 200, 0);
    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_DIFF, result, path1, path2);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0xff, 0));
    lv_vector_dsc_add_path(ctx, result);

    /* Draw all in one snapshot */
    draw_vector(ctx);
    draw_snapshot("clipper_operations");

    /* Clean up */
    lv_vector_path_delete(path1);
    lv_vector_path_delete(path2);
    lv_vector_path_delete(result);
    lv_vector_dsc_delete(ctx);
}

void test_draw_same_path_different_stroke_dsc(void)
{
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_t * path_moved = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    /* Clear background */
    lv_area_t rect = {0, 0, 640, 480};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect);

    /* Same path, different stroke properties.
     * Layout: 2x2 quadrants in one snapshot.
     *   TL: stroke_width (3 variants)
     *   TR: stroke_cap   (3 variants)
     *   BL: stroke_join  (3 variants)
     *   BR: miter_limit  (3 variants, with miter join)
     */

    /* Use simple geometry:
     * - For width/cap: straight line
     * - For join/miter_limit: polyline with a corner
     * Each quadrant has 3 non-overlapping rows.
     */
    lv_vector_path_clear(path);
    lv_fpoint_t p0 = { 30, 40 };
    lv_fpoint_t p1 = { 200, 40 };

    /* Corner polyline for join/miter (reference: test_draw_lines_group)
     * Use a 4-point polyline to show join behavior clearly.
     */
    lv_fpoint_t j0 = { 30, 60 };
    lv_fpoint_t j1 = { 80, 10 };
    lv_fpoint_t j2 = { 130, 110 };
    lv_fpoint_t j3 = { 180, 60 };

    /* Build base path as a straight line (used by TL/TR) */
    lv_vector_path_move_to(path, &p0);
    lv_vector_path_line_to(path, &p1);

    lv_vector_dsc_set_fill_opa(ctx, LV_OPA_TRANSP);
    lv_vector_dsc_set_stroke_opa(ctx, LV_OPA_COVER);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_black());
    lv_vector_dsc_set_stroke_width(ctx, 10.0f);

    /* Helper: move (copy) the same path geometry to target quadrant without using
     * dsc_save/dsc_restore or clip. This keeps "same path" semantics (same geometry),
     * while meeting the restriction.
     */
    const int32_t q_dx[4] = { 0, 240, 0, 240 };
    const int32_t q_dy[4] = { 0, 0, 240, 240 };
    const int32_t row_h = 60;

    /* TL: stroke_width */
    {
        const float widths[] = { 2.0f, 8.0f, 16.0f };
        for(uint32_t i = 0; i < 3; i++) {
            const int32_t dx = q_dx[0];
            const int32_t dy = q_dy[0] + (int32_t)i * row_h;

            lv_vector_path_clear(path_moved);
            lv_vector_path_move_to(path_moved, &(lv_fpoint_t) {
                p0.x + dx, p0.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                p1.x + dx, p1.y + dy
            });

            lv_vector_dsc_set_stroke_width(ctx, widths[i]);
            lv_vector_dsc_set_stroke_cap(ctx, LV_VECTOR_STROKE_CAP_BUTT);
            lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_MITER);
            lv_vector_dsc_set_stroke_miter_limit(ctx, 4.0f);
            lv_vector_dsc_add_path(ctx, path_moved);
        }
    }

    /* TR: stroke_cap */
    {
        const lv_vector_stroke_cap_t caps[] = {
            LV_VECTOR_STROKE_CAP_BUTT,
            LV_VECTOR_STROKE_CAP_SQUARE,
            LV_VECTOR_STROKE_CAP_ROUND,
        };
        for(uint32_t i = 0; i < 3; i++) {
            const int32_t dx = q_dx[1];
            const int32_t dy = q_dy[1] + (int32_t)i * row_h;

            lv_vector_path_clear(path_moved);
            lv_vector_path_move_to(path_moved, &(lv_fpoint_t) {
                p0.x + dx, p0.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                p1.x + dx, p1.y + dy
            });

            lv_vector_dsc_set_stroke_width(ctx, 12.0f);
            lv_vector_dsc_set_stroke_cap(ctx, caps[i]);
            lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_MITER);
            lv_vector_dsc_set_stroke_miter_limit(ctx, 4.0f);
            lv_vector_dsc_add_path(ctx, path_moved);
        }
    }

    /* BL: stroke_join */
    {
        const lv_vector_stroke_join_t joins[] = {
            LV_VECTOR_STROKE_JOIN_MITER,
            LV_VECTOR_STROKE_JOIN_BEVEL,
            LV_VECTOR_STROKE_JOIN_ROUND,
        };
        for(uint32_t i = 0; i < 3; i++) {
            const int32_t dx = q_dx[2];
            const int32_t dy = q_dy[2] + (int32_t)i * row_h;

            lv_vector_path_clear(path_moved);
            lv_vector_path_move_to(path_moved, &(lv_fpoint_t) {
                j0.x + dx, j0.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                j1.x + dx, j1.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                j2.x + dx, j2.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                j3.x + dx, j3.y + dy
            });

            lv_vector_dsc_set_stroke_width(ctx, 12.0f);
            lv_vector_dsc_set_stroke_cap(ctx, LV_VECTOR_STROKE_CAP_BUTT);
            lv_vector_dsc_set_stroke_join(ctx, joins[i]);
            lv_vector_dsc_set_stroke_miter_limit(ctx, 4.0f);
            lv_vector_dsc_add_path(ctx, path_moved);
        }
    }

    /* BR: miter_limit (keep join=miter) */
    {
        const uint16_t limits[] = { 1, 4, 12 };
        for(uint32_t i = 0; i < 3; i++) {
            const int32_t dx = q_dx[3];
            const int32_t dy = q_dy[3] + (int32_t)i * row_h;

            lv_vector_path_clear(path_moved);
            lv_vector_path_move_to(path_moved, &(lv_fpoint_t) {
                j0.x + dx, j0.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                j1.x + dx, j1.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                j2.x + dx, j2.y + dy
            });
            lv_vector_path_line_to(path_moved, &(lv_fpoint_t) {
                j3.x + dx, j3.y + dy
            });

            lv_vector_dsc_set_stroke_width(ctx, 12.0f);
            lv_vector_dsc_set_stroke_cap(ctx, LV_VECTOR_STROKE_CAP_BUTT);
            lv_vector_dsc_set_stroke_join(ctx, LV_VECTOR_STROKE_JOIN_MITER);
            lv_vector_dsc_set_stroke_miter_limit(ctx, limits[i]);
            lv_vector_dsc_add_path(ctx, path_moved);
        }
    }

    draw_vector(ctx);
    draw_snapshot(SNAPSHOT_NAME(same_path_different_stroke_dsc));

    /* Cleanup */
    lv_vector_path_delete(path_moved);
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(ctx);
}

#endif
#endif
