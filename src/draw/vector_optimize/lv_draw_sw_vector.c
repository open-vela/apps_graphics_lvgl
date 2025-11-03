/**
 * @file lv_draw_sw_vector.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../sw/lv_draw_sw.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE && LV_USE_THORVG && !LV_USE_VG_LITE_THORVG
#if LV_USE_THORVG_EXTERNAL
    #include <thorvg_capi.h>
#else
    #include "../../libs/thorvg/thorvg_capi.h"
#endif
#include "../../stdlib/lv_string.h"
#include "lv_vector_polygon.h"
#include <math.h>
#include <float.h>

/*********************
 *      DEFINES
 *********************/

#define LV_SW_PATH_CAST(ptr)  (lv_platform_sw_path_t *)(ptr)

#define LV_SW_PATH_ARRAY_DEFAULT_SIZE 8

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    float x;
    float y;
    float w;
    float h;
} _tvg_rect;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} _tvg_color;

typedef struct {
    Tvg_Canvas * canvas;
    int32_t partial_y_offset;
    int32_t translate_x;
    int32_t translate_y;
    lv_opa_t opa;
} _tvg_draw_state;

typedef struct _lv_platform_sw_path_t {
    lv_platform_path_base_t base;
    lv_array_t ops;
    lv_array_t points;
} lv_platform_sw_path_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

static void _lv_area_to_tvg(_tvg_rect * rect, const lv_area_t * area)
{
    rect->x = area->x1;
    rect->y = area->y1;
    rect->w = area->x2 - area->x1;
    rect->h = area->y2 - area->y1;
}

static void _lv_color_to_tvg(_tvg_color * color, const lv_color32_t * c, lv_opa_t opa)
{
    color->r = c->red;
    color->g = c->green;
    color->b = c->blue;
    color->a = LV_OPA_MIX2(c->alpha, opa);
}

static void _lv_matrix_to_tvg(Tvg_Matrix * tm, const lv_matrix_t * m)
{
    tm->e11 = m->m[0][0];
    tm->e12 = m->m[0][1];
    tm->e13 = m->m[0][2];
    tm->e21 = m->m[1][0];
    tm->e22 = m->m[1][1];
    tm->e23 = m->m[1][2];
    tm->e31 = m->m[2][0];
    tm->e32 = m->m[2][1];
    tm->e33 = m->m[2][2];
}

static void _set_paint_matrix(Tvg_Paint * obj, const Tvg_Matrix * m)
{
    tvg_paint_set_transform(obj, m);
}

static void _set_paint_shape(Tvg_Paint * obj, const lv_platform_sw_path_t * p)
{
    uint32_t pidx = 0;
    lv_vector_path_op_t * op = lv_array_front(&p->ops);
    uint32_t size = lv_array_size(&p->ops);
    for(uint32_t i = 0; i < size; i++) {
        switch(op[i]) {
            case LV_VECTOR_PATH_OP_MOVE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&p->points, pidx);
                    tvg_shape_move_to(obj, pt->x, pt->y);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_LINE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&p->points, pidx);
                    tvg_shape_line_to(obj, pt->x, pt->y);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_QUAD_TO: {
                    LV_ASSERT(pidx > 0);
                    lv_fpoint_t * pt1 = lv_array_at(&p->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&p->points, pidx + 1);

                    lv_fpoint_t * last_pt = lv_array_at(&p->points, pidx - 1);

                    lv_fpoint_t cp[2];
                    cp[0].x = (last_pt->x + 2 * pt1->x) * (1.0f / 3.0f);
                    cp[0].y = (last_pt->y + 2 * pt1->y) * (1.0f / 3.0f);
                    cp[1].x = (pt2->x + 2 * pt1->x) * (1.0f / 3.0f);
                    cp[1].y = (pt2->y + 2 * pt1->y) * (1.0f / 3.0f);

                    tvg_shape_cubic_to(obj, cp[0].x, cp[0].y, cp[1].x, cp[1].y, pt2->x, pt2->y);
                    pidx += 2;
                }
                break;
            case LV_VECTOR_PATH_OP_CUBIC_TO: {
                    lv_fpoint_t * pt1 = lv_array_at(&p->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&p->points, pidx + 1);
                    lv_fpoint_t * pt3 = lv_array_at(&p->points, pidx + 2);

                    tvg_shape_cubic_to(obj, pt1->x, pt1->y, pt2->x, pt2->y, pt3->x, pt3->y);
                    pidx += 3;
                }
                break;
            case LV_VECTOR_PATH_OP_CLOSE: {
                    tvg_shape_close(obj);
                }
                break;
        }
    }
}

