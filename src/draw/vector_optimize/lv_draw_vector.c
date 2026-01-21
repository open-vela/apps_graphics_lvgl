/**
* @file lv_draw_vector.c
 *
 */

/*********************
*      INCLUDES
 *********************/
#include "lv_draw_vector_private.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE

#include "../../misc/lv_ll.h"
#include "../../misc/lv_types.h"
#include "../../stdlib/lv_string.h"
#include <math.h>
#include <float.h>

#define MATH_PI  3.14159265358979323846f
#define MATH_HALF_PI 1.57079632679489661923f

#define DEG_TO_RAD 0.017453292519943295769236907684886f
#define RAD_TO_DEG 57.295779513082320876798154814105f

#define MATH_RADIANS(deg) ((deg) * DEG_TO_RAD)
#define MATH_DEGRESS(rad) ((rad) * RAD_TO_DEG)

#define DASH_MAX 32
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
/*********************
*      DEFINES
 *********************/

#ifndef M_PI
    #define M_PI 3.1415926f
#endif

#define LV_VECTOR_ALLOCATOR_INIT_SIZE 1024

/**********************
*      TYPEDEFS
 **********************/

/**********************
*  STATIC PROTOTYPES
 **********************/

static void _copy_draw_attrs(lv_vector_draw_style_t style, lv_vector_draw_style_attrs_t * dst_attrs,
                             const lv_vector_draw_style_attrs_t * src_attrs)
{
    switch(style) {
        case LV_VECTOR_DRAW_STYLE_SOLID:
            dst_attrs->color = src_attrs->color;
            break;
        case LV_VECTOR_DRAW_STYLE_PATTERN:
            lv_memcpy(&dst_attrs->img_dsc, &src_attrs->img_dsc, sizeof(lv_draw_image_dsc_t));
            break;
        case LV_VECTOR_DRAW_STYLE_GRADIENT:
            lv_memcpy(&dst_attrs->gradient, &src_attrs->gradient, sizeof(lv_vector_gradient_t));
            break;
    }
}

static void _copy_fill_dsc(lv_vector_fill_dsc_t * fill_dsc, const lv_vector_fill_dsc_t * fill_src)
{
    fill_dsc->style = fill_src->style;
    fill_dsc->opa = fill_src->opa;
    fill_dsc->fill_rule = fill_src->fill_rule;
    fill_dsc->fill_units = fill_src->fill_units;
    lv_memcpy(&(fill_dsc->matrix), &(fill_src->matrix), sizeof(lv_matrix_t));
    _copy_draw_attrs(fill_dsc->style, &fill_dsc->draw_attrs, &fill_src->draw_attrs);
}

static void _copy_stroke_dsc(lv_vector_stroke_dsc_t * stroke_dsc, const lv_vector_stroke_dsc_t * stroke_src)
{
    stroke_dsc->style = stroke_src->style;
    stroke_dsc->opa = stroke_src->opa;
    stroke_dsc->width = stroke_src->width;
    stroke_dsc->cap = stroke_src->cap;
    stroke_dsc->join = stroke_src->join;
    stroke_dsc->miter_limit = stroke_src->miter_limit;
    stroke_dsc->dash_count = stroke_src->dash_count;
    if(stroke_src->dash_count > 0) {
        lv_memcpy(stroke_dsc->dash_pattern, stroke_src->dash_pattern, sizeof(float) * stroke_src->dash_count);
    }
    lv_memcpy(&(stroke_dsc->matrix), &(stroke_src->matrix), sizeof(lv_matrix_t));
    _copy_draw_attrs(stroke_dsc->style, &stroke_dsc->draw_attrs, &stroke_src->draw_attrs);
}

/**********************
*   GLOBAL FUNCTIONS
 **********************/

static inline void lv_vector_ensure_write_access(lv_vector_path_t * path)
{
    path->impl->flags |= PATH_FLAG_CHANGED;
    if(path->impl->ref_count > 1) {
        lv_platform_path_base_t * new_impl = path->impl->handlers->clone(path->impl);
        LV_ASSERT_MALLOC(new_impl);

        new_impl->ref_count = 1;
        new_impl->flags = path->impl->flags;
        new_impl->handlers = path->impl->handlers;

        lv_vector_path_unref(path->impl);
        path->impl = new_impl;
    }
}

static inline void lv_vector_init_platform_path(lv_platform_path_base_t * impl, int flags)
{
    impl->ref_count = 1;
    impl->flags = flags;
    impl->handlers = lv_vector_get_platform_handlers();
}

void lv_matrix_transform_point(const lv_matrix_t * matrix, lv_fpoint_t * point)
{
    float x = point->x;
    float y = point->y;

    point->x = x * matrix->m[0][0] + y * matrix->m[0][1] + matrix->m[0][2];
    point->y = x * matrix->m[1][0] + y * matrix->m[1][1] + matrix->m[1][2];
}

void lv_matrix_transform_path(const lv_matrix_t * matrix, lv_vector_path_t * path)
{
    LV_ASSERT_NULL(matrix);
    LV_ASSERT_NULL(path);
    if(lv_vector_path_is_empty(path)) return;

    lv_vector_ensure_write_access(path);
    path->impl->handlers->transform(path->impl, matrix);
}

