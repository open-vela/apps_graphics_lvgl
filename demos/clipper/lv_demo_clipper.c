/*********************
 *      INCLUDES
 *********************/

#include "lv_demo_clipper.h"

#if LV_USE_DEMO_POLYGON_CLIPPER
#include <uikit/uikit.h>

#include "time.h"
#include "unistd.h"
#include "sys/time.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

#define MAX_SCENE_ID 4

typedef struct {
    lv_obj_t obj;
    int scene_id;
    int process;
    int step;
    lv_vector_path_t * path1;
    lv_vector_path_t * path2;
    lv_vector_path_t * path3;
    lv_timer_t * anim_timer;
} lv_vector_clipper_obj_t;

#define MY_CLASS &lv_vector_clipper_class

static void lv_vector_clipper_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_vector_clipper_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_vector_clipper_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void lv_vector_clipper_draw(lv_obj_t * obj, lv_event_t * e);

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_obj_t * lv_vector_clipper_create(lv_obj_t * parent);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

static void _draw_scene_1(lv_vector_dsc_t * ctx, const lv_vector_clipper_obj_t * clipper)
{
    lv_vector_dsc_identity(ctx);

    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0, 0));
    lv_vector_dsc_add_path(ctx, clipper->path1);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0, 0xff));
    lv_vector_dsc_add_path(ctx, clipper->path2);

    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_INTERSECT, clipper->path3, clipper->path1, clipper->path2);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, clipper->path3);
}

static void _draw_scene_2(lv_vector_dsc_t * ctx, const lv_vector_clipper_obj_t * clipper)
{
    lv_vector_dsc_identity(ctx);

    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0, 0));
    lv_vector_dsc_add_path(ctx, clipper->path1);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0, 0xff));
    lv_vector_dsc_add_path(ctx, clipper->path2);

    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_DIFF, clipper->path3, clipper->path2, clipper->path1);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, clipper->path3);
}

static void _draw_scene_3(lv_vector_dsc_t * ctx, const lv_vector_clipper_obj_t * clipper)
{
    lv_vector_dsc_identity(ctx);

    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0, 0));
    lv_vector_dsc_set_fill_opa(ctx, 120);
    lv_vector_dsc_add_path(ctx, clipper->path1);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0, 0xff));
    lv_vector_dsc_set_fill_opa(ctx, 120);
    lv_vector_dsc_add_path(ctx, clipper->path2);

    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_XOR, clipper->path3, clipper->path2, clipper->path1);
    lv_vector_dsc_set_fill_opa(ctx, 255);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0xff, 0x00));
    lv_vector_dsc_add_path(ctx, clipper->path3);
}

static void _draw_scene_4(lv_vector_dsc_t * ctx, const lv_vector_clipper_obj_t * clipper)
{
    lv_vector_dsc_identity(ctx);

    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0xff, 0, 0));
    lv_vector_dsc_add_path(ctx, clipper->path1);
    lv_vector_dsc_set_fill_color(ctx, lv_color_make(0, 0, 0xff));
    lv_vector_dsc_add_path(ctx, clipper->path2);

    lv_vector_path_polygon_clipper(LV_VECTOR_CLIPPER_UNION, clipper->path3, clipper->path2, clipper->path1);
    lv_vector_dsc_set_stroke_color(ctx, lv_color_make(0, 0xff, 0x00));
    lv_vector_dsc_set_stroke_width(ctx, 5);
    lv_vector_dsc_set_stroke_opa(ctx, 255);
    lv_vector_dsc_set_fill_opa(ctx, 0);
    lv_vector_dsc_add_path(ctx, clipper->path3);
}

static void draw_animation(lv_obj_t * obj, lv_layer_t * layer)
{
    int32_t w = lv_obj_get_width(obj);
    int32_t h = lv_obj_get_height(obj);
    lv_vector_clipper_obj_t * clipper = (lv_vector_clipper_obj_t *)obj;
    lv_vector_dsc_t * ctx = lv_vector_dsc_create(layer);

    lv_area_t rect = {0, 0, w, h};
    lv_vector_dsc_set_fill_color(ctx, lv_color_white());
    lv_vector_clear_area(ctx, &rect); // clear screen

    switch(clipper->scene_id) {
        case 1:
            _draw_scene_1(ctx, clipper);
            break;
        case 2:
            _draw_scene_2(ctx, clipper);
            break;
        case 3:
            _draw_scene_3(ctx, clipper);
            break;
        case 4:
            _draw_scene_4(ctx, clipper);
            break;
    }

    lv_draw_vector(ctx); // submit draw
    lv_vector_dsc_delete(ctx);
}