static Tvg_Stroke_Cap _lv_stroke_cap_to_tvg(lv_vector_stroke_cap_t cap)
{
    switch(cap) {
        case LV_VECTOR_STROKE_CAP_SQUARE:
            return TVG_STROKE_CAP_SQUARE;
        case LV_VECTOR_STROKE_CAP_ROUND:
            return TVG_STROKE_CAP_ROUND;
        case LV_VECTOR_STROKE_CAP_BUTT:
            return TVG_STROKE_CAP_BUTT;
        default:
            return TVG_STROKE_CAP_SQUARE;
    }
}

static Tvg_Stroke_Join _lv_stroke_join_to_tvg(lv_vector_stroke_join_t join)
{
    switch(join) {
        case LV_VECTOR_STROKE_JOIN_BEVEL:
            return TVG_STROKE_JOIN_BEVEL;
        case LV_VECTOR_STROKE_JOIN_ROUND:
            return TVG_STROKE_JOIN_ROUND;
        case LV_VECTOR_STROKE_JOIN_MITER:
            return TVG_STROKE_JOIN_MITER;
        default:
            return TVG_STROKE_JOIN_BEVEL;
    }
}

static Tvg_Stroke_Fill _lv_spread_to_tvg(lv_vector_gradient_spread_t sp)
{
    switch(sp) {
        case LV_VECTOR_GRADIENT_SPREAD_PAD:
            return TVG_STROKE_FILL_PAD;
        case LV_VECTOR_GRADIENT_SPREAD_REPEAT:
            return TVG_STROKE_FILL_REPEAT;
        case LV_VECTOR_GRADIENT_SPREAD_REFLECT:
            return TVG_STROKE_FILL_REFLECT;
        default:
            return TVG_STROKE_FILL_PAD;
    }
}

static void _setup_gradient(Tvg_Gradient * gradient, const lv_vector_gradient_t * grad,
                            const lv_matrix_t * matrix)
{
    Tvg_Color_Stop * stops = (Tvg_Color_Stop *)lv_malloc(sizeof(Tvg_Color_Stop) * grad->stops_count);
    LV_ASSERT_MALLOC(stops);
    for(uint16_t i = 0; i < grad->stops_count; i++) {
        const lv_gradient_stop_t * s = &(grad->stops[i]);

        stops[i].offset = s->frac / 255.0f;
        stops[i].r = s->color.red;
        stops[i].g = s->color.green;
        stops[i].b = s->color.blue;
        stops[i].a = s->opa;
    }

    tvg_gradient_set_color_stops(gradient, stops, grad->stops_count);
    tvg_gradient_set_spread(gradient, _lv_spread_to_tvg(grad->spread));
    Tvg_Matrix mtx;
    _lv_matrix_to_tvg(&mtx, matrix);
    tvg_gradient_set_transform(gradient, &mtx);
    lv_free(stops);
}

static void _set_paint_stroke_gradient(Tvg_Paint * obj, const lv_vector_gradient_t * g, const lv_matrix_t * m)
{
    Tvg_Gradient * grad = NULL;
    if(g->style == LV_VECTOR_GRADIENT_STYLE_RADIAL) {
        grad = tvg_radial_gradient_new();
        tvg_radial_gradient_set(grad, g->cx, g->cy, g->cr);
        _setup_gradient(grad, g, m);
        tvg_shape_set_stroke_radial_gradient(obj, grad);
    }
    else {
        grad = tvg_linear_gradient_new();
        tvg_linear_gradient_set(grad, g->x1, g->y1, g->x2, g->y2);
        _setup_gradient(grad, g, m);
        tvg_shape_set_stroke_linear_gradient(obj, grad);
    }
}

