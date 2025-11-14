/**
 * @file lv_vg_lite_stroke_path.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_vg_lite_stroke_path.h"

#if LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC

#include "lv_draw_vg_lite_type.h"
#include "lv_vg_lite_path.h"
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

struct _lv_vg_lite_path_t * lv_vg_lite_stroke_path_get(struct _lv_draw_vg_lite_unit_t * unit,
                                                       const lv_vector_path_t * path,
                                                       const lv_vector_stroke_dsc_t * dsc)
{
    LV_PROFILER_DRAW_BEGIN;
    LV_ASSERT_NULL(unit);
    LV_ASSERT_NULL(unit->stroke_path);
    LV_ASSERT(!unit->stroke_path_in_use);
    lv_vg_lite_path_reset(unit->stroke_path, VG_LITE_FP32);
    lv_vg_lite_path_set_bounding_box(unit->stroke_path, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

    if(!lv_vector_stroke_generate(path, dsc, vg_path_generate_cb, unit->stroke_path)) {
        LV_PROFILER_DRAW_END;
        return NULL;
    }

    lv_vg_lite_path_end(unit->stroke_path);

    /* Expand the bounding box by the stroke width to ensure the stroke is fully visible */
    float expand_bound = dsc->width;
    float min_x, min_y, max_x, max_y;
    lv_vg_lite_path_get_bounding_box(unit->stroke_path, &min_x, &min_y, &max_x, &max_y);
    lv_vg_lite_path_set_bounding_box(
        unit->stroke_path,
        min_x - expand_bound,
        min_y - expand_bound,
        max_x + expand_bound,
        max_y + expand_bound);

    unit->stroke_path_in_use = true;

    LV_PROFILER_DRAW_END;
    return unit->stroke_path;
}

void lv_vg_lite_stroke_path_drop(struct _lv_draw_vg_lite_unit_t * unit, struct _lv_vg_lite_path_t * path)
{
    LV_ASSERT_NULL(unit);
    LV_ASSERT_NULL(path);
    LV_ASSERT(unit->stroke_path == path);
    unit->stroke_path_in_use = false;
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

#endif /*LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC*/
