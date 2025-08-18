
/**
 * @file lv_vector_clipper.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <math.h>

#include "lv_vector_clipper.h"
#include "lv_vector_polygon.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE
#include "../../libs/gpc/gpc.h"

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

static lv_vector_path_data_t * _make_polygon(gpc_polygon * polygon, const lv_vector_path_t * polygon_path)
{
    size_t init_caps = 4;
    polygon->contour = (gpc_vertex_list *)lv_malloc(init_caps * sizeof(gpc_vertex_list));
    polygon->num_contours = 1;
    gpc_vertex_list * pl = &(polygon->contour[polygon->num_contours - 1]);
    pl->num_vertices = 1;

    const lv_vector_path_t * path = polygon_path;

    lv_vector_path_data_t * path_data = lv_malloc_zeroed(sizeof(lv_vector_path_data_t));
    LV_ASSERT_MALLOC(path_data);
    lv_array_init(&path_data->ops, 8, sizeof(lv_vector_path_op_t));
    lv_array_init(&path_data->points, 8, sizeof(lv_fpoint_t));

    lv_vector_path_get_data(path->impl, path_data);

    bool line_to = false;
    uint32_t pidx = 0;
    uint32_t len = lv_array_size(&path_data->ops);
    lv_vector_path_op_t * op = lv_array_front(&path_data->ops);

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
                    pl->vertex = (gpc_vertex *)lv_array_at(&path_data->points, pidx);
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

    return path_data;
}

static void _clip_polygon(lv_vector_clipper_t type, lv_vector_path_t * result_path, const lv_vector_path_t * path1,
                          const lv_vector_path_t * path2)
{
    lv_vector_path_clear(result_path);

    gpc_polygon poly_a = {.num_contours = 0, .hole = NULL, .contour = NULL};
    gpc_polygon poly_b = {.num_contours = 0, .hole = NULL, .contour = NULL};
    gpc_polygon result = {.num_contours = 0, .hole = NULL, .contour = NULL};

    lv_vector_path_data_t * path_data_a = _make_polygon(&poly_a, path1);
    lv_vector_path_data_t * path_data_b = _make_polygon(&poly_b, path2);

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

    lv_array_deinit(&path_data_a->ops);
    lv_array_deinit(&path_data_a->points);
    lv_free(path_data_a);

    lv_array_deinit(&path_data_b->ops);
    lv_array_deinit(&path_data_b->points);
    lv_free(path_data_b);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool lv_vector_path_polygon_clipper(lv_vector_clipper_t type, lv_vector_path_t * result_path,
                                    const lv_vector_path_t * path1, const lv_vector_path_t * path2)
{
    LV_ASSERT_NULL(result_path);
    LV_ASSERT_NULL(path1);
    LV_ASSERT_NULL(path2);

    if(lv_vector_path_is_empty(path1)) {
        LV_LOG_ERROR("path1 is empty!");
        return false;
    }

    if(lv_vector_path_is_empty(path2)) {
        LV_LOG_ERROR("path2 is empty!");
        return false;
    }

    bool need_free1 = false, need_free2 = false;

    lv_vector_path_t * orig_path = (lv_vector_path_t *)path1;
    lv_vector_path_t * clip_path = (lv_vector_path_t *)path2;
    if(!(orig_path->impl->flags & PATH_FLAG_POLYGON)) {
        need_free1 = true;
        orig_path = lv_vector_path_create(0);
        lv_vector_path_to_polygon(orig_path, path1);
    }

    if(!(clip_path->impl->flags & PATH_FLAG_POLYGON)) {
        need_free2 = true;
        clip_path = lv_vector_path_create(0);
        lv_vector_path_to_polygon(clip_path, path2);
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