static void _set_paint_stroke(Tvg_Paint * obj, const lv_vector_stroke_dsc_t * dsc)
{
    if(dsc->style == LV_VECTOR_DRAW_STYLE_SOLID) {
        _tvg_color c;
        _lv_color_to_tvg(&c, &dsc->draw_attrs.color, dsc->opa);
        tvg_shape_set_stroke_color(obj, c.r, c.g, c.b, c.a);
    }
    else {   /*gradient*/
        _set_paint_stroke_gradient(obj, &dsc->draw_attrs.gradient, &dsc->matrix);
    }

    tvg_shape_set_stroke_width(obj, dsc->width);
    tvg_shape_set_stroke_miterlimit(obj, dsc->miter_limit);
    tvg_shape_set_stroke_cap(obj, _lv_stroke_cap_to_tvg(dsc->cap));
    tvg_shape_set_stroke_join(obj, _lv_stroke_join_to_tvg(dsc->join));

    if(dsc->dash_count > 0) {
        tvg_shape_set_stroke_dash(obj, dsc->dash_pattern, dsc->dash_count);
    }
}

static Tvg_Fill_Rule _lv_fill_rule_to_tvg(lv_vector_fill_t rule)
{
    switch(rule) {
        case LV_VECTOR_FILL_NONZERO:
            return TVG_FILL_RULE_WINDING;
        case LV_VECTOR_FILL_EVENODD:
            return TVG_FILL_RULE_EVEN_ODD;
        default:
            return TVG_FILL_RULE_WINDING;
    }
}

static void _set_paint_fill_gradient(Tvg_Paint * obj, const lv_vector_gradient_t * g, const lv_matrix_t * m)
{
    Tvg_Gradient * grad = NULL;
    if(g->style == LV_VECTOR_GRADIENT_STYLE_RADIAL) {
        grad = tvg_radial_gradient_new();
        tvg_radial_gradient_set(grad, g->cx, g->cy, g->cr);
        _setup_gradient(grad, g, m);
        tvg_shape_set_radial_gradient(obj, grad);
    }
    else {
        grad = tvg_linear_gradient_new();
        tvg_linear_gradient_set(grad, g->x1, g->y1, g->x2, g->y2);
        _setup_gradient(grad, g, m);
        tvg_shape_set_linear_gradient(obj, grad);
    }
}

static void _set_paint_fill_pattern(Tvg_Paint * obj, Tvg_Canvas * canvas, const lv_draw_image_dsc_t * p,
                                    const lv_matrix_t * m, const lv_opa_t opa)
{
    lv_image_decoder_dsc_t decoder_dsc;
    lv_image_decoder_args_t args = { 0 };
    lv_result_t res = lv_image_decoder_open(&decoder_dsc, p->src, &args);
    if(res != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to open image");
        return;
    }

    if(!decoder_dsc.decoded) {
        lv_image_decoder_close(&decoder_dsc);
        LV_LOG_ERROR("Image not ready");
        return;
    }

    const lv_image_header_t * header = &decoder_dsc.decoded->header;
    lv_color_format_t cf = header->cf;

    if(cf != LV_COLOR_FORMAT_ARGB8888) {
        lv_image_decoder_close(&decoder_dsc);
        LV_LOG_ERROR("Not support color format: %d", cf);
        return;
    }

    const uint32_t tvg_stride = header->w * sizeof(uint32_t);
    if(header->stride != tvg_stride) {
        LV_LOG_WARN("img_stride != tvg_stride (%" LV_PRIu32 " != %" LV_PRIu32 "), width = %" LV_PRIu32,
                    (uint32_t)header->stride,
                    tvg_stride, (uint32_t)header->w);
        lv_result_t result = lv_draw_buf_adjust_stride((lv_draw_buf_t *)decoder_dsc.decoded, tvg_stride);
        if(result != LV_RESULT_OK) {
            lv_image_decoder_close(&decoder_dsc);
            LV_LOG_ERROR("Failed to adjust stride");
            return;
        }
    }

    Tvg_Paint * img = tvg_picture_new();
    tvg_picture_load_raw(img, (uint32_t *)decoder_dsc.decoded->data, header->w, header->h, true);
    Tvg_Paint * clip_path = tvg_paint_duplicate(obj);
    tvg_paint_set_composite_method(img, clip_path, TVG_COMPOSITE_METHOD_CLIP_PATH);
    tvg_paint_set_opacity(img, LV_UDIV255(p->opa * opa));

    Tvg_Matrix mtx;
    _lv_matrix_to_tvg(&mtx, m);
    tvg_paint_set_transform(img, &mtx);
    tvg_canvas_push(canvas, img);
    lv_image_decoder_close(&decoder_dsc);
}

