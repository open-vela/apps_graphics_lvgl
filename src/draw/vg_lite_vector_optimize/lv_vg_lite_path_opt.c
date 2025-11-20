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
#include "../vg_lite/lv_vg_lite_pending.h"
#include <float.h>

/*********************
 *      DEFINES
 *********************/

#define LV_VG_LITE_PATH_CAST(ptr) (lv_platform_vg_lite_path_t *)(ptr)

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

void lv_vg_lite_path_add_end(lv_vg_lite_path_t * vg_path)
{
    if(vg_path->base.add_end == 0) {
        lv_vg_lite_path_end(vg_path);
    }
}

void lv_vg_lite_path_clear_end(lv_vg_lite_path_t * vg_path)
{
    if(vg_path->base.add_end) {
        vg_path->base.path_length -= vg_path->format_len;
        vg_path->base.add_end = 0;
    }
}

void lv_vg_lite_path_expand_bounding_box(lv_vg_lite_path_t * path)
{
    float min_x, min_y, max_x, max_y;
    lv_vg_lite_path_get_bounding_box(path, &min_x, &min_y, &max_x, &max_y);

    float width = max_x - min_x;
    float height = max_y - min_y;

    /* Expand bounding box by 1/4 of its size in each direction */
    float new_min_x = min_x - width / 4;
    float new_min_y = min_y - height / 4;
    float new_max_x = max_x + width / 4;
    float new_max_y = max_y + height / 4;

    lv_vg_lite_path_set_bounding_box(path, new_min_x, new_min_y, new_max_x, new_max_y);
}