/* path functions */
lv_vector_path_t * lv_vector_path_create(lv_vector_path_quality_t quality)
{
    lv_vector_path_t * path = lv_malloc(sizeof(lv_vector_path_t));
    if(!path) return NULL;

    lv_platform_path_base_t * impl = lv_vector_get_platform_handlers()->create(quality);
    if(!impl) {
        lv_free(path);
        return NULL;
    }

    lv_vector_init_platform_path(impl, 0);
    path->impl = impl;
    return path;
}

void lv_vector_path_copy(lv_vector_path_t * target_path, const lv_vector_path_t * path)
{
    LV_ASSERT_NULL(target_path);
    LV_ASSERT_NULL(path);
    if((target_path == path) || (lv_vector_path_is_empty(path))) return;

    lv_vector_ensure_write_access(target_path);
    target_path->impl->handlers->clear(target_path->impl);
    target_path->impl->handlers->concat(target_path->impl, path->impl);
}

void lv_vector_path_clear(lv_vector_path_t * path)
{
    LV_ASSERT_NULL(path);

    lv_vector_ensure_write_access(path);
    path->impl->handlers->clear(path->impl);
}

void lv_vector_path_delete(lv_vector_path_t * path)
{
    if(path == NULL) return;
    lv_vector_path_unref(path->impl);
    lv_free(path);
}

void lv_vector_path_move_to(lv_vector_path_t * path, const lv_fpoint_t * p)
{
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(p);

    lv_vector_ensure_write_access(path);
    path->impl->handlers->move_to(path->impl, p);
}

void lv_vector_path_line_to(lv_vector_path_t * path, const lv_fpoint_t * p)
{
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(p);

    if(lv_vector_path_is_empty(path)) {
        /*first op must be move_to*/
        return;
    }

    lv_vector_ensure_write_access(path);
    path->impl->handlers->line_to(path->impl, p);
}

void lv_vector_path_quad_to(lv_vector_path_t * path, const lv_fpoint_t * ctrl, const lv_fpoint_t * end)
{
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(ctrl);
    LV_ASSERT_NULL(end);

    if(lv_vector_path_is_empty(path)) {
        /*first op must be move_to*/
        return;
    }

    lv_vector_ensure_write_access(path);
    path->impl->handlers->quad_to(path->impl, ctrl, end);
}

void lv_vector_path_cubic_to(lv_vector_path_t * path, const lv_fpoint_t * ctrl1,
                             const lv_fpoint_t * ctrl2, const lv_fpoint_t * end)
{
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(ctrl1);
    LV_ASSERT_NULL(ctrl2);
    LV_ASSERT_NULL(end);

    if(lv_vector_path_is_empty(path)) {
        /*first op must be move_to*/
        return;
    }

    lv_vector_ensure_write_access(path);
    path->impl->handlers->cubic_to(path->impl, ctrl1, ctrl2, end);
}

void lv_vector_path_close(lv_vector_path_t * path)
{
    LV_ASSERT_NULL(path);

    if(lv_vector_path_is_empty(path)) {
        /*first op must be move_to*/
        return;
    }

    lv_vector_ensure_write_access(path);
    path->impl->handlers->close_path(path->impl);
}

void lv_vector_path_get_bounding(const lv_vector_path_t * path, lv_area_t * area)
{
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(area);

    path->impl->handlers->get_bounding(path->impl, area);
}

lv_vector_path_quality_t lv_vector_path_get_quality(lv_vector_path_t * path)
{
    return path->impl->handlers->get_quality(path->impl);
}

bool lv_vector_path_is_empty(const lv_vector_path_t * path)
{
    return lv_vector_path_impl_is_empty(path->impl);
}

bool lv_vector_path_is_polygon(const lv_vector_path_t * path)
{
    return path->impl->flags & PATH_FLAG_POLYGON;
}

size_t lv_vector_path_get_mem_size(const lv_vector_path_t * path)
{
    return path->impl->handlers->get_mem_size(path->impl);
}