static void _set_paint_fill(Tvg_Paint * obj, Tvg_Canvas * canvas, const lv_vector_fill_dsc_t * dsc,
                            const lv_matrix_t * matrix, const lv_opa_t opa)
{
    tvg_shape_set_fill_rule(obj, _lv_fill_rule_to_tvg(dsc->fill_rule));

    if(dsc->style == LV_VECTOR_DRAW_STYLE_SOLID) {
        _tvg_color c;
        _lv_color_to_tvg(&c, &dsc->draw_attrs.color, dsc->opa);
        tvg_shape_set_fill_color(obj, c.r, c.g, c.b, c.a);
    }
    else if(dsc->style == LV_VECTOR_DRAW_STYLE_PATTERN) {
        lv_matrix_t imx = *matrix;

        if(dsc->fill_units == LV_VECTOR_FILL_UNITS_OBJECT_BOUNDING_BOX) {
            /* Convert to object bounding box coordinates */
            float x, y, w, h;
            tvg_paint_get_bounds(obj, &x, &y, &w, &h, false);
            lv_matrix_translate(&imx, x, y);
        }

        lv_matrix_multiply(&imx, &dsc->matrix);
        _set_paint_fill_pattern(obj, canvas, &dsc->draw_attrs.img_dsc, &imx, opa);
    }
    else if(dsc->style == LV_VECTOR_DRAW_STYLE_GRADIENT) {
        _set_paint_fill_gradient(obj, &dsc->draw_attrs.gradient, &dsc->matrix);
    }
}

static Tvg_Blend_Method _lv_blend_to_tvg(lv_vector_blend_t blend)
{
    switch(blend) {
        case LV_VECTOR_BLEND_SRC_OVER:
            return TVG_BLEND_METHOD_NORMAL;
        case LV_VECTOR_BLEND_SCREEN:
            return TVG_BLEND_METHOD_SCREEN;
        case LV_VECTOR_BLEND_MULTIPLY:
            return TVG_BLEND_METHOD_MULTIPLY;
        case LV_VECTOR_BLEND_NONE:
            return TVG_BLEND_METHOD_SRCOVER;
        case LV_VECTOR_BLEND_ADDITIVE:
            return TVG_BLEND_METHOD_ADD;
        case LV_VECTOR_BLEND_DARKEN:
            return TVG_BLEND_METHOD_DARKEN;
        case LV_VECTOR_BLEND_LIGHTEN:
            return TVG_BLEND_METHOD_LIGHTEN;
        case LV_VECTOR_BLEND_HARDLIGHT:
            return TVG_BLEND_METHOD_HARDLIGHT;
        case LV_VECTOR_BLEND_SOFTLIGHT:
            return TVG_BLEND_METHOD_SOFTLIGHT;
        case LV_VECTOR_BLEND_OVERLAY:
            return TVG_BLEND_METHOD_OVERLAY;
        case LV_VECTOR_BLEND_COLORBURN:
            return TVG_BLEND_METHOD_COLORBURN;
        case LV_VECTOR_BLEND_COLORDODGE:
            return TVG_BLEND_METHOD_COLORDODGE;
        case LV_VECTOR_BLEND_DIFFERENCE:
            return TVG_BLEND_METHOD_DIFFERENCE;
        case LV_VECTOR_BLEND_EXCLUSION:
            return TVG_BLEND_METHOD_EXCLUSION;
        case LV_VECTOR_BLEND_SRC_IN:
        case LV_VECTOR_BLEND_DST_OVER:
        case LV_VECTOR_BLEND_DST_IN:
        case LV_VECTOR_BLEND_SUBTRACTIVE:
        /*not support yet.*/
        default:
            return TVG_BLEND_METHOD_NORMAL;
    }
}

