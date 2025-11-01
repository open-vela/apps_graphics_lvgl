/**
 * @file lv_vg_lite_draw.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_draw_vg_lite.h"

#if LV_USE_DRAW_VG_LITE

#include "../lv_draw.h"
#include "lv_draw_vg_lite_type.h"
#include "lv_vg_lite_path.h"
#include "lv_vg_lite_utils.h"
#include "lv_vg_lite_decoder.h"
#include "lv_vg_lite_grad.h"
#include "lv_vg_lite_pending.h"
#include "lv_vg_lite_stroke.h"
#include "lv_vg_lite_stroke_path.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE
    #include "../vg_lite_vector_optimize/lv_vg_lite_stroke_opt.h"
    #include "../vg_lite_vector_optimize/lv_vg_lite_stroke_path_opt.h"
#endif
/*********************
 *      DEFINES
 *********************/

#define VG_LITE_DRAW_UNIT_ID 2

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int32_t draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);

static int32_t draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);

static int32_t draw_delete(lv_draw_unit_t * draw_unit);

static void draw_event_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_vg_lite_init(void)
{
#if LV_VG_LITE_USE_GPU_INIT
    extern void gpu_init(void);
    static bool inited = false;
    if(!inited) {
        gpu_init();
        inited = true;
    }
#endif

    lv_vg_lite_dump_info();

    lv_draw_buf_vg_lite_init_handlers();

    lv_draw_vg_lite_unit_t * unit = lv_draw_create_unit(sizeof(lv_draw_vg_lite_unit_t));
    unit->base_unit.dispatch_cb = draw_dispatch;
    unit->base_unit.evaluate_cb = draw_evaluate;
    unit->base_unit.delete_cb = draw_delete;
    unit->base_unit.event_cb = draw_event_cb;
    unit->base_unit.name = "VG_LITE";

    lv_vg_lite_image_dsc_init(unit);
#if LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE
    unit->grad_ctx = lv_vg_lite_grad_ctx_create(LV_VG_LITE_GRAD_CACHE_CNT, unit);
    lv_vg_lite_stroke_init(unit, LV_VG_LITE_STROKE_CACHE_CNT);
    lv_vg_lite_stroke_path_init(unit);
#endif
    lv_vg_lite_path_init(unit);
    lv_vg_lite_decoder_init();
    lv_draw_vg_lite_label_init(unit);

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE && LV_VG_LITE_USE_PATH_UPLOAD
    lv_draw_vg_lite_vector_init(unit);
#endif
}

