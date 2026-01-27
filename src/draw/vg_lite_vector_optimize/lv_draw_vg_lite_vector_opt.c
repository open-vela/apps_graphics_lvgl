/**
 * @file lv_draw_vg_lite_vector_optimize.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "../vg_lite/lv_draw_vg_lite.h"
#include "lv_vg_lite_path_opt.h"

#if LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC_OPTIMIZE

#include "../vg_lite/lv_draw_vg_lite_type.h"
#include "../vg_lite/lv_vg_lite_path.h"
#include "../vg_lite/lv_vg_lite_pending.h"
#include "../vg_lite/lv_vg_lite_utils.h"
#include "../vg_lite/lv_vg_lite_grad.h"
#include "lv_vg_lite_stroke_opt.h"
#include "lv_vg_lite_stroke_path_opt.h"
#include <float.h>
#include <math.h>

#if LV_VG_LITE_USE_PATH_UPLOAD
static void vector_path_release_cb(void * entry, void * user_data)
{
    LV_UNUSED(user_data);
    lv_platform_path_base_t * impl = *((lv_platform_path_base_t **)entry);
    lv_vector_path_unref(impl);
}

void lv_draw_vg_lite_vector_init(struct _lv_draw_vg_lite_unit_t * u)
{
    LV_ASSERT_NULL(u);
    u->vector_pending = lv_vg_lite_pending_create(sizeof(lv_platform_path_base_t *), 8);
    lv_vg_lite_pending_set_free_cb(u->vector_pending, vector_path_release_cb, NULL);
}

void lv_draw_vg_lite_vector_deinit(struct _lv_draw_vg_lite_unit_t * u)
{
    LV_ASSERT_NULL(u);
    LV_ASSERT_NULL(u->vector_pending);
    lv_vg_lite_pending_destroy(u->vector_pending);
    u->vector_pending = NULL;
}
#endif

/*********************
 *      DEFINES
 *********************/

#define OPA_MIX(opa1, opa2) LV_UDIV255((opa1) * (opa2))

#if LV_VG_LITE_FLUSH_MAX_COUNT > 0
    #define DRAW_VECTOR_FLUSH_COUNT_MAX 0
#else
    /* When using IDLE Flush mode, reduce the number of flushes */
    #define DRAW_VECTOR_FLUSH_COUNT_MAX 8
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef void * path_drop_data_t;
typedef void (*path_drop_func_t)(struct _lv_draw_vg_lite_unit_t *, path_drop_data_t);

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void task_draw_cb(void * ctx, const lv_platform_path_base_t * path_impl, const lv_vector_draw_dsc_t * dsc);