static void _set_paint_blend_mode(Tvg_Paint * obj, lv_vector_blend_t blend)
{
    tvg_paint_set_blend_method(obj, _lv_blend_to_tvg(blend));
}

static void _task_draw_cb(void * ctx, const lv_platform_path_base_t * path_impl, const lv_vector_draw_dsc_t * dsc)
{
    lv_platform_sw_path_t * path = (lv_platform_sw_path_t *)path_impl;
    _tvg_draw_state * state = (_tvg_draw_state *)ctx;
    Tvg_Canvas * canvas = (Tvg_Canvas *)state->canvas;

    Tvg_Paint * obj = tvg_shape_new();

    if(!path) {  /*clear*/
        _tvg_rect rc;
        _lv_area_to_tvg(&rc, &dsc->scissor_area);

        _tvg_color c;
        _lv_color_to_tvg(&c, &dsc->fill_dsc->draw_attrs.color, dsc->fill_dsc->opa);

        Tvg_Matrix mtx = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
        };
        _set_paint_matrix(obj, &mtx);
        tvg_shape_append_rect(obj, rc.x + state->translate_x, rc.y + state->translate_y, rc.w, rc.h, 0, 0);
        tvg_shape_set_fill_color(obj, c.r, c.g, c.b, c.a);
    }
    else {
        lv_matrix_t matrix;
        lv_matrix_identity(&matrix);
        lv_matrix_translate(&matrix, state->translate_x, state->translate_y);
        lv_matrix_multiply(&matrix, &dsc->matrix);

        Tvg_Matrix mtx;
        _lv_matrix_to_tvg(&mtx, &matrix);
        _set_paint_matrix(obj, &mtx);

        _set_paint_shape(obj, path);

        if(dsc->fill_dsc->opa > 0) {
            _set_paint_fill(obj, canvas, dsc->fill_dsc, &matrix, state->opa);
        }

        if(dsc->stroke_dsc->opa > 0) {
            _set_paint_stroke(obj, dsc->stroke_dsc);
        }
        _set_paint_blend_mode(obj, dsc->blend_mode);
    }

    tvg_paint_set_opacity(obj, state->opa);
    tvg_canvas_push(canvas, obj);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_sw_vector(lv_draw_unit_t * draw_unit, const lv_draw_vector_task_dsc_t * dsc)
{
    lv_draw_task_t * t = ((lv_draw_sw_unit_t *)draw_unit)->task_act;

    if(dsc->draw_task_list.task_list == NULL)
        return;

    lv_layer_t * layer = dsc->base.layer;
    lv_draw_buf_t * draw_buf = layer->draw_buf;
    if(draw_buf == NULL)
        return;

    lv_color_format_t cf = draw_buf->header.cf;

    if(cf != LV_COLOR_FORMAT_ARGB8888 && \
       cf != LV_COLOR_FORMAT_XRGB8888) {
        LV_LOG_ERROR("unsupported layer color: %d", cf);
        return;
    }

    void * buf = draw_buf->data;
    int32_t width = lv_area_get_width(&layer->buf_area);
    int32_t height = lv_area_get_height(&layer->buf_area);
    uint32_t stride = draw_buf->header.stride;
    Tvg_Canvas * canvas = tvg_swcanvas_create();
    tvg_swcanvas_set_target(canvas, buf, stride / 4, width, height, TVG_COLORSPACE_ARGB8888);

    _tvg_draw_state state = {canvas, 0, -layer->buf_area.x1, -layer->buf_area.y1, t->opa};
    _lv_vector_for_each_destroy_tasks((lv_vector_draw_task_list_t *)&dsc->draw_task_list, _task_draw_cb, &state);

    if(tvg_canvas_draw(canvas) == TVG_RESULT_SUCCESS) {
        tvg_canvas_sync(canvas);
    }

    tvg_canvas_destroy(canvas);
}

