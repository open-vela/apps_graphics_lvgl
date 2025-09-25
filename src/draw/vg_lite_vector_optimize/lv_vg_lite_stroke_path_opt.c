/**
 * @file lv_vg_lite_stroke_path_opt.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_vg_lite_stroke_path_opt.h"

#if LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE

#include "../vg_lite/lv_draw_vg_lite_type.h"
#include "../vg_lite/lv_vg_lite_path.h"
#include <float.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void vg_path_generate_cb(lv_vector_path_op_t op, const lv_fpoint_t * pt, void * data);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_vg_lite_stroke_path_init(struct _lv_draw_vg_lite_unit_t * unit)
{
    LV_ASSERT_NULL(unit);
    unit->stroke_path = lv_vg_lite_path_create(VG_LITE_FP32);
    unit->stroke_path_in_use = false;
}

void lv_vg_lite_stroke_path_deinit(struct _lv_draw_vg_lite_unit_t * unit)
{
    LV_ASSERT_NULL(unit);
    LV_ASSERT(!unit->stroke_path_in_use);
    lv_vg_lite_path_destroy(unit->stroke_path);
    unit->stroke_path = NULL;
}

struct _lv_vg_lite_path_t * lv_vg_lite_stroke_path_get(lv_platform_vg_lite_path_t * impl,
                                                       const lv_vector_stroke_dsc_t * dsc)
{
    LV_PROFILER_DRAW_BEGIN;
    LV_ASSERT_NULL(impl->stroke_path_cache);
    lv_vg_lite_path_reset(impl->stroke_path_cache, VG_LITE_FP32);
    lv_vg_lite_path_set_bounding_box(impl->stroke_path_cache, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    if(!lv_vector_stroke_generate((lv_platform_path_base_t *)impl, dsc, vg_path_generate_cb, impl->stroke_path_cache)) {
        lv_vg_lite_path_destroy(impl->stroke_path_cache);
        impl->stroke_path_cache = NULL;
        LV_PROFILER_DRAW_END;
        return NULL;
    }

    vg_lite_path_t * vg_stroke_path = lv_vg_lite_path_get_path(impl->stroke_path_cache);
    float expand_bound = dsc->width;
    vg_stroke_path->bounding_box[0] -= expand_bound;
    vg_stroke_path->bounding_box[1] -= expand_bound;
    vg_stroke_path->bounding_box[2] += expand_bound;
    vg_stroke_path->bounding_box[3] += expand_bound;

    LV_PROFILER_DRAW_END;
    return impl->stroke_path_cache;

}


void lv_vg_lite_stroke_path_drop(struct _lv_draw_vg_lite_unit_t * unit, struct _lv_vg_lite_path_t * path)
{
    LV_ASSERT_NULL(unit);
    LV_ASSERT_NULL(path);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void vg_path_generate_cb(lv_vector_path_op_t op, const lv_fpoint_t * pt, void * data)
{
    lv_vg_lite_path_t * path = data;

    float min_x, min_y, max_x, max_y;
    lv_vg_lite_path_get_bounding_box(path, &min_x, &min_y, &max_x, &max_y);

#define CMP_BOUNDS(point)                           \
    do {                                            \
        if((point)->x < min_x) min_x = (point)->x;  \
        if((point)->y < min_y) min_y = (point)->y;  \
        if((point)->x > max_x) max_x = (point)->x;  \
        if((point)->y > max_y) max_y = (point)->y;  \
    } while(0)

    switch(op) {
        case LV_VECTOR_PATH_OP_MOVE_TO:
            CMP_BOUNDS(pt);
            lv_vg_lite_path_move_to(path, pt->x, pt->y);
            break;
        case LV_VECTOR_PATH_OP_LINE_TO:
            CMP_BOUNDS(pt);
            lv_vg_lite_path_line_to(path, pt->x, pt->y);
            break;
        case LV_VECTOR_PATH_OP_CLOSE:
            lv_vg_lite_path_close(path);
            break;
        default:
            break;
    }

    lv_vg_lite_path_set_bounding_box(path, min_x, min_y, max_x, max_y);
}

#endif /*LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE*/