void lv_vector_path_append_rectangle(lv_vector_path_t * path, float x, float y, float w, float h, float rx, float ry)
{
    if(w <= 0.0f || h <= 0.0f) return;

    float hw = w * 0.5f;
    float hh = h * 0.5f;

    if(rx > hw) rx = hw;
    if(ry > hh) ry = hh;

    if(rx <= 0.0f && ry <= 0.0f) {
        lv_fpoint_t pt = {x, y};
        lv_vector_path_move_to(path, &pt);
        pt.x += w;
        lv_vector_path_line_to(path, &pt);
        pt.y += h;
        lv_vector_path_line_to(path, &pt);
        pt.x -= w;
        lv_vector_path_line_to(path, &pt);
        lv_vector_path_close(path);
        return;
    }

    if(rx == hw && ry == hh) {
        lv_fpoint_t pt = {x + hw, y + hh};
        lv_vector_path_append_circle(path, &pt, rx, ry);
        return;
    }

    float hrx = rx * 0.5f;
    float hry = ry * 0.5f;
    lv_fpoint_t pt, pt2, pt3;

    pt.x = x + rx;
    pt.y = y;
    lv_vector_path_move_to(path, &pt);

    pt.x = x + w - rx;
    pt.y = y;
    lv_vector_path_line_to(path, &pt);

    pt.x = x + w - rx + hrx;
    pt.y = y;
    pt2.x = x + w;
    pt2.y = y + ry - hry;
    pt3.x = x + w;
    pt3.y = y + ry;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    pt.x = x + w;
    pt.y = y + h - ry;
    lv_vector_path_line_to(path, &pt);

    pt.x = x + w;
    pt.y = y + h - ry + hry;
    pt2.x = x + w - rx + hrx;
    pt2.y = y + h;
    pt3.x = x + w - rx;
    pt3.y = y + h;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    pt.x = x + rx;
    pt.y = y + h;
    lv_vector_path_line_to(path, &pt);

    pt.x = x + rx - hrx;
    pt.y = y + h;
    pt2.x = x;
    pt2.y = y + h - ry + hry;
    pt3.x = x;
    pt3.y = y + h - ry;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    pt.x = x;
    pt.y = y + ry;
    lv_vector_path_line_to(path, &pt);

    pt.x = x;
    pt.y = y + ry - hry;
    pt2.x = x + rx - hrx;
    pt2.y = y;
    pt3.x = x + rx;
    pt3.y = y;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    lv_vector_path_close(path);
}
void lv_vector_path_append_circle(lv_vector_path_t * path, const lv_fpoint_t * c, float rx, float ry)
{
    float krx = rx * 0.552284f;
    float kry = ry * 0.552284f;
    float cx = c->x;
    float cy = c->y;

    lv_fpoint_t pt, pt2, pt3;
    pt.x = cx;
    pt.y = cy - ry;
    lv_vector_path_move_to(path, &pt);

    pt.x = cx + krx;
    pt.y = cy - ry;
    pt2.x = cx + rx;
    pt2.y = cy - kry;
    pt3.x = cx + rx;
    pt3.y = cy;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    pt.x = cx + rx;
    pt.y = cy + kry;
    pt2.x = cx + krx;
    pt2.y = cy + ry;
    pt3.x = cx;
    pt3.y = cy + ry;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    pt.x = cx - krx;
    pt.y = cy + ry;
    pt2.x = cx - rx;
    pt2.y = cy + kry;
    pt3.x = cx - rx;
    pt3.y = cy;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    pt.x = cx - rx;
    pt.y = cy - kry;
    pt2.x = cx - krx;
    pt2.y = cy - ry;
    pt3.x = cx;
    pt3.y = cy - ry;
    lv_vector_path_cubic_to(path, &pt, &pt2, &pt3);

    lv_vector_path_close(path);
}

/**
 * Add a arc to the path
 * @param path              pointer to a path
 * @param c                 pointer to a `lv_fpoint_t` variable for center of the circle
 * @param radius            the radius for arc
 * @param start_angle       the start angle for arc
 * @param sweep             the sweep angle for arc, could be negative
 * @param pie               true: draw a pie, false: draw a arc
 */
void lv_vector_path_append_arc(lv_vector_path_t * path, const lv_fpoint_t * c, float radius, float start_angle,
                               float sweep, bool pie)
{
    float cx = c->x;
    float cy = c->y;

    /* just circle */
    if(sweep >= 360.0f || sweep <= -360.0f) {
        lv_vector_path_append_circle(path, c, radius, radius);
        return;
    }

    start_angle = MATH_RADIANS(start_angle);
    sweep = MATH_RADIANS(sweep);

    int n_curves = (int)ceil(fabsf(sweep / MATH_HALF_PI));
    float sweep_sign = sweep < 0 ? -1.f : 1.f;
    float fract = fmodf(sweep, MATH_HALF_PI);
    fract = (fabsf(fract) < FLT_EPSILON) ? MATH_HALF_PI * sweep_sign : fract;

    /* Start from here */
    lv_fpoint_t start = {
        .x = radius * cosf(start_angle),
        .y = radius * sinf(start_angle),
    };

    if(pie) {
        lv_vector_path_move_to(path, &(lv_fpoint_t) {
            cx, cy
        });
        lv_vector_path_line_to(path, &(lv_fpoint_t) {
            start.x + cx, start.y + cy
        });
    }
    else {
        lv_vector_path_move_to(path, &(lv_fpoint_t) {
            start.x + cx, start.y + cy
        });
    }

    for(int i = 0; i < n_curves; ++i) {
        float end_angle = start_angle + ((i != n_curves - 1) ? MATH_HALF_PI * sweep_sign : fract);
        float end_x = radius * cosf(end_angle);
        float end_y = radius * sinf(end_angle);

        /* variables needed to calculate bezier control points */

        /** get bezier control points using article:
         * (http://itc.ktu.lt/index.php/ITC/article/view/11812/6479)
         */
        float ax = start.x;
        float ay = start.y;
        float bx = end_x;
        float by = end_y;
        float q1 = ax * ax + ay * ay;
        float q2 = ax * bx + ay * by + q1;
        float k2 = (4.0f / 3.0f) * ((sqrtf(2 * q1 * q2) - q2) / (ax * by - ay * bx));

        /* Next start point is the current end point */
        start.x = end_x;
        start.y = end_y;

        end_x += cx;
        end_y += cy;

        lv_fpoint_t ctrl1 = {ax - k2 * ay + cx, ay + k2 * ax + cy};
        lv_fpoint_t ctrl2 = {bx + k2 * by + cx, by - k2 * bx + cy};
        lv_fpoint_t end = {end_x, end_y};
        lv_vector_path_cubic_to(path, &ctrl1, &ctrl2, &end);
        start_angle = end_angle;
    }

    if(pie) {
        lv_vector_path_close(path);
    }
}