static struct lv_platform_path_base_t * lv_sw_path_create_cb(lv_vector_path_quality_t quality)
{
    LV_UNUSED(quality);
    lv_platform_sw_path_t * path = lv_malloc_zeroed(sizeof(lv_platform_sw_path_t));
    LV_ASSERT_MALLOC(path);
    lv_array_init(&path->ops, LV_SW_PATH_ARRAY_DEFAULT_SIZE, sizeof(uint8_t));
    lv_array_init(&path->points, LV_SW_PATH_ARRAY_DEFAULT_SIZE, sizeof(lv_fpoint_t));
    return (lv_platform_path_base_t *)path;
}

static void lv_sw_path_destroy_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    lv_array_deinit(&path->ops);
    lv_array_deinit(&path->points);
    lv_free(path);
}

static struct lv_platform_path_base_t * lv_sw_path_clone_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * src = LV_SW_PATH_CAST(self);
    lv_platform_sw_path_t * dst = (lv_platform_sw_path_t *)lv_sw_path_create_cb(LV_VECTOR_PATH_QUALITY_MEDIUM);

    lv_array_copy(&dst->ops, &src->ops);
    lv_array_copy(&dst->points, &src->points);
    return (lv_platform_path_base_t *)dst;
}

static void lv_sw_path_concat_cb(struct lv_platform_path_base_t * self, struct lv_platform_path_base_t * other)
{
    lv_platform_sw_path_t * dst = LV_SW_PATH_CAST(self);
    lv_platform_sw_path_t * src = LV_SW_PATH_CAST(other);
    lv_array_concat(&dst->ops, &src->ops);
    lv_array_concat(&dst->points, &src->points);
}

static void lv_sw_path_move_to_cb(struct lv_platform_path_base_t * self, const lv_fpoint_t * p)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    CHECK_AND_RESIZE_PATH_CONTAINER(path, 1);

    uint8_t op = LV_VECTOR_PATH_OP_MOVE_TO;
    OP_PUSH_BACK(&path->ops, &op);
    POINT_PUSH_BACK(&path->points, p);
}

static void lv_sw_path_line_to_cb(struct lv_platform_path_base_t * self, const lv_fpoint_t * p)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    if(lv_array_is_empty(&path->ops)) {
        return;
    }

    CHECK_AND_RESIZE_PATH_CONTAINER(path, 1);
    uint8_t op = LV_VECTOR_PATH_OP_LINE_TO;
    OP_PUSH_BACK(&path->ops, &op);
    POINT_PUSH_BACK(&path->points, p);
}

static void lv_sw_path_quad_to_cb(struct lv_platform_path_base_t * self, const lv_fpoint_t * p1, const lv_fpoint_t * p2)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    if(lv_array_is_empty(&path->ops)) {
        /*first op must be move_to*/
        return;
    }

    CHECK_AND_RESIZE_PATH_CONTAINER(path, 2);

    uint8_t op = LV_VECTOR_PATH_OP_QUAD_TO;
    OP_PUSH_BACK(&path->ops, &op);
    POINT2_PUSH_BACK(&path->points, p1, p2);
}

static void lv_sw_path_cubic_to_cb(struct lv_platform_path_base_t * self,
                                   const lv_fpoint_t * p1, const lv_fpoint_t * p2,
                                   const lv_fpoint_t * p3)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    if(lv_array_is_empty(&path->ops)) {
        /*first op must be move_to*/
        return;
    }

    CHECK_AND_RESIZE_PATH_CONTAINER(path, 3);

    uint8_t op = LV_VECTOR_PATH_OP_CUBIC_TO;
    OP_PUSH_BACK(&path->ops, &op);
    POINT3_PUSH_BACK(&path->points, p1, p2, p3);
}

static void lv_sw_path_close_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    if(lv_array_is_empty(&path->ops)) {
        /*first op must be move_to*/
        return;
    }

    CHECK_AND_RESIZE_PATH_CONTAINER(path, 1);

    uint8_t op = LV_VECTOR_PATH_OP_CLOSE;
    OP_PUSH_BACK(&path->ops, &op);
}

static void lv_sw_path_clear_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    lv_array_clear(&path->ops);
    lv_array_clear(&path->points);
}