/**********************
 *   STATIC FUNCTIONS
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

static void lv_vg_lite_path_update_bounding_box_after_append(lv_vg_lite_path_t * dst, lv_vg_lite_path_t * src)
{
    LV_ASSERT_NULL(dst);
    LV_ASSERT_NULL(src);

    float dst_min_x, dst_min_y, dst_max_x, dst_max_y;
    float src_min_x, src_min_y, src_max_x, src_max_y;

    lv_vg_lite_path_get_bounding_box(dst, &dst_min_x, &dst_min_y, &dst_max_x, &dst_max_y);
    lv_vg_lite_path_get_bounding_box(src, &src_min_x, &src_min_y, &src_max_x, &src_max_y);

    float min_x = (dst_min_x < src_min_x) ? dst_min_x : src_min_x;
    float min_y = (dst_min_y < src_min_y) ? dst_min_y : src_min_y;
    float max_x = (dst_max_x > src_max_x) ? dst_max_x : src_max_x;
    float max_y = (dst_max_y > src_max_y) ? dst_max_y : src_max_y;

    lv_vg_lite_path_set_bounding_box(dst, min_x, min_y, max_x, max_y);
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
    lv_vg_lite_path_set_bounding_box(path->vg_path, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    path->stroke_path_cache = lv_vg_lite_path_create(VG_LITE_FP32);
    lv_vg_lite_path_set_quality(path->stroke_path_cache, vg_quality);
    lv_vg_lite_path_set_bounding_box(path->stroke_path_cache, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    return (lv_platform_path_base_t *)path;
}

static void lv_vg_lite_path_destroy_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);

    if(path->vg_path) {
        lv_vg_lite_path_destroy(path->vg_path);
    }

    if(path->stroke_path_cache) {
        lv_vg_lite_path_destroy(path->stroke_path_cache);
    }

    lv_free(path);
}

static struct lv_platform_path_base_t * lv_vg_lite_path_clone_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * src = LV_VG_LITE_PATH_CAST(self);
    lv_platform_vg_lite_path_t * dst = lv_malloc_zeroed(sizeof(lv_platform_vg_lite_path_t));
    dst->vg_path = lv_vg_lite_path_create(src->vg_path->base.format);
    lv_vg_lite_path_append_path(dst->vg_path, src->vg_path);
    lv_vg_lite_path_clear_end(dst->vg_path);

    dst->stroke_path_cache = lv_vg_lite_path_create(src->stroke_path_cache->base.format);

    lv_vg_lite_path_update_bounding_box_after_append(dst->vg_path, src->vg_path);
    return (lv_platform_path_base_t *)dst;
}

static void lv_vg_lite_path_concat_cb(struct lv_platform_path_base_t * self, struct lv_platform_path_base_t * other)
{
    lv_platform_vg_lite_path_t * dst = LV_VG_LITE_PATH_CAST(self);
    lv_platform_vg_lite_path_t * src = LV_VG_LITE_PATH_CAST(other);

    lv_vg_lite_path_append_path(dst->vg_path, src->vg_path);
    lv_vg_lite_path_update_bounding_box_after_append(dst->vg_path, src->vg_path);
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
    lv_vg_lite_path_set_bounding_box(path->vg_path, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
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
typedef struct {
    const vg_lite_matrix_t * matrix;
    lv_vg_lite_path_t * new_path;
} lv_vg_lite_path_transform_ctx_t;

static void lv_vg_lite_path_transform_and_build_cb(void * user_data, uint8_t op_code, const float * data, uint32_t len)
{
    LV_ASSERT_NULL(user_data);
    LV_ASSERT_NULL(data);

    lv_vg_lite_path_transform_ctx_t * ctx = (lv_vg_lite_path_transform_ctx_t *)user_data;
    const vg_lite_matrix_t * matrix = ctx->matrix;
    lv_vg_lite_path_t * new_path = ctx->new_path;

    uint32_t point_count = 0;

    /* Determine number of points based on op_code */
    switch(op_code) {
        case VLC_OP_MOVE:
        case VLC_OP_LINE:
            point_count = 1;
            break;
        case VLC_OP_QUAD:
            point_count = 2;
            break;
        case VLC_OP_CUBIC:
            point_count = 3;
            break;
        case VLC_OP_CLOSE:
            lv_vg_lite_path_close(new_path);
            return;
        default:
            return; /* Skip unsupported op codes */
    }

    /* Ensure we have enough data */
    if(len < point_count * 2) {
        return;
    }

    /* Transform points and build new path */
    lv_point_precise_t points[3];
    for(uint32_t i = 0; i < point_count; i++) {
        const float * point_data = &data[i * 2];
        lv_point_precise_t point = {
            .x = point_data[0],
            .y = point_data[1]
        };
        points[i] = lv_vg_lite_matrix_transform_point(matrix, &point);
    }

    /* Build new path */
    switch(op_code) {
        case VLC_OP_MOVE:
            lv_vg_lite_path_move_to(new_path, points[0].x, points[0].y);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[0]);
            break;
        case VLC_OP_LINE:
            lv_vg_lite_path_line_to(new_path, points[0].x, points[0].y);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[0]);
            break;
        case VLC_OP_QUAD:
            lv_vg_lite_path_quad_to(new_path, points[0].x, points[0].y, points[1].x, points[1].y);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[0]);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[1]);
            break;
        case VLC_OP_CUBIC:
            lv_vg_lite_path_cubic_to(new_path,
                                     points[0].x, points[0].y,
                                     points[1].x, points[1].y,
                                     points[2].x, points[2].y);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[0]);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[1]);
            lv_vg_lite_path_update_bounding_box_by_point(new_path, (lv_fpoint_t *)&points[2]);
            break;
    }
}

