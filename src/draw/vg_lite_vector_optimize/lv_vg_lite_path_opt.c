/**
 * @file lv_vg_lite_path.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_vg_lite_path_opt.h"

#if LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE
#include "../vg_lite/lv_draw_vg_lite_type.h"
#include "../vg_lite/lv_vg_lite_math.h"
#include <float.h>

/*********************
 *      DEFINES
 *********************/

#define LV_VG_LITE_PATH_CAST(ptr) \
    (assert(ptr != NULL), \
     (lv_platform_vg_lite_path_t *)(ptr))

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
static void lv_vg_lite_path_update_bounding_box_by_point(lv_vg_lite_path_t * path, const lv_fpoint_t * point)
{
    LV_ASSERT_NULL(point);
    LV_ASSERT_NULL(path);

    /* update bounds */
    float min_x, min_y, max_x, max_y;
    lv_vg_lite_path_get_bounding_box(path, &min_x, &min_y, &max_x, &max_y);

    if(point->x < min_x) min_x = point->x;
    if(point->y < min_y) min_y = point->y;
    if(point->x > max_x) max_x = point->x;
    if(point->y > max_y) max_y = point->y;

    /* set bounds */
    lv_vg_lite_path_set_bounding_box(path, min_x, min_y, max_x, max_y);
}

static struct lv_platform_path_base_t * lv_vg_lite_path_create_cb(lv_vector_path_quality_t quality)
{
    lv_platform_vg_lite_path_t * path = lv_malloc_zeroed(sizeof(lv_platform_vg_lite_path_t));
    vg_lite_quality_t vg_quality = VG_LITE_MEDIUM;
    switch(quality) {
        case LV_VECTOR_PATH_QUALITY_LOW:
            vg_quality = VG_LITE_LOW;
            break;
        case LV_VECTOR_PATH_QUALITY_MEDIUM:
            vg_quality = VG_LITE_MEDIUM;
            break;
        case LV_VECTOR_PATH_QUALITY_HIGH:
            vg_quality = VG_LITE_HIGH;
            break;
        default:
            break;
    }
    path->vg_path = lv_vg_lite_path_create(VG_LITE_FP32);
    lv_vg_lite_path_set_quality(path->vg_path, vg_quality);
    lv_vg_lite_path_set_bounding_box(path->vg_path, FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN);
    return (lv_platform_path_base_t *)path;
}

static void lv_vg_lite_path_destroy_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);

    if(path->vg_path) {
        lv_vg_lite_path_destroy(path->vg_path);
    }
}

static struct lv_platform_path_base_t * lv_vg_lite_path_clone_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * src = LV_VG_LITE_PATH_CAST(self);
    lv_platform_vg_lite_path_t * dst = lv_malloc_zeroed(sizeof(lv_platform_vg_lite_path_t));
    dst->vg_path = lv_vg_lite_path_create(src->vg_path->base.format);
    lv_vg_lite_path_append_path(dst->vg_path, src->vg_path);
    return (lv_platform_path_base_t *)dst;
}

static void lv_vg_lite_path_concat_cb(struct lv_platform_path_base_t * self, struct lv_platform_path_base_t * other)
{
    lv_platform_vg_lite_path_t * dst = LV_VG_LITE_PATH_CAST(self);
    lv_platform_vg_lite_path_t * src = LV_VG_LITE_PATH_CAST(other);
    lv_vg_lite_path_append_path(dst->vg_path, src->vg_path);
}

static void lv_vg_lite_path_move_to_cb(struct lv_platform_path_base_t * self, const lv_fpoint_t * p)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_move_to(path->vg_path, p->x, p->y);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p);
}

static void lv_vg_lite_path_line_to_cb(struct lv_platform_path_base_t * self, const lv_fpoint_t * p)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_line_to(path->vg_path, p->x, p->y);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p);
}

static void lv_vg_lite_path_quad_to_cb(struct lv_platform_path_base_t * self, const lv_fpoint_t * p1,
                                       const lv_fpoint_t * p2)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_quad_to(path->vg_path, p1->x, p1->y, p2->x, p2->y);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p1);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p2);
}