static void _create_scene_1(lv_vector_clipper_obj_t * clipper)
{
    clipper->process = 0;
    clipper->step = 1;

    lv_vector_path_clear(clipper->path1);
    lv_vector_path_clear(clipper->path2);

    lv_vector_path_t * p1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_area_t rect = {10, 10, 350, 250};
    lv_vector_path_append_rect(p1, &rect, 40, 40);
    lv_vector_path_to_polygon(clipper->path1, p1);

    lv_vector_path_clear(p1);

    lv_fpoint_t p = {400, 350};
    lv_vector_path_append_circle(p1, &p, 150, 100);
    lv_vector_path_to_polygon(clipper->path2, p1);

    lv_vector_path_delete(p1);
}

static void _create_scene_2(lv_vector_clipper_obj_t * clipper)
{
    clipper->process = 0;
    clipper->step = 1;

    lv_vector_path_clear(clipper->path1);
    lv_vector_path_clear(clipper->path2);

    lv_matrix_t matrix;

    lv_vector_path_t * p1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    vg_vector_shapes_factory(VG_VECTOR_SHAPE_CLOVER, p1);
    lv_matrix_identity(&matrix);
    lv_matrix_scale(&matrix, 0.5f, 0.5f);
    lv_matrix_translate(&matrix, 400, 400);
    lv_matrix_transform_path(&matrix, p1);
    lv_vector_path_to_polygon(clipper->path1, p1);

    lv_vector_path_clear(p1);

    vg_vector_shapes_factory(VG_VECTOR_SHAPE_HEART, p1);
    lv_matrix_identity(&matrix);
    lv_matrix_scale(&matrix, 0.3f, 0.3f);
    lv_matrix_translate(&matrix, 900, 600);
    lv_matrix_transform_path(&matrix, p1);
    lv_vector_path_to_polygon(clipper->path2, p1);
}

static void _create_scene_3(lv_vector_clipper_obj_t * clipper)
{
    clipper->process = 0;
    clipper->step = 1;

    lv_vector_path_clear(clipper->path1);
    lv_vector_path_clear(clipper->path2);

    lv_vector_path_t * p1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_area_t rect = {100, 50, 200, 150};
    lv_vector_path_append_rect(p1, &rect, 20, 20);
    lv_vector_path_to_polygon(clipper->path1, p1);

    lv_vector_path_clear(p1);

    lv_fpoint_t p = {400, 120};
    lv_vector_path_append_circle(p1, &p, 80, 60);
    lv_vector_path_to_polygon(clipper->path2, p1);

    lv_vector_path_delete(p1);
}

static void _create_scene_4(lv_vector_clipper_obj_t * clipper)
{
    clipper->process = 0;
    clipper->step = 1;

    lv_vector_path_clear(clipper->path1);
    lv_vector_path_clear(clipper->path2);

    lv_matrix_t matrix;

    lv_vector_path_t * p1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    vg_vector_shapes_factory(VG_VECTOR_SHAPE_CLOVER, p1);
    lv_matrix_identity(&matrix);
    lv_matrix_scale(&matrix, 0.5f, 0.5f);
    lv_matrix_translate(&matrix, 300, 200);
    lv_matrix_transform_path(&matrix, p1);
    lv_vector_path_to_polygon(clipper->path1, p1);

    lv_vector_path_clear(p1);

    vg_vector_shapes_factory(VG_VECTOR_SHAPE_HEART, p1);
    lv_matrix_identity(&matrix);
    lv_matrix_scale(&matrix, 0.3f, 0.3f);
    lv_matrix_translate(&matrix, 900, 400);
    lv_matrix_transform_path(&matrix, p1);
    lv_vector_path_to_polygon(clipper->path2, p1);
}

static void _update_scenes(lv_vector_clipper_obj_t * clipper)
{
    switch(clipper->scene_id) {
        case 1:
            _create_scene_1(clipper);
            break;
        case 2:
            _create_scene_2(clipper);
            break;
        case 3:
            _create_scene_3(clipper);
            break;
        case 4:
            _create_scene_4(clipper);
            break;
    }
}

static void lv_vector_clipper_click(lv_obj_t * obj, int32_t x, int32_t y)
{
    lv_vector_clipper_obj_t * clipper = (lv_vector_clipper_obj_t *)obj;

    clipper->scene_id++;
    if(clipper->scene_id > MAX_SCENE_ID) {
        clipper->scene_id = 1;
    }
    _update_scenes(clipper);
}