void lv_vector_path_append_path(lv_vector_path_t * path, const lv_vector_path_t * subpath)
{
    if(lv_vector_path_is_empty(subpath)) return;

    lv_vector_ensure_write_access(path);
    path->impl->handlers->concat(path->impl, subpath->impl);
}

/* draw dsc functions */

static inline void lv_vector_dsc_fill_ensure_write_access(lv_linear_allocator * allocator,
                                                          lv_vector_fill_dsc_t ** fill_dsc)
{
    if((*fill_dsc)->use_count > 0) {
        lv_vector_fill_dsc_t * new_dsc = allocator->alloc(allocator, sizeof(lv_vector_fill_dsc_t));
        LV_ASSERT_MALLOC(new_dsc);

        _copy_fill_dsc(new_dsc, *fill_dsc);
        new_dsc->use_count = 0;

        *fill_dsc = new_dsc;
    }
}

static inline void lv_vector_dsc_stroke_ensure_write_access(lv_linear_allocator * allocator,
                                                            lv_vector_stroke_dsc_t ** stroke_dsc)
{
    if((*stroke_dsc)->use_count > 0) {
        lv_vector_stroke_dsc_t * new_dsc = allocator->alloc(allocator, sizeof(lv_vector_stroke_dsc_t));
        LV_ASSERT_MALLOC(new_dsc);

        new_dsc->dash_count = (*stroke_dsc)->dash_count;
        if(new_dsc->dash_count > 0) {
            new_dsc->dash_pattern = allocator->alloc(allocator, sizeof(float) * new_dsc->dash_count);
            LV_ASSERT_MALLOC(new_dsc->dash_pattern);
        }

        _copy_stroke_dsc(new_dsc, *stroke_dsc);
        new_dsc->use_count = 0;
        new_dsc->stroke_dsc_changed = false;

        *stroke_dsc = new_dsc;
    }
}

static void lv_vector_draw_dsc_reset(lv_linear_allocator * allocator, lv_vector_draw_dsc_t * dsc)
{
    dsc->fill_dsc = allocator->alloc(allocator, sizeof(lv_vector_fill_dsc_t));
    LV_ASSERT_MALLOC(dsc->fill_dsc);

    dsc->stroke_dsc = allocator->alloc(allocator, sizeof(lv_vector_stroke_dsc_t));
    LV_ASSERT_MALLOC(dsc->stroke_dsc);
}

lv_vector_dsc_t * lv_vector_dsc_create(lv_layer_t * layer)
{
    lv_vector_dsc_t * dsc = lv_zalloc(sizeof(lv_vector_dsc_t));
    LV_ASSERT_MALLOC(dsc);

    dsc->current_dsc = lv_zalloc(sizeof(lv_vector_draw_dsc_t));
    LV_ASSERT_MALLOC(dsc->current_dsc);

    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, LV_VECTOR_ALLOCATOR_INIT_SIZE);
    LV_ASSERT_MALLOC(allocator);

    dsc->tasks.draw_task_list.task_list = NULL;
    dsc->tasks.draw_task_list.allocator = allocator;

    lv_vector_draw_dsc_reset(allocator, dsc->current_dsc);

    dsc->layer = layer;

    lv_vector_fill_dsc_t * fill_dsc = dsc->current_dsc->fill_dsc;
    fill_dsc->use_count = 0;
    fill_dsc->style = LV_VECTOR_DRAW_STYLE_SOLID;
    fill_dsc->draw_attrs.color = lv_color_to_32(lv_color_black(), 0xFF);
    fill_dsc->opa = LV_OPA_COVER;
    fill_dsc->fill_rule = LV_VECTOR_FILL_NONZERO;
    fill_dsc->fill_units = LV_VECTOR_FILL_UNITS_OBJECT_BOUNDING_BOX;
    lv_matrix_identity(&(fill_dsc->matrix)); /*identity matrix*/

    lv_vector_stroke_dsc_t * stroke_dsc = dsc->current_dsc->stroke_dsc;
    stroke_dsc->use_count = 0;
    stroke_dsc->style = LV_VECTOR_DRAW_STYLE_SOLID;
    stroke_dsc->draw_attrs.color = lv_color_to_32(lv_color_black(), 0xFF);
    stroke_dsc->opa = LV_OPA_0; /*default no stroke*/
    stroke_dsc->width = 1.0f;
    stroke_dsc->cap = LV_VECTOR_STROKE_CAP_BUTT;
    stroke_dsc->join = LV_VECTOR_STROKE_JOIN_MITER;
    stroke_dsc->miter_limit = 4.0f;
    stroke_dsc->dash_count = 0;
    lv_matrix_identity(&(stroke_dsc->matrix)); /*identity matrix*/
    stroke_dsc->stroke_dsc_changed = false;

    dsc->current_dsc->blend_mode = LV_VECTOR_BLEND_SRC_OVER;
    dsc->current_dsc->scissor_area = layer->_clip_area;
    lv_matrix_identity(&(dsc->current_dsc->matrix)); /*identity matrix*/
    return dsc;
}

