/**
 * @file lv_vector_polygon.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <math.h>

#include "lv_vector_polygon.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _flatten_quadratic_curve(const lv_fpoint_t * sp, const lv_fpoint_t * cp,
                                     const lv_fpoint_t * ep, path_generate_cb cb, void * user_data)
{
    float dx1 = cp->x - sp->x;
    float dy1 = cp->y - sp->y;
    float dx2 = ep->x - cp->x;
    float dy2 = ep->y - cp->y;

    float length = sqrtf(dx1 * dx1 + dy1 * dy1) + sqrtf(dx2 * dx2 + dy2 * dy2);
    int nstep = (int)roundf(length * 0.25f);

    if(nstep < 4) nstep = 4;

    float sub_step = 1.0f / nstep;
    float sub_step2 = sub_step * sub_step;

    float tx = (sp->x - cp->x * 2.0f + ep->x) * sub_step2;
    float ty = (sp->y - cp->y * 2.0f + ep->y) * sub_step2;

    float fx = sp->x;
    float fy = sp->y;

    float dfx = tx + (cp->x - sp->x) * (2.0f * sub_step);
    float dfy = ty + (cp->y - sp->y) * (2.0f * sub_step);

    float ddfx = tx * 2.0f;
    float ddfy = ty * 2.0f;

    float step = nstep;

    while(step >= 0) {
        if(step == nstep) {
            lv_fpoint_t p = {.x = sp->x, .y = sp->y};
            cb(LV_VECTOR_PATH_OP_LINE_TO, &p, user_data);
            --step;
            continue;
        }

        if(step == 0) {
            lv_fpoint_t p = {.x = ep->x, .y = ep->y};
            cb(LV_VECTOR_PATH_OP_LINE_TO, &p, user_data);
            --step;
            continue;
        }

        fx += dfx;
        fy += dfy;
        dfx += ddfx;
        dfy += ddfy;
        lv_fpoint_t p = {.x = fx, .y = fy};
        cb(LV_VECTOR_PATH_OP_LINE_TO, &p, user_data);
        --step;
    }
}

static void _flatten_cubic_curve(const lv_fpoint_t * sp, const lv_fpoint_t * cp1,
                                 const lv_fpoint_t * cp2, const lv_fpoint_t * ep, path_generate_cb cb, void * user_data)
{
    float dx1 = cp1->x - sp->x;
    float dy1 = cp1->y - sp->y;
    float dx2 = cp2->x - cp1->x;
    float dy2 = cp2->y - cp1->y;
    float dx3 = ep->x - cp2->x;
    float dy3 = ep->y - cp2->y;

    float length = sqrtf(dx1 * dx1 + dy1 * dy1) + sqrtf(dx2 * dx2 + dy2 * dy2) + sqrtf(dx3 * dx3 + dy3 * dy3);
    int nstep = (int)roundf(length * 0.25f);

    if(nstep < 4) nstep = 4;

    float sub_step = 1.0f / nstep;
    float sub_step2 = sub_step * sub_step;
    float sub_step3 = sub_step * sub_step * sub_step;

    float pe1 = 3.0f * sub_step;
    float pe2 = 3.0f * sub_step2;
    float pe4 = 6.0f * sub_step2;
    float pe5 = 6.0f * sub_step3;

    float tx1 = sp->x - cp1->x * 2.0f + cp2->x;
    float ty1 = sp->y - cp1->y * 2.0f + cp2->y;

    float tx2 = (cp1->x - cp2->x) * 3.0f - sp->x + ep->x;
    float ty2 = (cp1->y - cp2->y) * 3.0f - sp->y + ep->y;

    float fx = sp->x;
    float fy = sp->y;

    float dfx = (cp1->x - sp->x) * pe1 + tx1 * pe2 + tx2 * sub_step3;
    float dfy = (cp1->y - sp->y) * pe1 + ty1 * pe2 + ty2 * sub_step3;

    float ddfx = tx1 * pe4 + tx2 * pe5;
    float ddfy = ty1 * pe4 + ty2 * pe5;

    float dddfx = tx2 * pe5;
    float dddfy = ty2 * pe5;

    float step = nstep;

    while(step >= 0) {
        if(step == nstep) {
            lv_fpoint_t p = {.x = sp->x, .y = sp->y};
            cb(LV_VECTOR_PATH_OP_LINE_TO, &p, user_data);
            --step;
            continue;
        }

        if(step == 0) {
            lv_fpoint_t p = {.x = ep->x, .y = ep->y};
            cb(LV_VECTOR_PATH_OP_LINE_TO, &p, user_data);
            --step;
            continue;
        }

        fx += dfx;
        fy += dfy;
        dfx += ddfx;
        dfy += ddfy;
        ddfx += dddfx;
        ddfy += dddfy;

        lv_fpoint_t p = {.x = fx, .y = fy};
        cb(LV_VECTOR_PATH_OP_LINE_TO, &p, user_data);
        --step;
    }
}

void lv_flatten_quadratic_curve(const lv_fpoint_t * sp, const lv_fpoint_t * cp,
                                const lv_fpoint_t * ep, path_generate_cb cb, void * user_data)
{
    _flatten_quadratic_curve(sp, cp, ep, cb, user_data);
}

void lv_flatten_cubic_curve(const lv_fpoint_t * sp, const lv_fpoint_t * cp1,
                            const lv_fpoint_t * cp2, const lv_fpoint_t * ep, path_generate_cb cb, void * user_data)
{
    _flatten_cubic_curve(sp, cp1, cp2, ep, cb, user_data);
}

static void _flatten_path(const lv_platform_path_base_t * impl, path_generate_cb cb, void * user_data)
{
    lv_vector_path_transform_data_t path_data = {
        .cb = cb,
        .user_data = user_data,
        .last_point = (lv_fpoint_t)
        {
            0, 0
        }
    };
    lv_vector_path_transform_path(impl, &path_data);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
bool lv_vector_path_flatten(const lv_platform_path_base_t * impl, path_generate_cb cb, void * user_data)
{
    LV_ASSERT_NULL(impl);
    LV_ASSERT_NULL(cb);

    if(lv_vector_path_impl_is_empty(impl)) {
        LV_LOG_ERROR("path is empty!");
        return false;
    }

    _flatten_path(impl, cb, user_data);
    return true;
}

static void path_to_polygon_cb(lv_vector_path_op_t op, const lv_fpoint_t * pt, void * data)
{
    lv_vector_path_t * fpath = (lv_vector_path_t *)data;

    switch(op) {
        case LV_VECTOR_PATH_OP_MOVE_TO: {
                lv_vector_path_move_to(fpath, pt);
            }
            break;
        case LV_VECTOR_PATH_OP_LINE_TO: {
                lv_vector_path_line_to(fpath, pt);
            }
            break;
        case LV_VECTOR_PATH_OP_CLOSE: {
                lv_vector_path_close(fpath);
            }
            break;
    }
}

bool lv_vector_path_to_polygon(lv_vector_path_t * result, const lv_vector_path_t * path)
{
    LV_ASSERT_NULL(result);
    LV_ASSERT_NULL(path);

    if(lv_vector_path_is_empty(path)) {
        LV_LOG_ERROR("path is empty!");
        return false;
    }

    if(path->impl->flags & PATH_FLAG_POLYGON) {
        lv_vector_path_copy(result, path);
        return true;
    }
    else {
        lv_vector_path_clear(result);
        bool ret = lv_vector_path_flatten(path->impl, path_to_polygon_cb, result);
        if(ret) {
            result->impl->flags |= PATH_FLAG_POLYGON; // polygon flag
        }
        return ret;
    }
}

#endif