static void lv_vg_lite_path_cubic_to_cb(struct lv_platform_path_base_t * self,
                                        const lv_fpoint_t * p1, const lv_fpoint_t * p2,
                                        const lv_fpoint_t * p3)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_cubic_to(path->vg_path, p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p1);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p2);
    lv_vg_lite_path_update_bounding_box_by_point(path->vg_path, p3);
}

static void lv_vg_lite_path_close_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_close(path->vg_path);
}

static void lv_vg_lite_path_clear_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_reset(path->vg_path, path->vg_path->base.format);
    lv_vg_lite_path_set_bounding_box(path->vg_path, FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN);
}

static void lv_vg_lite_path_get_bounds_cb(struct lv_platform_path_base_t * self, lv_area_t * area)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    float min_x, min_y, max_x, max_y;
    lv_vg_lite_path_get_bounding_box(path->vg_path, &min_x, &min_y, &max_x, &max_y);
    area->x1 = (int32_t)min_x;
    area->y1 = (int32_t)min_y;
    area->x2 = (int32_t)max_x;
    area->y2 = (int32_t)max_y;
}

static lv_vector_path_quality_t lv_vg_lite_path_get_quality_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    vg_lite_quality_t vg_quality = path->vg_path->base.quality;
    lv_vector_path_quality_t lv_quality = LV_VECTOR_PATH_QUALITY_MEDIUM;
    switch(vg_quality) {
        case VG_LITE_LOW:
            lv_quality = LV_VECTOR_PATH_QUALITY_LOW;
            break;
        case VG_LITE_MEDIUM:
            lv_quality = LV_VECTOR_PATH_QUALITY_MEDIUM;
            break;
        case VG_LITE_HIGH:
            lv_quality = LV_VECTOR_PATH_QUALITY_HIGH;
            break;
        default:
            break;
    }
    return lv_quality;
}

static void lv_vg_lite_path_transform_cb(struct lv_platform_path_base_t * self, const lv_matrix_t * matrix)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    lv_vg_lite_path_set_transform(path->vg_path, (const vg_lite_matrix_t *)matrix);
}

static void get_path_data_cb(void * user_data, uint8_t op_code, const float * data, uint32_t len)
{
    lv_vector_path_data_t * parser = (lv_vector_path_data_t *)user_data;

    uint8_t lv_op = LV_VECTOR_PATH_OP_MOVE_TO;
    switch(op_code) {
        case VLC_OP_CLOSE:
            lv_op = LV_VECTOR_PATH_OP_CLOSE;
            OP_PUSH_BACK(&parser->ops, &lv_op);
            break;
        case VLC_OP_MOVE:
            lv_op = LV_VECTOR_PATH_OP_MOVE_TO;
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT_PUSH_BACK(&parser->points, (lv_fpoint_t *)data);
            break;
        case VLC_OP_LINE:
            lv_op = LV_VECTOR_PATH_OP_LINE_TO;
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT_PUSH_BACK(&parser->points, (lv_fpoint_t *)data);
            break;
        case VLC_OP_QUAD:
            lv_op = LV_VECTOR_PATH_OP_QUAD_TO;
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT2_PUSH_BACK(&parser->points, (lv_fpoint_t *)data, (lv_fpoint_t *)(data + 2));
            break;
        case VLC_OP_CUBIC:
            lv_op = LV_VECTOR_PATH_OP_CUBIC_TO;
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT3_PUSH_BACK(&parser->points, (lv_fpoint_t *)data, (lv_fpoint_t *)(data + 2), (lv_fpoint_t *)(data + 4));
            break;
        default:
            LV_LOG_ERROR("Invalid opcode: %d", op_code);
            break;
    }
}

static void lv_vg_lite_path_get_data_cb(struct lv_platform_path_base_t * self, lv_vector_path_data_t * data)
{
    LV_ASSERT_NULL(self);
    LV_ASSERT_NULL(data);

    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    vg_lite_path_t * vg_path = lv_vg_lite_path_get_path(path->vg_path);
    if(vg_path) {
        lv_vg_lite_path_for_each_data(vg_path, get_path_data_cb, data);
    }
}