void lv_vector_dsc_delete(lv_vector_dsc_t * dsc)
{
    if(!dsc) return;

    if(dsc->tasks.draw_task_list.task_list) {
        _lv_vector_for_each_destroy_tasks(&dsc->tasks.draw_task_list, NULL, NULL);
        dsc->tasks.draw_task_list.task_list = NULL;
    }

    if(dsc->tasks.draw_task_list.allocator) {
        lv_linear_allocator_delete(dsc->tasks.draw_task_list.allocator);
        dsc->tasks.draw_task_list.allocator = NULL;
    }

    if(dsc->current_dsc) {
        lv_free(dsc->current_dsc);
        dsc->current_dsc = NULL;
    }

    lv_free(dsc);
}

void lv_vector_dsc_set_blend_mode(lv_vector_dsc_t * dsc, lv_vector_blend_t blend)
{
    dsc->current_dsc->blend_mode = blend;
}

void lv_vector_dsc_set_transform(lv_vector_dsc_t * dsc, const lv_matrix_t * matrix)
{
    lv_memcpy(&(dsc->current_dsc->matrix), matrix, sizeof(lv_matrix_t));
}

void lv_vector_dsc_set_fill_color(lv_vector_dsc_t * dsc, lv_color_t color)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    dsc->current_dsc->fill_dsc->style = LV_VECTOR_DRAW_STYLE_SOLID;
    dsc->current_dsc->fill_dsc->draw_attrs.color = lv_color_to_32(color, 0xFF);
}

void lv_vector_dsc_set_fill_color32(lv_vector_dsc_t * dsc, lv_color32_t color)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    dsc->current_dsc->fill_dsc->style = LV_VECTOR_DRAW_STYLE_SOLID;
    dsc->current_dsc->fill_dsc->draw_attrs.color = color;
}

void lv_vector_dsc_set_fill_opa(lv_vector_dsc_t * dsc, lv_opa_t opa)
{
    if(dsc->current_dsc->fill_dsc->opa == opa) return;
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    dsc->current_dsc->fill_dsc->opa = opa;
}

void lv_vector_dsc_set_fill_rule(lv_vector_dsc_t * dsc, lv_vector_fill_t rule)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    dsc->current_dsc->fill_dsc->fill_rule = rule;
}

void lv_vector_dsc_set_fill_units(lv_vector_dsc_t * dsc, const lv_vector_fill_units_t units)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    dsc->current_dsc->fill_dsc->fill_units = units;
}

void lv_vector_dsc_set_fill_image(lv_vector_dsc_t * dsc, const lv_draw_image_dsc_t * img_dsc)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    dsc->current_dsc->fill_dsc->style = LV_VECTOR_DRAW_STYLE_PATTERN;

    lv_memcpy(&(dsc->current_dsc->fill_dsc->draw_attrs.img_dsc), img_dsc, sizeof(lv_draw_image_dsc_t));
}

void lv_vector_dsc_set_fill_linear_gradient(lv_vector_dsc_t * dsc, float x1, float y1, float x2, float y2)
{
    if(x1 == x2 && y1 == y2) {
        LV_LOG_ERROR("Gradient start and end points are the same, (%f, %f)->(%f, %f)", x1, y1, x2, y2);
        return;
    }

    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    lv_vector_gradient_t * gradient = &dsc->current_dsc->fill_dsc->draw_attrs.gradient;
    dsc->current_dsc->fill_dsc->style = LV_VECTOR_DRAW_STYLE_GRADIENT;
    gradient->style = LV_VECTOR_GRADIENT_STYLE_LINEAR;
    gradient->x1 = x1;
    gradient->y1 = y1;
    gradient->x2 = x2;
    gradient->y2 = y2;
    gradient->stops_count = 0;
    gradient->spread = LV_VECTOR_GRADIENT_SPREAD_PAD;
}

void lv_vector_dsc_set_fill_radial_gradient(lv_vector_dsc_t * dsc, float cx, float cy, float radius)
{
    if(radius <= 0) {
        LV_LOG_ERROR("Radius must be greater than 0, radius: %f", radius);
        return;
    }

    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    lv_vector_gradient_t * gradient = &dsc->current_dsc->fill_dsc->draw_attrs.gradient;
    dsc->current_dsc->fill_dsc->style = LV_VECTOR_DRAW_STYLE_GRADIENT;
    gradient->style = LV_VECTOR_GRADIENT_STYLE_RADIAL;
    gradient->cx = cx;
    gradient->cy = cy;
    gradient->cr = radius;
    gradient->stops_count = 0;
    gradient->spread = LV_VECTOR_GRADIENT_SPREAD_PAD;
}

void lv_vector_dsc_set_fill_gradient_spread(lv_vector_dsc_t * dsc, lv_vector_gradient_spread_t spread)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);
    dsc->current_dsc->fill_dsc->draw_attrs.gradient.spread = spread;
}

void lv_vector_dsc_set_fill_gradient_color_stops(lv_vector_dsc_t * dsc, const lv_gradient_stop_t * stops,
                                                 uint16_t count)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    if(count > LV_GRADIENT_MAX_STOPS) {
        LV_LOG_WARN("Gradient stops limited: %d, max: %d", count, LV_GRADIENT_MAX_STOPS);
        count = LV_GRADIENT_MAX_STOPS;
    }

    lv_memcpy(&(dsc->current_dsc->fill_dsc->draw_attrs.gradient.stops), stops, sizeof(lv_gradient_stop_t) * count);
    dsc->current_dsc->fill_dsc->draw_attrs.gradient.stops_count = count;
}