void lv_draw_vg_lite_deinit(void)
{
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool check_image_is_supported(const lv_draw_image_dsc_t * dsc)
{
    return lv_vg_lite_is_src_cf_supported(dsc->header.cf);
}

static bool check_arc_is_supported(const lv_draw_arc_dsc_t * dsc)
{
    if(dsc->img_src == NULL) {
        return true;
    }

    lv_image_header_t header;
    lv_result_t res = lv_image_decoder_get_info(dsc->img_src, &header);
    if(res != LV_RESULT_OK) {
        LV_LOG_TRACE("get image info failed");
        return false;
    }

    return lv_vg_lite_is_src_cf_supported(header.cf);
}

static void draw_execute(lv_draw_vg_lite_unit_t * u)
{
    lv_draw_task_t * t = u->task_act;
    lv_draw_unit_t * draw_unit = (lv_draw_unit_t *)u;

    lv_layer_t * layer = u->base_unit.target_layer;

    lv_vg_lite_buffer_from_draw_buf(&u->target_buffer, layer->draw_buf);

    /* VG-Lite will output premultiplied image, set the flag correspondingly. */
    lv_draw_buf_set_flag(layer->draw_buf, LV_IMAGE_FLAGS_PREMULTIPLIED);

    vg_lite_identity(&u->global_matrix);
    if(layer->buf_area.x1 || layer->buf_area.y1) {
        vg_lite_translate(-layer->buf_area.x1, -layer->buf_area.y1, &u->global_matrix);
    }

#if LV_DRAW_TRANSFORM_USE_MATRIX
    vg_lite_matrix_t layer_matrix;
    lv_vg_lite_matrix(&layer_matrix, &t->matrix);
    lv_vg_lite_matrix_multiply(&u->global_matrix, &layer_matrix);

    /* Crop out extra pixels drawn due to scaling accuracy issues */
    lv_area_t scissor_area = layer->phy_clip_area;
#else
    lv_area_t scissor_area = layer->_clip_area;
#endif
    lv_area_move(&scissor_area, -layer->buf_area.x1, -layer->buf_area.y1);
    if(vg_lite_query_feature(gcFEATURE_BIT_VG_SCISSOR)) {
        lv_vg_lite_set_scissor_area(&scissor_area);
    }

    switch(t->type) {
        case LV_DRAW_TASK_TYPE_LETTER:
            lv_draw_vg_lite_letter(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_LABEL:
            lv_draw_vg_lite_label(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_FILL:
            lv_draw_vg_lite_fill(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_BORDER:
            lv_draw_vg_lite_border(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_BOX_SHADOW:
            lv_draw_vg_lite_box_shadow(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_IMAGE:
            lv_draw_vg_lite_img(draw_unit, t->draw_dsc, &t->area, false);
            break;
        case LV_DRAW_TASK_TYPE_ARC:
            lv_draw_vg_lite_arc(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_LINE:
            lv_draw_vg_lite_line(draw_unit, t->draw_dsc);
            break;
        case LV_DRAW_TASK_TYPE_LAYER:
            lv_draw_vg_lite_layer(draw_unit, t->draw_dsc, &t->area);
            break;
        case LV_DRAW_TASK_TYPE_TRIANGLE:
            lv_draw_vg_lite_triangle(draw_unit, t->draw_dsc);
            break;
        case LV_DRAW_TASK_TYPE_MASK_RECTANGLE:
            lv_draw_vg_lite_mask_rect(draw_unit, t->draw_dsc, &t->area);
            break;
#if LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE
        case LV_DRAW_TASK_TYPE_VECTOR:
            lv_draw_vg_lite_vector(draw_unit, t->draw_dsc);
            break;
#endif
        default:
            break;
    }

    lv_vg_lite_flush(u);
}

static int32_t draw_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    lv_draw_vg_lite_unit_t * u = (lv_draw_vg_lite_unit_t *)draw_unit;

    /* Return immediately if it's busy with draw task. */
    if(u->task_act) {
        return 0;
    }

    /* Try to get an ready to draw. */
    lv_draw_task_t * t = lv_draw_get_next_available_task(layer, NULL, VG_LITE_DRAW_UNIT_ID);

    /* Return 0 is no selection, some tasks can be supported by other units. */
    if(!t || t->preferred_draw_unit_id != VG_LITE_DRAW_UNIT_ID) {
        lv_vg_lite_finish(u);
        return -1;
    }

    /* Return if target buffer format is not supported. */
    if(!lv_vg_lite_is_dest_cf_supported(layer->color_format)) {
        return -1;
    }

    void * buf = lv_draw_layer_alloc_buf(layer);
    if(!buf) {
        return -1;
    }

    t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
    u->base_unit.target_layer = layer;
    u->base_unit.clip_area = &t->clip_area;
    u->task_act = t;

    draw_execute(u);

    u->task_act->state = LV_DRAW_TASK_STATE_READY;
    u->task_act = NULL;

    /*The draw unit is free now. Request a new dispatching as it can get a new task*/
    lv_draw_dispatch_request();

    return 1;
}

static int32_t draw_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);

    /* Return if target buffer format is not supported. */
    const lv_draw_dsc_base_t * base_dsc = task->draw_dsc;
    if(!lv_vg_lite_is_dest_cf_supported(base_dsc->layer->color_format)) {
        return -1;
    }

    switch(task->type) {
        case LV_DRAW_TASK_TYPE_LABEL:
        case LV_DRAW_TASK_TYPE_LETTER:
        case LV_DRAW_TASK_TYPE_FILL:
        case LV_DRAW_TASK_TYPE_BORDER:
#if LV_VG_LITE_USE_BOX_SHADOW
        case LV_DRAW_TASK_TYPE_BOX_SHADOW:
#endif
        case LV_DRAW_TASK_TYPE_LAYER:
        case LV_DRAW_TASK_TYPE_LINE:
        case LV_DRAW_TASK_TYPE_TRIANGLE:
        case LV_DRAW_TASK_TYPE_MASK_RECTANGLE:

#if LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE
        case LV_DRAW_TASK_TYPE_VECTOR:
#endif
            break;

        case LV_DRAW_TASK_TYPE_ARC: {
                if(!check_arc_is_supported(task->draw_dsc)) {
                    return 0;
                }
            }
            break;

        case LV_DRAW_TASK_TYPE_IMAGE: {
                if(!check_image_is_supported(task->draw_dsc)) {
                    return 0;
                }
            }
            break;

        default:
            /*The draw unit is not able to draw this task. */
            return 0;
    }

    /* The draw unit is able to draw this task. */
    task->preference_score = 80;
    task->preferred_draw_unit_id = VG_LITE_DRAW_UNIT_ID;
    return 1;
}

static int32_t draw_delete(lv_draw_unit_t * draw_unit)
{
    lv_draw_vg_lite_unit_t * unit = (lv_draw_vg_lite_unit_t *)draw_unit;

    lv_vg_lite_image_dsc_deinit(unit);
#if LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE
    lv_vg_lite_grad_ctx_delete(unit->grad_ctx);
    lv_vg_lite_stroke_deinit(unit);
    lv_vg_lite_stroke_path_deinit(unit);
#endif
    lv_vg_lite_path_deinit(unit);
    lv_vg_lite_decoder_deinit();
    lv_draw_vg_lite_label_deinit(unit);

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE && LV_VG_LITE_USE_PATH_UPLOAD
    lv_draw_vg_lite_vector_deinit(unit);
#endif
    return 1;
}

static void draw_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_draw_vg_lite_unit_t * unit = lv_event_get_current_target(e);

    switch(code) {
        case LV_EVENT_CANCEL: {
#if LV_USE_VECTOR_GRAPHIC || LV_USE_VECTOR_GRAPHIC_OPTIMIZE
                /**
                 * Because VG-Lite will deinitialize the context (including the GPU independent heap)
                 * before the GPU goes to sleep, it is necessary to first discard and dereference
                 * all caches that depend on the independent heap.
                 */
                lv_cache_drop_all(lv_vg_lite_grad_ctx_get_cache(unit->grad_ctx), NULL);
                lv_cache_drop_all(unit->stroke_cache, NULL);
                LV_LOG_INFO("dropt all cache");
#endif
            }
            break;
        case LV_EVENT_REFR_START: {
                unit->run_time_start = lv_vg_lite_get_running_time();
            }
            break;
        case LV_EVENT_RENDER_READY: {
                unit->run_time_last = lv_vg_lite_get_running_time() - unit->run_time_start;
                if(e->param) {
                    *(lv_value_precise_t *)(e->param) += (lv_value_precise_t)unit->run_time_last;
                }
            }
            break;
        case LV_EVENT_FOCUSED:
            lv_vg_lite_set_dump_param_enable(true);
            break;
        case LV_EVENT_DEFOCUSED:
            lv_vg_lite_set_dump_param_enable(false);
            break;
        case LV_EVENT_HIT_TEST:
            lv_vg_lite_dump_info();
            break;
        default:
            break;
    }
}

#endif /*LV_USE_DRAW_VG_LITE*/