static void lv_sw_path_get_bounds_cb(struct lv_platform_path_base_t * self, lv_area_t * area)
{
    LV_ASSERT_NULL(self);
    LV_ASSERT_NULL(area);

    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    uint32_t len = lv_array_size(&path->points);
    if(len == 0) {
        lv_memzero(area, sizeof(lv_area_t));
        return;
    }

    lv_fpoint_t * p = lv_array_at(&path->points, 0);
    float x1 = p[0].x;
    float x2 = p[0].x;
    float y1 = p[0].y;
    float y2 = p[0].y;

    for(uint32_t i = 1; i < len; i++) {
        if(p[i].x < x1) x1 = p[i].x;
        if(p[i].y < y1) y1 = p[i].y;
        if(p[i].x > x2) x2 = p[i].x;
        if(p[i].y > y2) y2 = p[i].y;
    }

    area->x1 = lroundf(x1);
    area->y1 = lroundf(y1);
    area->x2 = lroundf(x2);
    area->y2 = lroundf(y2);
}

static lv_vector_path_quality_t lv_sw_path_get_quality_cb(struct lv_platform_path_base_t * self)
{
    LV_UNUSED(self);
    LV_LOG_WARN("not implemented");
    return LV_VECTOR_PATH_QUALITY_MEDIUM;
}

void lv_sw_matrix_transform_point(const lv_matrix_t * matrix, lv_fpoint_t * point)
{
    float x = point->x;
    float y = point->y;

    point->x = x * matrix->m[0][0] + y * matrix->m[0][1] + matrix->m[0][2];
    point->y = x * matrix->m[1][0] + y * matrix->m[1][1] + matrix->m[1][2];
}

static void lv_sw_path_transform_cb(struct lv_platform_path_base_t * self, const lv_matrix_t * matrix)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);

    lv_fpoint_t * pt = lv_array_front(&path->points);
    uint32_t size = lv_array_size(&path->points);
    for(uint32_t i = 0; i < size; i++) {
        lv_sw_matrix_transform_point(matrix, &pt[i]);
    }
}

static void lv_sw_path_get_data_cb(struct lv_platform_path_base_t * self, lv_vector_path_data_t * data)
{
    lv_platform_sw_path_t * src = LV_SW_PATH_CAST(self);

    lv_array_copy(&data->ops, &src->ops);
    lv_array_copy(&data->points, &src->points);
}

static void lv_sw_path_transform_path_cb(struct lv_platform_path_base_t * self,
                                         lv_vector_path_transform_data_t * transform_data)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);

    uint32_t pidx = 0;
    uint32_t len = lv_array_size(&path->ops);
    lv_vector_path_op_t * op = lv_array_front(&path->ops);
    void * user_data = transform_data->user_data;
    for(uint32_t i = 0; i < len; i++) {
        switch(op[i]) {
            case LV_VECTOR_PATH_OP_MOVE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&path->points, pidx);
                    transform_data->cb(LV_VECTOR_PATH_OP_MOVE_TO, pt, user_data);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_LINE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&path->points, pidx);
                    transform_data->cb(LV_VECTOR_PATH_OP_LINE_TO, pt, user_data);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_QUAD_TO: {
                    LV_ASSERT(pidx > 0);
                    lv_fpoint_t * pt1 = lv_array_at(&path->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&path->points, pidx + 1);
                    lv_fpoint_t * last_pt = lv_array_at(&path->points, pidx - 1);

                    lv_flatten_quadratic_curve(last_pt, pt1, pt2, transform_data->cb, user_data);
                    pidx += 2;
                }
                break;
            case LV_VECTOR_PATH_OP_CUBIC_TO: {
                    LV_ASSERT(pidx > 0);
                    lv_fpoint_t * pt1 = lv_array_at(&path->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&path->points, pidx + 1);
                    lv_fpoint_t * pt3 = lv_array_at(&path->points, pidx + 2);
                    lv_fpoint_t * last_pt = lv_array_at(&path->points, pidx - 1);

                    lv_flatten_cubic_curve(last_pt, pt1, pt2, pt3, transform_data->cb, user_data);
                    pidx += 3;
                }
                break;
            case LV_VECTOR_PATH_OP_CLOSE: {
                    transform_data->cb(LV_VECTOR_PATH_OP_CLOSE, NULL, user_data);
                }
                break;
        }
    }

    transform_data->cb(LV_VECTOR_POLYGON_STOP, NULL, user_data); // for polygon stop flag
}