void lv_vector_dsc_set_fill_transform(lv_vector_dsc_t * dsc, const lv_matrix_t * matrix)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);

    lv_memcpy(&(dsc->current_dsc->fill_dsc->matrix), matrix, sizeof(lv_matrix_t));
}

void lv_vector_dsc_set_stroke_transform(lv_vector_dsc_t * dsc, const lv_matrix_t * matrix)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    lv_memcpy(&(dsc->current_dsc->stroke_dsc->matrix), matrix, sizeof(lv_matrix_t));
}

void lv_vector_dsc_set_stroke_color32(lv_vector_dsc_t * dsc, lv_color32_t color)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    dsc->current_dsc->stroke_dsc->style = LV_VECTOR_DRAW_STYLE_SOLID;
    dsc->current_dsc->stroke_dsc->draw_attrs.color = color;
}

void lv_vector_dsc_set_stroke_color(lv_vector_dsc_t * dsc, lv_color_t color)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    dsc->current_dsc->stroke_dsc->style = LV_VECTOR_DRAW_STYLE_SOLID;
    dsc->current_dsc->stroke_dsc->draw_attrs.color = lv_color_to_32(color, 0xFF);
}

void lv_vector_dsc_set_stroke_opa(lv_vector_dsc_t * dsc, lv_opa_t opa)
{
    if(dsc->current_dsc->stroke_dsc->opa == opa) return;

    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    dsc->current_dsc->stroke_dsc->opa = opa;
}

void lv_vector_dsc_set_stroke_width(lv_vector_dsc_t * dsc, float width)
{
    if(width < 0.0f) {
        LV_LOG_ERROR("Stroke width must be greater than 0, width: %f", width);
        return;
    }

    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    dsc->current_dsc->stroke_dsc->width = width;
    dsc->current_dsc->stroke_dsc->stroke_dsc_changed = true;
}

void lv_vector_dsc_set_stroke_dash(lv_vector_dsc_t * dsc, float * dash_pattern, uint16_t dash_count)
{
    if(!dsc || !dsc->current_dsc || !dsc->tasks.draw_task_list.allocator) {
        return;
    }

    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator,
                                             &dsc->current_dsc->stroke_dsc);

    if(!dash_pattern || dash_count == 0) {
        dsc->current_dsc->stroke_dsc->dash_count = 0;
        return;
    }

    dash_count = MIN(dash_count, DASH_MAX);
    uint16_t final_count = (dash_count % 2 != 0) ? MIN(dash_count * 2, DASH_MAX) : dash_count;

    float * new_pattern = dsc->tasks.draw_task_list.allocator->alloc(
                              dsc->tasks.draw_task_list.allocator, final_count * sizeof(float));
    if(!new_pattern) {
        LV_LOG_ERROR("Failed to allocate memory for dash pattern");
        return;
    }

    if(dash_count % 2 != 0) {
        for(uint16_t i = 0; i < final_count; i++) {
            new_pattern[i] = dash_pattern[i % dash_count];
        }
    }
    else {
        lv_memcpy(new_pattern, dash_pattern, final_count * sizeof(float));
    }

    dsc->current_dsc->stroke_dsc->dash_pattern = new_pattern;
    dsc->current_dsc->stroke_dsc->dash_count = final_count;
    dsc->current_dsc->stroke_dsc->stroke_dsc_changed = true;
}

void lv_vector_dsc_set_stroke_cap(lv_vector_dsc_t * dsc, lv_vector_stroke_cap_t cap)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);
    dsc->current_dsc->stroke_dsc->cap = cap;
    dsc->current_dsc->stroke_dsc->stroke_dsc_changed = true;
}

void lv_vector_dsc_set_stroke_join(lv_vector_dsc_t * dsc, lv_vector_stroke_join_t join)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);
    dsc->current_dsc->stroke_dsc->join = join;
    dsc->current_dsc->stroke_dsc->stroke_dsc_changed = true;
}

void lv_vector_dsc_set_stroke_miter_limit(lv_vector_dsc_t * dsc, uint16_t miter_limit)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);
    dsc->current_dsc->stroke_dsc->miter_limit = miter_limit;
    dsc->current_dsc->stroke_dsc->stroke_dsc_changed = true;
}

void lv_vector_dsc_set_stroke_linear_gradient(lv_vector_dsc_t * dsc, float x1, float y1, float x2, float y2)
{
    if(x1 == x2 && y1 == y2) {
        LV_LOG_ERROR("Gradient start and end points are the same, (%f, %f)->(%f, %f)", x1, y1, x2, y2);
        return;
    }

    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);
    dsc->current_dsc->stroke_dsc->style = LV_VECTOR_DRAW_STYLE_GRADIENT;

    lv_vector_gradient_t * gradient = &dsc->current_dsc->stroke_dsc->draw_attrs.gradient;
    gradient->style = LV_VECTOR_GRADIENT_STYLE_LINEAR;
    gradient->x1 = x1;
    gradient->y1 = y1;
    gradient->x2 = x2;
    gradient->y2 = y2;
    gradient->stops_count = 0;
    gradient->spread = LV_VECTOR_GRADIENT_SPREAD_PAD;
}