static vg_lite_blend_t lv_blend_to_vg(lv_vector_blend_t blend);
static vg_lite_fill_t lv_fill_to_vg(lv_vector_fill_t fill_rule);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_vg_lite_vector(lv_draw_unit_t * draw_unit, const lv_draw_vector_task_dsc_t * dsc)
{
    if(dsc->draw_task_list.task_list == NULL)
        return;

    lv_layer_t * layer = dsc->base.layer;
    if(layer->draw_buf == NULL)
        return;

    LV_PROFILER_DRAW_BEGIN;
    _lv_vector_for_each_destroy_tasks((lv_vector_draw_task_list_t *)&dsc->draw_task_list, task_draw_cb, draw_unit);
    LV_PROFILER_DRAW_END;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static vg_lite_color_t lv_color32_to_vg(lv_color32_t color, lv_opa_t opa)
{
    uint8_t a = LV_OPA_MIX2(color.alpha, opa);
    if(a < LV_OPA_COVER) {
        color.red = LV_UDIV255(color.red * a);
        color.green = LV_UDIV255(color.green * a);
        color.blue = LV_UDIV255(color.blue * a);
    }
    return (uint32_t)a << 24 | (uint32_t)color.blue << 16 | (uint32_t)color.green << 8 | color.red;
}

static vg_lite_blend_t lv_blend_to_vg(lv_vector_blend_t blend)
{
    switch(blend) {
        case LV_VECTOR_BLEND_SRC_OVER:
            return VG_LITE_BLEND_SRC_OVER;
        case LV_VECTOR_BLEND_SCREEN:
            return VG_LITE_BLEND_SCREEN;
        case LV_VECTOR_BLEND_MULTIPLY:
            return VG_LITE_BLEND_MULTIPLY;
        case LV_VECTOR_BLEND_NONE:
            return VG_LITE_BLEND_NONE;
        case LV_VECTOR_BLEND_ADDITIVE:
            return VG_LITE_BLEND_ADDITIVE;
        case LV_VECTOR_BLEND_SRC_IN:
            return VG_LITE_BLEND_SRC_IN;
        case LV_VECTOR_BLEND_DST_OVER:
            return VG_LITE_BLEND_DST_OVER;
        case LV_VECTOR_BLEND_DST_IN:
            return VG_LITE_BLEND_DST_IN;
        case LV_VECTOR_BLEND_SUBTRACTIVE:
            return VG_LITE_BLEND_SUBTRACT;
        case LV_VECTOR_BLEND_DARKEN:
            return VG_LITE_BLEND_DARKEN;
        case LV_VECTOR_BLEND_LIGHTEN:
            return VG_LITE_BLEND_LIGHTEN;
        default:
            return VG_LITE_BLEND_SRC_OVER;
    }
}

static vg_lite_fill_t lv_fill_to_vg(lv_vector_fill_t fill_rule)
{
    switch(fill_rule) {
        case LV_VECTOR_FILL_NONZERO:
            return VG_LITE_FILL_NON_ZERO;
        case LV_VECTOR_FILL_EVENODD:
            return VG_LITE_FILL_EVEN_ODD;
        default:
            return VG_LITE_FILL_NON_ZERO;
    }
}

static void draw_fill(lv_draw_vg_lite_unit_t * u,
                      const lv_platform_path_base_t * impl,
                      lv_vg_lite_path_t * lv_vg_path,
                      const lv_vector_draw_dsc_t * dsc,
                      vg_lite_matrix_t * matrix,
                      const lv_fpoint_t * offset,
                      const lv_opa_t opa)
{
    LV_PROFILER_DRAW_BEGIN;

    const vg_lite_color_t vg_color = lv_color32_to_vg(dsc->fill_dsc->draw_attrs.color, OPA_MIX(dsc->fill_dsc->opa, opa));

    const vg_lite_blend_t blend = lv_blend_to_vg(dsc->blend_mode);
    const vg_lite_fill_t fill = lv_fill_to_vg(dsc->fill_dsc->fill_rule);

    /* If it is fill mode, the end op code should be added */
    lv_vg_lite_path_add_end(lv_vg_path);
#if LV_VG_LITE_USE_PATH_UPLOAD
    if(!VLM_PATH_GET_UPLOAD_BIT(lv_vg_path->base)) {
        lv_vg_lite_path_finish_upload(lv_vg_path);
    }
#endif

    vg_lite_path_t * vg_path = lv_vg_lite_path_get_path(lv_vg_path);
    LV_VG_LITE_ASSERT_PATH(vg_path);

    switch(dsc->fill_dsc->style) {
        case LV_VECTOR_DRAW_STYLE_SOLID: {
                /* normal draw shape */
                lv_vg_lite_draw(
                    &u->target_buffer,
                    vg_path,
                    fill,
                    matrix,
                    blend,
                    vg_color);
            }
            break;
        case LV_VECTOR_DRAW_STYLE_PATTERN: {
                /* draw image */
                vg_lite_buffer_t image_buffer;
                lv_image_decoder_dsc_t decoder_dsc;
                if(lv_vg_lite_buffer_open_image(&image_buffer, &decoder_dsc, dsc->fill_dsc->draw_attrs.img_dsc.src, false, true)) {
                    /* Calculate pattern matrix. Should start from path bond box, and also apply fill matrix. */
                    vg_lite_matrix_t pattern_matrix = *matrix;

                    if(dsc->fill_dsc->fill_units == LV_VECTOR_FILL_UNITS_OBJECT_BOUNDING_BOX) {
                        /* Convert to object bounding box coordinates */
                        vg_lite_translate(offset->x, offset->y, &pattern_matrix);
                    }

                    vg_lite_matrix_t fill_matrix;
                    lv_vg_lite_matrix(&fill_matrix, &dsc->fill_dsc->matrix);
                    lv_vg_lite_matrix_multiply(&pattern_matrix, &fill_matrix);

                    const lv_draw_image_dsc_t * img_dsc = &dsc->fill_dsc->draw_attrs.img_dsc;
                    lv_draw_image_dsc_t tmp_dsc;
                    if(opa < LV_OPA_COVER) {
                        tmp_dsc = dsc->fill_dsc->draw_attrs.img_dsc;
                        tmp_dsc.opa = OPA_MIX(tmp_dsc.opa, opa);
                        img_dsc = &tmp_dsc;
                    }

                    vg_lite_color_t recolor = lv_vg_lite_image_recolor(&image_buffer, img_dsc);

                    if(dsc->fill_dsc->draw_attrs.img_dsc.colorkey) {
                        lv_vg_lite_set_color_key(dsc->fill_dsc->draw_attrs.img_dsc.colorkey);
                    }

                    lv_vg_lite_draw_pattern(
                        &u->target_buffer,
                        vg_path,
                        fill,
                        matrix,
                        &image_buffer,
                        &pattern_matrix,
                        blend,
                        VG_LITE_PATTERN_COLOR,
                        0,
                        recolor,
                        VG_LITE_FILTER_BI_LINEAR);

                    if(dsc->fill_dsc->draw_attrs.img_dsc.colorkey) {
                        lv_vg_lite_set_color_key(NULL);
                    }

                    lv_vg_lite_pending_add(u->image_dsc_pending, &decoder_dsc);
                }
            }
            break;
        case LV_VECTOR_DRAW_STYLE_GRADIENT: {
                vg_lite_matrix_t grad_matrix = *matrix;
                vg_lite_matrix_t fill_matrix;
                lv_vg_lite_matrix(&fill_matrix, &dsc->fill_dsc->matrix);
                lv_vg_lite_matrix_multiply(&grad_matrix, &fill_matrix);

                const lv_vector_gradient_t * gradient = &dsc->fill_dsc->draw_attrs.gradient;
                lv_vector_gradient_t tmp_gradient;
                if(opa < LV_OPA_COVER) {
                    tmp_gradient = dsc->fill_dsc->draw_attrs.gradient;
                    for(uint16_t i = 0; i < tmp_gradient.stops_count; i++) {
                        tmp_gradient.stops[i].opa = OPA_MIX(tmp_gradient.stops[i].opa, opa);
                    }

                    gradient = &tmp_gradient;
                }

                lv_vg_lite_draw_grad(
                    u->grad_ctx,
                    &u->target_buffer,
                    vg_path,
                    gradient,
                    &grad_matrix,
                    matrix,
                    fill,
                    blend);
            }
            break;
        default:
            LV_LOG_WARN("unsupported style: %d", dsc->fill_dsc->style);
            break;
    }
#if LV_VG_LITE_USE_PATH_UPLOAD
    /* Increase ref count before adding to pending queue */
    lv_vector_path_ref((lv_platform_path_base_t *)impl);
    lv_vg_lite_pending_add(u->vector_pending, &impl);
#endif

    LV_PROFILER_DRAW_END;
}

static void draw_stroke(lv_draw_vg_lite_unit_t * u,
                        const lv_platform_path_base_t * impl,
                        lv_vg_lite_path_t * lv_vg_path,
                        const lv_vector_draw_dsc_t * dsc,
                        vg_lite_matrix_t * matrix,
                        const lv_opa_t opa)
{
    LV_PROFILER_DRAW_BEGIN;

    vg_lite_path_t * vg_path = lv_vg_lite_path_get_path(lv_vg_path);
    lv_vector_stroke_dsc_t * stroke_dsc = dsc->stroke_dsc;

#if LV_VG_LITE_USE_STROKE_TO_PATH
    lv_platform_vg_lite_path_t * impl_path = (lv_platform_vg_lite_path_t *)impl;

    lv_vg_lite_path_t * lv_vg_stroke_path;
    /* stroke_path_cache can be NULL on first use. */
    if(impl_path->stroke_path_cache && (impl_path->stroke_path_cache->base.path_length != 0)
       && (!(impl_path->base.flags & PATH_FLAG_CHANGED))
       && (!(stroke_dsc->stroke_dsc_changed))) {
        lv_vg_stroke_path = impl_path->stroke_path_cache;
    }
    else {
        lv_vg_stroke_path = lv_vg_lite_stroke_path_get(impl_path, stroke_dsc);
        if(!lv_vg_stroke_path) {
            LV_LOG_ERROR("convert stroke to path failed");
            LV_PROFILER_DRAW_END;
            return;
        }

#if LV_VG_LITE_USE_STROKE_PATH_CACHE
        impl_path->stroke_path_cache = lv_vg_stroke_path;
#endif
        impl_path->base.flags &= ~PATH_FLAG_CHANGED;
        stroke_dsc->stroke_dsc_changed = false;

        lv_vg_lite_path_add_end(lv_vg_stroke_path);
        lv_vg_lite_path_set_quality(lv_vg_stroke_path, vg_path->quality);

#if LV_VG_LITE_USE_PATH_UPLOAD
        lv_vg_lite_path_finish_upload(lv_vg_stroke_path);
#endif
    }

    vg_lite_path_t * vg_stroke_path = lv_vg_lite_path_get_path(lv_vg_stroke_path);
    const vg_lite_color_t vg_color = lv_color32_to_vg(dsc->stroke_dsc->draw_attrs.color, OPA_MIX(dsc->stroke_dsc->opa,
                                                                                                 opa));

#define STROKE_DROP() lv_vg_lite_stroke_path_drop(u, lv_vg_stroke_path)

#else
    LV_UNUSED(impl);
    u->stroke_path = lv_vg_path;
    u->stroke_path_in_use = false;

    lv_cache_entry_t * stroke_cache_entey = lv_vg_lite_stroke_get(u, lv_vg_path, stroke_dsc);
    if(!stroke_cache_entey) {
        LV_LOG_ERROR("convert stroke failed");
        LV_PROFILER_DRAW_END;
        return;
    }

    vg_lite_path_t * vg_stroke_path = lv_vg_lite_path_get_path(lv_vg_lite_stroke_get_path(stroke_cache_entey));

    /* set stroke params */
    vg_stroke_path->quality = vg_path->quality;
    vg_stroke_path->stroke_color = lv_color32_to_vg(dsc->stroke_dsc->draw_attrs.color, OPA_MIX(dsc->stroke_dsc->opa, opa));
    const vg_lite_color_t vg_color = 0;

    /* set stroke path bounding box */
    lv_memcpy(vg_stroke_path->bounding_box, vg_path->bounding_box, sizeof(vg_path->bounding_box));

    float expand_bound = stroke_dsc->opa ? stroke_dsc->width : 0;
    vg_stroke_path->bounding_box[0] -= expand_bound;
    vg_stroke_path->bounding_box[1] -= expand_bound;
    vg_stroke_path->bounding_box[2] += expand_bound + 1;
    vg_stroke_path->bounding_box[3] += expand_bound + 1;

#define STROKE_DROP() lv_vg_lite_stroke_drop(u, stroke_cache_entey)

#endif

    LV_VG_LITE_ASSERT_PATH(vg_stroke_path);

    const vg_lite_blend_t blend = lv_blend_to_vg(dsc->blend_mode);

    switch(stroke_dsc->style) {
        case LV_VECTOR_DRAW_STYLE_SOLID: {
                /* normal draw shape */
                lv_vg_lite_draw(
                    &u->target_buffer,
                    vg_stroke_path,
                    VG_LITE_FILL_NON_ZERO,
                    matrix,
                    blend,
                    vg_color);
            }
            break;
#if LV_VG_LITE_USE_STROKE_TO_PATH
        case LV_VECTOR_DRAW_STYLE_GRADIENT: {
                vg_lite_matrix_t grad_matrix = *matrix;
                vg_lite_matrix_t fill_matrix;
                lv_vg_lite_matrix(&fill_matrix, &stroke_dsc->matrix);
                lv_vg_lite_matrix_multiply(&grad_matrix, &fill_matrix);

                lv_vg_lite_draw_grad(
                    u->grad_ctx,
                    &u->target_buffer,
                    vg_stroke_path,
                    &stroke_dsc->draw_attrs.gradient,
                    &grad_matrix,
                    matrix,
                    VG_LITE_FILL_NON_ZERO,
                    blend);
            }
            break;
#endif /* LV_VG_LITE_USE_STROKE_TO_PATH */
        default:
            LV_LOG_WARN("unsupported style: %d", stroke_dsc->style);
            break;
    }
#if LV_VG_LITE_USE_PATH_UPLOAD
    /* Increase ref count before adding to pending queue */
    lv_vector_path_ref((lv_platform_path_base_t *)impl);
    lv_vg_lite_pending_add(u->vector_pending, &impl);
#endif
    STROKE_DROP();
    LV_PROFILER_DRAW_END;
}

static void task_draw_cb(void * ctx, const lv_platform_path_base_t * path_impl, const lv_vector_draw_dsc_t * dsc)
{
    LV_PROFILER_DRAW_BEGIN;
    lv_draw_vg_lite_unit_t * u = ctx;
    LV_VG_LITE_ASSERT_DEST_BUFFER(&u->target_buffer);

    lv_vector_fill_dsc_t * fill_dsc = dsc->fill_dsc;
    lv_vector_stroke_dsc_t * stroke_dsc = dsc->stroke_dsc;

    vg_lite_matrix_t matrix = u->global_matrix;

    const lv_area_t scissor_area = lv_matrix_is_identity((lv_matrix_t *)&matrix)
                                   ? dsc->scissor_area
                                   : lv_matrix_transform_area((lv_matrix_t *)&matrix, &dsc->scissor_area);

    /* clear area */
    if(!path_impl) {
        vg_lite_color_t c = lv_color32_to_vg(dsc->fill_dsc->draw_attrs.color, OPA_MIX(dsc->fill_dsc->opa, u->task_act->opa));
        vg_lite_rectangle_t rect;
        lv_vg_lite_rect(&rect, &scissor_area);
        LV_PROFILER_DRAW_BEGIN_TAG("vg_lite_clear");
        LV_VG_LITE_CHECK_ERROR(vg_lite_clear(&u->target_buffer, &rect, c), {
            lv_vg_lite_buffer_dump_info(&u->target_buffer);
            LV_LOG_ERROR("rect: X%d Y%d W%d H%d", rect.x, rect.y, rect.width, rect.height);
            lv_vg_lite_color_dump_info(c);
        });
        LV_PROFILER_DRAW_END_TAG("vg_lite_clear");
        LV_PROFILER_DRAW_END;
        return;
    }

    if(fill_dsc->opa == LV_OPA_TRANSP && stroke_dsc->opa == LV_OPA_TRANSP) {
        LV_LOG_TRACE("Full transparent, no need to draw");
        LV_PROFILER_DRAW_END;
        return;
    }

    /* transform matrix */
    vg_lite_matrix_t dsc_matrix;
    lv_vg_lite_matrix(&dsc_matrix, &dsc->matrix);
    lv_vg_lite_matrix_multiply(&matrix, &dsc_matrix);
    LV_VG_LITE_ASSERT_MATRIX(&matrix);

    /* convert path */
    lv_fpoint_t offset = {0, 0};
    lv_vg_lite_path_t * lv_vg_path = ((lv_platform_vg_lite_path_t *)path_impl)->vg_path;

    float min_x, min_y, max_x, max_y;
    lv_vg_lite_path_get_bounding_box(lv_vg_path, &min_x, &min_y, &max_x, &max_y);
    offset.x = lroundf(min_x);
    offset.y = lroundf(min_y);

    /* Save original bounding box and expand it */
    float orig_min_x = min_x, orig_min_y = min_y, orig_max_x = max_x, orig_max_y = max_y;
    if(vg_lite_query_feature(gcFEATURE_BIT_VG_SCISSOR)) {
        /* set scissor area */
        lv_vg_lite_set_scissor_area(&scissor_area);
        LV_LOG_TRACE("Set scissor area: X1:%" LV_PRId32 ", Y1:%" LV_PRId32 ", X2:%" LV_PRId32 ", Y2:%" LV_PRId32,
                     scissor_area.x1, scissor_area.y1, scissor_area.x2, scissor_area.y2);

        lv_vg_lite_path_expand_bounding_box(lv_vg_path);
    }
    else {
        /* calc inverse matrix */
        vg_lite_matrix_t result;
        if(!lv_vg_lite_matrix_inverse(&result, &matrix)) {
            LV_LOG_ERROR("no inverse matrix");
            lv_vg_lite_matrix_dump_info(&matrix);
            LV_PROFILER_DRAW_END;
            return;
        }

        /* Reverse the clip area on the source */
        lv_point_precise_t p1 = { scissor_area.x1, scissor_area.y1 };
        lv_point_precise_t p1_res = lv_vg_lite_matrix_transform_point(&result, &p1);

        /* vg-lite bounding_box will crop the pixels on the edge, so +1px is needed here */
        lv_point_precise_t p2 = { scissor_area.x2 + 1, scissor_area.y2 + 1 };
        lv_point_precise_t p2_res = lv_vg_lite_matrix_transform_point(&result, &p2);

        lv_vg_lite_path_set_bounding_box(lv_vg_path, p1_res.x, p1_res.y, p2_res.x, p2_res.y);
    }

    const lv_opa_t layer_opa = u->task_act->opa;

    if(fill_dsc->opa) {
        draw_fill(u, path_impl, lv_vg_path, dsc, &matrix, &offset, layer_opa);
    }

    if(stroke_dsc->opa) {
        draw_stroke(u, path_impl, lv_vg_path, dsc, &matrix, layer_opa);
    }

#if LV_VG_LITE_USE_PATH_UPLOAD
    u->vector_count++;
    if(u->vector_count > DRAW_VECTOR_FLUSH_COUNT_MAX) {
        /* Flush in time to avoid accumulation of drawing commands */
        lv_vg_lite_flush(u);
    }
#else
    /* Flush in time to avoid accumulation of drawing commands */
    lv_vg_lite_flush(u);
#endif

    /* Restore original bounding box */
    lv_vg_lite_path_set_bounding_box(lv_vg_path, orig_min_x, orig_min_y, orig_max_x, orig_max_y);

    LV_PROFILER_DRAW_END;
}

#endif /*LV_USE_DRAW_VG_LITE && LV_USE_VECTOR_GRAPHIC*/