static bool lv_sw_path_is_empty_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);

    if(lv_array_size(&path->points) == 0) {
        return true;
    }
    return false;
}

size_t lv_sw_path_get_mem_size_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);
    size_t size = 0;
    size += sizeof(lv_platform_sw_path_t);
    size += path->ops.capacity * path->ops.element_size;
    size += path->points.capacity * path->points.element_size;
    return size;
}

#if LV_USE_VECTOR_DUMP_INFO
void lv_sw_path_dump_info_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_sw_path_t * path = LV_SW_PATH_CAST(self);

    uint32_t ops_size = lv_array_size(&path->ops);
    uint32_t points_size = lv_array_size(&path->points);
    size_t buf_size = (ops_size * 20) + (points_size * 20) + 1024; // Conservative estimate
    char * logInfo = lv_malloc(buf_size);
    if(!logInfo) {
        LV_LOG_ERROR("Failed to allocate memory for path dump info");
        return;
    }

    lv_snprintf(logInfo, buf_size, "ops|pts: %d|%d|{", (int)ops_size, (int)points_size);

    uint32_t pidx = 0;
    lv_vector_path_op_t * op = lv_array_front(&path->ops);
    for(uint32_t i = 0; i < ops_size; i++) {
        switch(op[i]) {
            case LV_VECTOR_PATH_OP_MOVE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&path->points, pidx);
                    lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo),
                                "M%f,%f ", pt->x, pt->y);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_LINE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&path->points, pidx);
                    lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo),
                                "L%f,%f ", pt->x, pt->y);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_QUAD_TO: {
                    lv_fpoint_t * pt1 = lv_array_at(&path->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&path->points, pidx + 1);
                    lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo),
                                "Q%f,%f %f,%f ", pt1->x, pt1->y, pt2->x, pt2->y);
                    pidx += 2;
                }
                break;
            case LV_VECTOR_PATH_OP_CUBIC_TO: {
                    lv_fpoint_t * pt1 = lv_array_at(&path->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&path->points, pidx + 1);
                    lv_fpoint_t * pt3 = lv_array_at(&path->points, pidx + 2);
                    lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo),
                                "C%f,%f %f,%f %f,%f ", pt1->x, pt1->y, pt2->x, pt2->y, pt3->x, pt3->y);
                    pidx += 3;
                }
                break;
            case LV_VECTOR_PATH_OP_CLOSE: {
                    lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo), "Z ");
                }
                break;
        }
        lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo), "|");
    }

    lv_snprintf(logInfo + lv_strlen(logInfo), buf_size - lv_strlen(logInfo), "}");
    LV_LOG_USER("Path dump: %s", logInfo);
    lv_free(logInfo);
}
#endif

static const lv_platform_path_handlers sw_path_handlers = {
    .create         = lv_sw_path_create_cb,
    .destroy        = lv_sw_path_destroy_cb,
    .clone          = lv_sw_path_clone_cb,
    .concat         = lv_sw_path_concat_cb,
    .move_to        = lv_sw_path_move_to_cb,
    .line_to        = lv_sw_path_line_to_cb,
    .quad_to        = lv_sw_path_quad_to_cb,
    .cubic_to       = lv_sw_path_cubic_to_cb,
    .close_path     = lv_sw_path_close_cb,
    .clear          = lv_sw_path_clear_cb,
    .get_bounding   = lv_sw_path_get_bounds_cb,
    .get_quality    = lv_sw_path_get_quality_cb,
    .transform      = lv_sw_path_transform_cb,
    .get_data       = lv_sw_path_get_data_cb,
    .is_empty       = lv_sw_path_is_empty_cb,
    .transform_path = lv_sw_path_transform_path_cb,
    .get_mem_size   = lv_sw_path_get_mem_size_cb,
#if LV_USE_VECTOR_DUMP_INFO
    .dump_path_info = lv_sw_path_dump_info_cb,
#endif
};

const lv_platform_path_handlers * lv_vector_get_platform_handlers(void)
{
    return &sw_path_handlers;
}

#endif /*LV_USE_DRAW_SW*/