void lv_vector_dsc_set_stroke_radial_gradient(lv_vector_dsc_t * dsc, float cx, float cy, float radius)
{
    if(radius <= 0) {
        LV_LOG_ERROR("Radius must be greater than 0, radius: %f", radius);
        return;
    }

    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    dsc->current_dsc->stroke_dsc->style = LV_VECTOR_DRAW_STYLE_GRADIENT;

    lv_vector_gradient_t * gradient = &dsc->current_dsc->stroke_dsc->draw_attrs.gradient;
    gradient->style = LV_VECTOR_GRADIENT_STYLE_RADIAL;
    gradient->cx = cx;
    gradient->cy = cy;
    gradient->cr = radius;
    gradient->stops_count = 0;
    gradient->spread = LV_VECTOR_GRADIENT_SPREAD_PAD;
}

void lv_vector_dsc_set_stroke_gradient_spread(lv_vector_dsc_t * dsc, lv_vector_gradient_spread_t spread)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);
    dsc->current_dsc->stroke_dsc->style = LV_VECTOR_DRAW_STYLE_GRADIENT;

    lv_vector_gradient_t * gradient = &dsc->current_dsc->stroke_dsc->draw_attrs.gradient;
    gradient->spread = spread;
}

void lv_vector_dsc_set_stroke_gradient_color_stops(lv_vector_dsc_t * dsc, const lv_gradient_stop_t * stops,
                                                   uint16_t count)
{
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    if(count > LV_GRADIENT_MAX_STOPS) {
        LV_LOG_WARN("Gradient stops limited: %d, max: %d", count, LV_GRADIENT_MAX_STOPS);
        count = LV_GRADIENT_MAX_STOPS;
    }

    lv_vector_gradient_t * gradient = &dsc->current_dsc->stroke_dsc->draw_attrs.gradient;
    gradient->stops_count = count;
    lv_memcpy(gradient->stops, stops, sizeof(lv_gradient_stop_t) * count);
}

lv_vector_draw_dsc_t * lv_vector_dsc_get_current_dsc(const lv_vector_dsc_t * dsc)
{
    return dsc->current_dsc;
}

void lv_vector_dsc_set_current_dsc(const lv_vector_dsc_t * dsc, lv_vector_draw_dsc_t * draw_dsc)
{
    lv_vector_dsc_fill_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->fill_dsc);
    lv_vector_dsc_stroke_ensure_write_access(dsc->tasks.draw_task_list.allocator, &dsc->current_dsc->stroke_dsc);

    _copy_fill_dsc(dsc->current_dsc->fill_dsc, draw_dsc->fill_dsc);

    if(draw_dsc->stroke_dsc->dash_count > 0) {
        dsc->current_dsc->stroke_dsc->dash_pattern =
            dsc->tasks.draw_task_list.allocator->alloc(dsc->tasks.draw_task_list.allocator,
                                                       sizeof(float) * draw_dsc->stroke_dsc->dash_count);
    }
    _copy_stroke_dsc(dsc->current_dsc->stroke_dsc, draw_dsc->stroke_dsc);

    dsc->current_dsc->fill_dsc->use_count = 0;
    dsc->current_dsc->stroke_dsc->use_count = 0;
    dsc->current_dsc->stroke_dsc->stroke_dsc_changed = false;
}

#if LV_USE_VECTOR_DUMP_INFO
void lv_vector_dump_info(const lv_vector_dsc_t * dsc)
{
    lv_vector_for_each_task_dump_info(&dsc->tasks.draw_task_list);
}
#endif

/* draw functions */
void lv_vector_dsc_add_path(lv_vector_dsc_t * dsc, const lv_vector_path_t * path)
{
    lv_area_t rect;
    if(!_lv_area_intersect(&rect, &(dsc->layer->_clip_area), &(dsc->current_dsc->scissor_area))) {
        return;
    }

    if(dsc->current_dsc->fill_dsc->opa == 0
       && dsc->current_dsc->stroke_dsc->opa == 0) {
        return;
    }

    if(lv_vector_path_is_empty(path)) {
        return;
    }

    if(!dsc->tasks.draw_task_list.task_list) {
        dsc->tasks.draw_task_list.task_list = lv_malloc(sizeof(lv_ll_t));
        LV_ASSERT_MALLOC(dsc->tasks.draw_task_list.task_list);
        _lv_ll_init(dsc->tasks.draw_task_list.task_list, sizeof(_lv_vector_draw_task));
    }

    _lv_vector_draw_task * new_task = (_lv_vector_draw_task *)_lv_ll_ins_tail(dsc->tasks.draw_task_list.task_list);
    lv_memset(new_task, 0, sizeof(_lv_vector_draw_task));

    new_task->path_impl = path->impl;
    lv_vector_path_ref(new_task->path_impl);

    new_task->dsc = dsc->tasks.draw_task_list.allocator->alloc(dsc->tasks.draw_task_list.allocator,
                                                               sizeof(lv_vector_draw_dsc_t));
    LV_ASSERT_MALLOC(new_task->dsc);

    new_task->dsc->fill_dsc = dsc->current_dsc->fill_dsc;
    new_task->dsc->stroke_dsc = dsc->current_dsc->stroke_dsc;
    new_task->dsc->scissor_area = rect;
    lv_memcpy(&(new_task->dsc->matrix), &(dsc->current_dsc->matrix), sizeof(lv_matrix_t));
    new_task->dsc->blend_mode = dsc->current_dsc->blend_mode;

    new_task->dsc->fill_dsc->use_count++;
    new_task->dsc->stroke_dsc->use_count++;
}

