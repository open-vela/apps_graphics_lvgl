
/**
 * @file lv_vector_clipper.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <math.h>

#include "lv_vector_clipper.h"

#if LV_USE_VECTOR_GRAPHIC
#include "../libs/gpc/gpc.h"

#define PATH_FLAG_POLYGON  1

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
static void _flatten_quadratic_curve(lv_vector_path_t * path, const lv_fpoint_t * sp, const lv_fpoint_t * cp,
                                     const lv_fpoint_t * ep)
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
            lv_vector_path_line_to(path, &p);
            --step;
            continue;
        }

        if(step == 0) {
            lv_fpoint_t p = {.x = ep->x, .y = ep->y};
            lv_vector_path_line_to(path, &p);
            --step;
            continue;
        }

        fx += dfx;
        fy += dfy;
        dfx += ddfx;
        dfy += ddfy;
        lv_fpoint_t p = {.x = fx, .y = fy};
        lv_vector_path_line_to(path, &p);
        --step;
    }
}

static void _flatten_cubic_curve(lv_vector_path_t * path, const lv_fpoint_t * sp, const lv_fpoint_t * cp1,
                                 const lv_fpoint_t * cp2, const lv_fpoint_t * ep)
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
            lv_vector_path_line_to(path, &p);
            --step;
            continue;
        }

        if(step == 0) {
            lv_fpoint_t p = {.x = ep->x, .y = ep->y};
            lv_vector_path_line_to(path, &p);
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
        lv_vector_path_line_to(path, &p);
        --step;
    }
}

static void _flatten_path(lv_vector_path_t * fpath, const lv_vector_path_t * path)
{
    lv_vector_path_clear(fpath);

    uint32_t pidx = 0;
    uint32_t len = lv_array_size(&path->ops);
    lv_vector_path_op_t * op = lv_array_front(&path->ops);
    lv_fpoint_t * start_point = lv_array_front(&path->points);
    for(uint32_t i = 0; i < len; i++) {
        switch(op[i]) {
            case LV_VECTOR_PATH_OP_MOVE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&path->points, pidx);
                    lv_vector_path_move_to(fpath, pt);
                    start_point = pt;
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_LINE_TO: {
                    lv_fpoint_t * pt = lv_array_at(&path->points, pidx);
                    lv_vector_path_line_to(fpath, pt);
                    pidx += 1;
                }
                break;
            case LV_VECTOR_PATH_OP_QUAD_TO: {
                    LV_ASSERT(pidx > 0);
                    lv_fpoint_t * pt1 = lv_array_at(&path->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&path->points, pidx + 1);
                    lv_fpoint_t * last_pt = lv_array_at(&path->points, pidx - 1);

                    _flatten_quadratic_curve(fpath, last_pt, pt1, pt2);
                    pidx += 2;
                }
                break;
            case LV_VECTOR_PATH_OP_CUBIC_TO: {
                    LV_ASSERT(pidx > 0);
                    lv_fpoint_t * pt1 = lv_array_at(&path->points, pidx);
                    lv_fpoint_t * pt2 = lv_array_at(&path->points, pidx + 1);
                    lv_fpoint_t * pt3 = lv_array_at(&path->points, pidx + 2);
                    lv_fpoint_t * last_pt = lv_array_at(&path->points, pidx - 1);

                    _flatten_cubic_curve(fpath, last_pt, pt1, pt2, pt3);
                    pidx += 3;
                }
                break;
            case LV_VECTOR_PATH_OP_CLOSE: {
                    lv_vector_path_line_to(fpath, start_point); // close polygon path
                    lv_vector_path_close(fpath);
                }
                break;
        }
    }

    fpath->flags |= PATH_FLAG_POLYGON; // polygon flag
}

static void _make_polygon(gpc_polygon * polygon, const lv_vector_path_t * polygon_path)
{
    size_t init_caps = 4;
    polygon->contour = (gpc_vertex_list *)lv_malloc(init_caps * sizeof(gpc_vertex_list));
    polygon->num_contours = 1;
    gpc_vertex_list * pl = &(polygon->contour[polygon->num_contours - 1]);
    pl->num_vertices = 1;

    const lv_vector_path_t * path = polygon_path;

    bool line_to = false;
    uint32_t pidx = 0;
    uint32_t len = lv_array_size(&path->ops);
    lv_vector_path_op_t * op = lv_array_front(&path->ops);

    for(uint32_t i = 0; i < len; i++) {
        switch(op[i]) {
            case LV_VECTOR_PATH_OP_MOVE_TO: {
                    if(line_to) {
                        polygon->num_contours++;
                        if(polygon->num_contours == (int32_t)init_caps) {
                            init_caps = init_caps << 1;
                            polygon->contour = (gpc_vertex_list *)lv_realloc(polygon->contour, sizeof(gpc_vertex_list) * init_caps);
                        }
                        pl = &(polygon->contour[polygon->num_contours - 1]);
                        pl->num_vertices = 1;
                        line_to = false;
                    }
                    pl->vertex = (gpc_vertex *)lv_array_at(&path->points, pidx);
                    pidx++;
                }
                break;
            case LV_VECTOR_PATH_OP_LINE_TO: {
                    pidx++;
                    pl->num_vertices++;
                    line_to = true;
                }
                break;
        }
    }
}

static void _clip_polygon(lv_vector_clipper_t type, lv_vector_path_t * result_path, const lv_vector_path_t * path1,
                          const lv_vector_path_t * path2)
{
    lv_vector_path_clear(result_path);

    gpc_polygon poly_a = {.num_contours = 0, .hole = NULL, .contour = NULL};
    gpc_polygon poly_b = {.num_contours = 0, .hole = NULL, .contour = NULL};
    gpc_polygon result = {.num_contours = 0, .hole = NULL, .contour = NULL};

    _make_polygon(&poly_a, path1);
    _make_polygon(&poly_b, path2);

    switch(type) {
        case LV_VECTOR_CLIPPER_INTERSECT:
            gpc_polygon_clip(GPC_INT, &poly_a, &poly_b, &result);
            break;
        case LV_VECTOR_CLIPPER_UNION:
            gpc_polygon_clip(GPC_UNION, &poly_a, &poly_b, &result);
            break;
        case LV_VECTOR_CLIPPER_XOR:
            gpc_polygon_clip(GPC_XOR, &poly_a, &poly_b, &result);
            break;
        case LV_VECTOR_CLIPPER_DIFF:
            gpc_polygon_clip(GPC_DIFF, &poly_a, &poly_b, &result);
            break;
    }

    lv_vector_path_op_t cur_op = LV_VECTOR_PATH_OP_MOVE_TO;

    for(int i = 0; i < result.num_contours; i++) {
        gpc_vertex_list * pl = &(result.contour[i]);
        for(int j = 0; j < pl->num_vertices; j++) {
            lv_fpoint_t * p = (lv_fpoint_t *)(&(pl->vertex[j]));
            if(cur_op == LV_VECTOR_PATH_OP_MOVE_TO) {
                lv_vector_path_move_to(result_path, p);
                cur_op = LV_VECTOR_PATH_OP_LINE_TO;
            }
            else {   // LV_VECTOR_PATH_OP_LINE_TO
                lv_vector_path_line_to(result_path, p);
            }
        }
        cur_op = LV_VECTOR_PATH_OP_MOVE_TO;
        lv_vector_path_close(result_path);
    }

    lv_free(poly_a.contour);
    lv_free(poly_b.contour);
    gpc_free_polygon(&result);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
bool lv_vector_path_to_polygon(lv_vector_path_t * result, const lv_vector_path_t * path)
{
    LV_ASSERT_NULL(result);
    LV_ASSERT_NULL(path);

    if(lv_array_size(&path->points) == 0) {
        LV_LOG_ERROR("path is empty!");
        return false;
    }

    if(path->flags & PATH_FLAG_POLYGON) {
        lv_vector_path_copy(result, path);
    }
    else {
        _flatten_path(result, path);
    }
    return true;
}

bool lv_vector_path_polygon_clipper(lv_vector_clipper_t type, lv_vector_path_t * result_path,
                                    const lv_vector_path_t * path1, const lv_vector_path_t * path2)
{
    LV_ASSERT_NULL(result_path);
    LV_ASSERT_NULL(path1);
    LV_ASSERT_NULL(path2);

    if(lv_array_size(&path1->points) == 0) {
        LV_LOG_ERROR("subject path is empty!");
        return false;
    }

    if(lv_array_size(&path2->points) == 0) {
        LV_LOG_ERROR("clip path is empty!");
        return false;
    }

    bool need_free1 = false, need_free2 = false;

    lv_vector_path_t * orig_path = (lv_vector_path_t *)path1;
    lv_vector_path_t * clip_path = (lv_vector_path_t *)path2;
    if(!(orig_path->flags & PATH_FLAG_POLYGON)) {
        need_free1 = true;
        orig_path = lv_vector_path_create(path1->quality);
        _flatten_path(orig_path, path1);
    }

    if(!(clip_path->flags & PATH_FLAG_POLYGON)) {
        need_free2 = true;
        clip_path = lv_vector_path_create(path2->quality);
        _flatten_path(clip_path, path2);
    }

    _clip_polygon(type, result_path, orig_path, clip_path);

    if(need_free1) {
        lv_vector_path_delete(orig_path);
    }
    if(need_free2) {
        lv_vector_path_delete(clip_path);
    }

    return true;
}

#endif