static void widget_draw(lv_vector_clipper_obj_t * clipper)
{
    lv_obj_t * obj = (lv_obj_t *)clipper;
    lv_obj_invalidate(obj);
}

static void _update_scene_1(lv_vector_clipper_obj_t * clipper)
{
    if(clipper->process == 50) {
        clipper->step = -1;
    }
    else if(clipper->process == 0) {
        clipper->step = 1;
    }

    clipper->process += clipper->step;

    float t = (float)clipper->step;

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, t * 5.0f, t * 5.0f);

    lv_matrix_transform_path(&matrix, clipper->path1);

    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, t * -5.0f, t * -5.0f);

    lv_matrix_transform_path(&matrix, clipper->path2);
}

static void _update_scene_2(lv_vector_clipper_obj_t * clipper)
{
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, -200, -160);
    lv_matrix_rotate(&matrix, 5);
    lv_matrix_translate(&matrix, 200, 160);

    lv_matrix_transform_path(&matrix, clipper->path1);
}

static void _update_scene_3(lv_vector_clipper_obj_t * clipper)
{
    if(clipper->process == 50) {
        clipper->step = -1;
    }
    else if(clipper->process == 0) {
        clipper->step = 1;
    }

    clipper->process += clipper->step;

    float t = (float)clipper->step;

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, t * 5.0f, 0);

    lv_matrix_transform_path(&matrix, clipper->path1);

    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, t * -5.0f, 0);

    lv_matrix_transform_path(&matrix, clipper->path2);
}

static void _update_scene_4(lv_vector_clipper_obj_t * clipper)
{
    if(clipper->process == 50) {
        clipper->step = -1;
    }
    else if(clipper->process == 0) {
        clipper->step = 1;
    }

    clipper->process += clipper->step;

    float t = (float)clipper->step;

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, t * 2.0f, 0);

    lv_matrix_transform_path(&matrix, clipper->path1);

    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, t * -2.0f, 0);

    lv_matrix_transform_path(&matrix, clipper->path2);
}

static void anim_timer_cb(lv_timer_t * param)
{
    lv_vector_clipper_obj_t * clipper = (lv_vector_clipper_obj_t *)param->user_data;

    switch(clipper->scene_id) {
        case 1:
            _update_scene_1(clipper);
            break;
        case 2:
            _update_scene_2(clipper);
            break;
        case 3:
            _update_scene_3(clipper);
            break;
        case 4:
            _update_scene_4(clipper);
            break;
    }

    widget_draw(clipper);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

const lv_obj_class_t lv_vector_clipper_class  = {
    .constructor_cb = lv_vector_clipper_constructor,
    .destructor_cb = lv_vector_clipper_destructor,
    .event_cb = lv_vector_clipper_event,
    .instance_size = sizeof(lv_vector_clipper_obj_t),
    .base_class = &lv_obj_class,
    .name = "lv_clipper",
};

static lv_obj_t * lv_vector_clipper_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

static void lv_vector_clipper_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_vector_clipper_obj_t * clipper = (lv_vector_clipper_obj_t *)obj;
    clipper->scene_id = 1;
    clipper->path1 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    clipper->path2 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    clipper->path3 = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);

    _create_scene_1(clipper);

    clipper->anim_timer = lv_timer_create(anim_timer_cb, 16, clipper);
}

static void lv_vector_clipper_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_vector_clipper_obj_t * clipper = (lv_vector_clipper_obj_t *)obj;
    lv_timer_delete(clipper->anim_timer);
    lv_vector_path_delete(clipper->path1);
    lv_vector_path_delete(clipper->path2);
    lv_vector_path_delete(clipper->path3);
}

static void lv_vector_clipper_draw(lv_obj_t * obj, lv_event_t * e)
{
    lv_layer_t * layer = lv_event_get_layer(e);
    draw_animation(obj, layer);
}

static void lv_vector_clipper_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_result_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);
    if(code == LV_EVENT_DRAW_MAIN) {
        lv_vector_clipper_draw(obj, e);
    }
    else if(code == LV_EVENT_CLICKED) {
        lv_point_t point;
        lv_indev_get_point(lv_indev_active(), &point);
        lv_vector_clipper_click(obj, point.x, point.y);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
void lv_demo_vector_clipper(void)
{
    lv_obj_t * clipper_widget = lv_vector_clipper_create(lv_scr_act());
    lv_obj_set_size(clipper_widget, LV_PCT(100), LV_PCT(100));
}

#else

void lv_demo_vector_clipper(void)
{
    /*fallback for online examples*/
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Vector graphics is not enabled");
    lv_obj_center(label);
}

#endif