static void transform_path_cb(void * user_data, uint8_t op_code, const float * data, uint32_t len)
{
    lv_vector_path_transform_data_t * transform_data = (lv_vector_path_transform_data_t *)user_data;
    switch(op_code) {
        case VLC_OP_CLOSE:
            transform_data->cb(LV_VECTOR_PATH_OP_CLOSE, NULL, transform_data->user_data);
            break;
        case VLC_OP_MOVE:
            transform_data->cb(LV_VECTOR_PATH_OP_MOVE_TO, (lv_fpoint_t *)data, transform_data->user_data);
            transform_data->last_point.x = data[0];
            transform_data->last_point.y = data[1];
            break;
        case VLC_OP_LINE:
            transform_data->cb(LV_VECTOR_PATH_OP_LINE_TO, (lv_fpoint_t *)data, transform_data->user_data);
            transform_data->last_point.x = data[0];
            transform_data->last_point.y = data[1];
            break;
        case VLC_OP_QUAD:
            lv_flatten_quadratic_curve(&(transform_data->last_point), (lv_fpoint_t *)data, (lv_fpoint_t *)(data + 2),
                                       transform_data->cb, transform_data->user_data);
            transform_data->last_point.x = data[2];
            transform_data->last_point.y = data[3];
            break;
        case VLC_OP_CUBIC:
            lv_flatten_cubic_curve(&(transform_data->last_point), (lv_fpoint_t *)data, (lv_fpoint_t *)(data + 2),
                                   (lv_fpoint_t *)(data + 4), transform_data->cb, transform_data->user_data);
            transform_data->last_point.x = data[4];
            transform_data->last_point.y = data[5];
            break;
        default:
            LV_LOG_ERROR("Invalid opcode: %d", op_code);
            break;
    }
}

static void lv_vg_lite_path_transform_path_cb(struct lv_platform_path_base_t * self,
                                              lv_vector_path_transform_data_t * transform_data)
{
    LV_ASSERT_NULL(self);
    LV_ASSERT_NULL(transform_data);

    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    vg_lite_path_t * vg_path = lv_vg_lite_path_get_path(path->vg_path);

    if(vg_path) {
        lv_vg_lite_path_for_each_data(vg_path, transform_path_cb, transform_data);
    }
    transform_data->cb(LV_VECTOR_POLYGON_STOP, NULL, transform_data->user_data); // for polygon stop flag
}

static bool lv_vg_lite_path_is_empty_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    if(path->vg_path->base.path_length == 0) {
        return true;
    }
    return false;
}

static const lv_platform_path_handlers vg_lite_path_handlers = {
    .create         = lv_vg_lite_path_create_cb,
    .destroy        = lv_vg_lite_path_destroy_cb,
    .clone          = lv_vg_lite_path_clone_cb,
    .concat         = lv_vg_lite_path_concat_cb,
    .move_to        = lv_vg_lite_path_move_to_cb,
    .line_to        = lv_vg_lite_path_line_to_cb,
    .quad_to        = lv_vg_lite_path_quad_to_cb,
    .cubic_to       = lv_vg_lite_path_cubic_to_cb,
    .close_path     = lv_vg_lite_path_close_cb,
    .clear          = lv_vg_lite_path_clear_cb,
    .get_bounding   = lv_vg_lite_path_get_bounds_cb,
    .get_quality    = lv_vg_lite_path_get_quality_cb,
    .transform      = lv_vg_lite_path_transform_cb,
    .get_data       = lv_vg_lite_path_get_data_cb,
    .is_empty       = lv_vg_lite_path_is_empty_cb,
    .transform_path = lv_vg_lite_path_transform_path_cb,
};

const lv_platform_path_handlers * lv_vector_get_platform_handlers(void)
{
    return &vg_lite_path_handlers;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_USE_DRAW_VG_LITE*/