static void lv_vg_lite_path_transform_cb(struct lv_platform_path_base_t * self, const lv_matrix_t * matrix)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    vg_lite_matrix_t vg_matrix = {0};
    lv_vg_lite_matrix(&vg_matrix, matrix);
    lv_vg_lite_path_set_transform(path->vg_path, &vg_matrix);

    vg_lite_path_t * vg_path = lv_vg_lite_path_get_path(path->vg_path);
    if(vg_path) {
        /* Create a temporary path for transformed data */
        lv_vg_lite_path_t * temp_path = lv_vg_lite_path_create(vg_path->format);
        lv_vg_lite_path_set_bounding_box(temp_path, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

        lv_vg_lite_path_transform_ctx_t ctx = {
            .matrix = &vg_matrix,
            .new_path = temp_path
        };

        /* Single pass: transform points and build new path */
        lv_vg_lite_path_for_each_data(vg_path, lv_vg_lite_path_transform_and_build_cb, &ctx);

        /* Replace original path with transformed one */
        lv_vg_lite_path_reset(path->vg_path, path->vg_path->base.format);
        lv_vg_lite_path_set_bounding_box(path->vg_path, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
        lv_vg_lite_path_append_path(path->vg_path, temp_path);
        lv_vg_lite_path_update_bounding_box_after_append(path->vg_path, temp_path);
        lv_vg_lite_path_destroy(temp_path);
    }
}

static void get_path_data_cb(void * user_data, uint8_t op_code, const float * data, uint32_t len)
{
    LV_UNUSED(len);
    lv_vector_path_data_t * parser = (lv_vector_path_data_t *)user_data;

    uint8_t lv_op = LV_VECTOR_PATH_OP_MOVE_TO;
    switch(op_code) {
        case VLC_OP_CLOSE:
            lv_op = LV_VECTOR_PATH_OP_CLOSE;
            CHECK_AND_RESIZE_PATH_CONTAINER(parser, 1);
            OP_PUSH_BACK(&parser->ops, &lv_op);
            break;
        case VLC_OP_MOVE:
            lv_op = LV_VECTOR_PATH_OP_MOVE_TO;
            CHECK_AND_RESIZE_PATH_CONTAINER(parser, 1);
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT_PUSH_BACK(&parser->points, (lv_fpoint_t *)data);
            break;
        case VLC_OP_LINE:
            lv_op = LV_VECTOR_PATH_OP_LINE_TO;
            CHECK_AND_RESIZE_PATH_CONTAINER(parser, 1);
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT_PUSH_BACK(&parser->points, (lv_fpoint_t *)data);
            break;
        case VLC_OP_QUAD:
            lv_op = LV_VECTOR_PATH_OP_QUAD_TO;
            CHECK_AND_RESIZE_PATH_CONTAINER(parser, 2);
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT2_PUSH_BACK(&parser->points, (lv_fpoint_t *)data, (lv_fpoint_t *)(data + 2));
            break;
        case VLC_OP_CUBIC:
            lv_op = LV_VECTOR_PATH_OP_CUBIC_TO;
            CHECK_AND_RESIZE_PATH_CONTAINER(parser, 3);
            OP_PUSH_BACK(&parser->ops, &lv_op);
            POINT3_PUSH_BACK(&parser->points, (lv_fpoint_t *)data, (lv_fpoint_t *)(data + 2), (lv_fpoint_t *)(data + 4));
            break;
        default:
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
    LV_UNUSED(len);
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

static size_t lv_vg_lite_path_get_mem_size(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    return path->vg_path->mem_size;
}

#if LV_USE_VECTOR_DUMP_INFO
static void lv_vg_lite_path_dump_info_cb(struct lv_platform_path_base_t * self)
{
    lv_platform_vg_lite_path_t * path = LV_VG_LITE_PATH_CAST(self);
    vg_lite_path_t * vg_path = lv_vg_lite_path_get_path(path->vg_path);
    lv_vg_lite_path_dump_info(vg_path);
}
#endif

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
    .get_mem_size   = lv_vg_lite_path_get_mem_size,
#if LV_USE_VECTOR_DUMP_INFO
    .dump_path_info = lv_vg_lite_path_dump_info_cb,
#endif
};

const lv_platform_path_handlers * lv_vector_get_platform_handlers(void)
{
    return &vg_lite_path_handlers;
}

#endif /*LV_USE_DRAW_VG_LITE*/