void lv_vector_clear_area(lv_vector_dsc_t * dsc, const lv_area_t * rect)
{
    lv_area_t r;
    if(!_lv_area_intersect(&r, &(dsc->layer->_clip_area), &(dsc->current_dsc->scissor_area))) {
        return;
    }

    lv_area_t final_rect;
    if(!_lv_area_intersect(&final_rect, &r, rect)) {
        return;
    }

    if(!dsc->tasks.draw_task_list.task_list) {
        dsc->tasks.draw_task_list.task_list = lv_malloc(sizeof(lv_ll_t));
        LV_ASSERT_MALLOC(dsc->tasks.draw_task_list.task_list);
        _lv_ll_init(dsc->tasks.draw_task_list.task_list, sizeof(_lv_vector_draw_task));
    }

    _lv_vector_draw_task * new_task = (_lv_vector_draw_task *)_lv_ll_ins_tail(dsc->tasks.draw_task_list.task_list);
    lv_memset(new_task, 0, sizeof(_lv_vector_draw_task));

    new_task->dsc = dsc->tasks.draw_task_list.allocator->alloc(dsc->tasks.draw_task_list.allocator,
                                                               sizeof(lv_vector_draw_dsc_t));
    LV_ASSERT_MALLOC(new_task->dsc);
    new_task->dsc->fill_dsc = dsc->tasks.draw_task_list.allocator->alloc(dsc->tasks.draw_task_list.allocator,
                                                                         sizeof(lv_vector_fill_dsc_t));
    LV_ASSERT_MALLOC(new_task->dsc->fill_dsc);

    new_task->dsc->fill_dsc->draw_attrs.color = dsc->current_dsc->fill_dsc->draw_attrs.color;
    new_task->dsc->fill_dsc->opa = dsc->current_dsc->fill_dsc->opa;
    lv_area_copy(&(new_task->dsc->scissor_area), &final_rect);
}

void lv_draw_vector(lv_vector_dsc_t * dsc)
{
    if(!dsc->tasks.draw_task_list.task_list) {
        return;
    }

    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, LV_VECTOR_ALLOCATOR_INIT_SIZE);
    LV_ASSERT_MALLOC(allocator);

    lv_vector_fill_dsc_t * fill_dsc = allocator->alloc(allocator, sizeof(lv_vector_fill_dsc_t));
    LV_ASSERT_MALLOC(fill_dsc);

    lv_vector_stroke_dsc_t * stroke_dsc = allocator->alloc(allocator, sizeof(lv_vector_stroke_dsc_t));
    LV_ASSERT_MALLOC(stroke_dsc);

    if(dsc->current_dsc->stroke_dsc->dash_count > 0) {
        stroke_dsc->dash_pattern = allocator->alloc(allocator, dsc->current_dsc->stroke_dsc->dash_count * sizeof(float));
        LV_ASSERT_MALLOC(stroke_dsc->dash_pattern);
    }

    _copy_fill_dsc(fill_dsc, dsc->current_dsc->fill_dsc);
    _copy_stroke_dsc(stroke_dsc, dsc->current_dsc->stroke_dsc);
    fill_dsc->use_count = 0;
    stroke_dsc->use_count = 0;
    stroke_dsc->stroke_dsc_changed = false;

    dsc->current_dsc->fill_dsc = fill_dsc;
    dsc->current_dsc->stroke_dsc = stroke_dsc;

    lv_layer_t * layer = dsc->layer;

    lv_draw_task_t * t = lv_draw_add_task(layer, &(layer->_clip_area), LV_DRAW_TASK_TYPE_VECTOR);
    lv_memcpy(t->draw_dsc, &(dsc->tasks), sizeof(lv_draw_vector_task_dsc_t));
    lv_draw_finalize_task_creation(layer, t);
    dsc->tasks.draw_task_list.task_list = NULL;
    dsc->tasks.draw_task_list.allocator = allocator;
}

/* draw dsc transform */
void lv_vector_dsc_identity(lv_vector_dsc_t * dsc)
{
    lv_matrix_identity(&(dsc->current_dsc->matrix)); /*identity matrix*/
}

void lv_vector_dsc_scale(lv_vector_dsc_t * dsc, float scale_x, float scale_y)
{
    lv_matrix_scale(&(dsc->current_dsc->matrix), scale_x, scale_y);
}

void lv_vector_dsc_rotate(lv_vector_dsc_t * dsc, float degree)
{
    lv_matrix_rotate(&(dsc->current_dsc->matrix), degree);
}

void lv_vector_dsc_translate(lv_vector_dsc_t * dsc, float tx, float ty)
{
    lv_matrix_translate(&(dsc->current_dsc->matrix), tx, ty);
}

void lv_vector_dsc_skew(lv_vector_dsc_t * dsc, float skew_x, float skew_y)
{
    lv_matrix_skew(&(dsc->current_dsc->matrix), skew_x, skew_y);
}

#endif /* LV_USE_VECTOR_GRAPHIC_OPTIMIZE */
