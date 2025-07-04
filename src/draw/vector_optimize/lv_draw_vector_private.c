
/**
 * @file lv_draw_vector_private.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_vector_private.h"

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
void lv_vector_path_ref(lv_platform_path_base_t * impl)
{
    LV_ASSERT_NULL(impl);
    impl->ref_count++;
}

void lv_vector_path_unref(lv_platform_path_base_t * impl)
{
    LV_ASSERT_NULL(impl);

    if(impl->ref_count == 0) LV_ASSERT(0);

    if(--impl->ref_count == 0) {
        impl->handlers->destroy(impl);
    }
}

void lv_vector_path_get_data(lv_platform_path_base_t * impl, lv_vector_path_data_t * data)
{
    LV_ASSERT_NULL(impl);
    LV_ASSERT_NULL(data);

    impl->handlers->get_data(impl, data);
}

void lv_vector_path_transform_path(const lv_platform_path_base_t * impl, lv_vector_path_transform_data_t * path_data)
{
    LV_ASSERT_NULL(impl);
    LV_ASSERT_NULL(path_data);

    impl->handlers->transform_path((lv_platform_path_base_t *)impl, path_data);
}

bool lv_vector_path_impl_is_empty(const lv_platform_path_base_t * impl)
{
    LV_ASSERT_NULL(impl);

    return impl->handlers->is_empty((lv_platform_path_base_t *)impl);
}

#if LV_USE_VECTOR_DUMP_INFO
static void _dump_matrix(const lv_matrix_t * matrix)
{
    LV_LOG_USER("       Matrix:");
    LV_LOG_USER("       [%f, %f, %f]", matrix->m[0][0], matrix->m[0][1], matrix->m[0][2]);
    LV_LOG_USER("       [%f, %f, %f]", matrix->m[1][0], matrix->m[1][1], matrix->m[1][2]);
    LV_LOG_USER("       [%f, %f, %f]", matrix->m[2][0], matrix->m[2][1], matrix->m[2][2]);
}

static void _dump_color(lv_color32_t color)
{
    LV_LOG_USER("       Color:(R:%d, G:%d, B:%d, A:%d)",
                color.red,
                color.green,
                color.blue,
                color.alpha);
}

static void _dump_fill_dsc(const lv_vector_fill_dsc_t * fill_dsc)
{
    LV_LOG_USER("  Fill Descriptor:");
    LV_LOG_USER("    Style: %d", fill_dsc->style);
    LV_LOG_USER("    Opacity: %d", fill_dsc->opa);
    LV_LOG_USER("    Fill Rule: %d", fill_dsc->fill_rule);
    LV_LOG_USER("    Fill Units: %d", fill_dsc->fill_units);

    switch(fill_dsc->style) {
        case LV_VECTOR_DRAW_STYLE_SOLID:
            _dump_color(fill_dsc->draw_attrs.color);
            break;
        case LV_VECTOR_DRAW_STYLE_GRADIENT:
            LV_LOG_USER("    Gradient: %d stops", fill_dsc->draw_attrs.gradient.stops_count);
            break;
        case LV_VECTOR_DRAW_STYLE_PATTERN:
            LV_LOG_USER("    Pattern Image");
            break;
    }

    _dump_matrix(&fill_dsc->matrix);
}

static void _dump_stroke_dsc(const lv_vector_stroke_dsc_t * stroke_dsc)
{
    LV_LOG_USER("  Stroke Descriptor:");
    LV_LOG_USER("    Style: %d", stroke_dsc->style);
    LV_LOG_USER("    Opacity: %d", stroke_dsc->opa);
    LV_LOG_USER("    Width: %f", stroke_dsc->width);
    LV_LOG_USER("    Cap: %d", stroke_dsc->cap);
    LV_LOG_USER("    Join: %d", stroke_dsc->join);
    LV_LOG_USER("    Miter Limit: %d", stroke_dsc->miter_limit);

    if(stroke_dsc->dash_count > 0) {
        LV_LOG_USER("    Dash Pattern:");
        for(int i = 0; i < stroke_dsc->dash_count; i++) {
            LV_LOG_USER("      [%d]: %f", i, stroke_dsc->dash_pattern[i]);
        }
    }

    switch(stroke_dsc->style) {
        case LV_VECTOR_DRAW_STYLE_SOLID:
            _dump_color(stroke_dsc->draw_attrs.color);
            break;
        case LV_VECTOR_DRAW_STYLE_GRADIENT:
            LV_LOG_USER("    Gradient: %d stops", stroke_dsc->draw_attrs.gradient.stops_count);
            break;
        case LV_VECTOR_DRAW_STYLE_PATTERN:
            LV_LOG_USER("    Pattern Image");
            break;
        default:
            break;
    }

    _dump_matrix(&stroke_dsc->matrix);
}

void lv_vector_dump_path_info(const lv_platform_path_base_t * impl)
{
    impl->handlers->dump_path_info((lv_platform_path_base_t *)impl);
}

void lv_vector_dump_dsc_info(const lv_vector_draw_dsc_t * dsc)
{
    LV_LOG_USER("Vector Draw Descriptor:");
    LV_LOG_USER("  Blend Mode: %d", dsc->blend_mode);

    LV_LOG_USER("  Scissor Area: [%d, %d, %d, %d]",
                dsc->scissor_area.x1,
                dsc->scissor_area.y1,
                dsc->scissor_area.x2,
                dsc->scissor_area.y2);

    _dump_matrix(&dsc->matrix);
    _dump_fill_dsc(dsc->fill_dsc);
    _dump_stroke_dsc(dsc->stroke_dsc);
}

void lv_vector_for_each_task_dump_info(const lv_vector_draw_task_list_t * draw_task_list)
{
    if(!draw_task_list || !draw_task_list->task_list) {
        LV_LOG_WARN("Invalid task list");
        return;
    }

    _lv_vector_draw_task * task = _lv_ll_get_head(draw_task_list->task_list);
    _lv_vector_draw_task * next_task = NULL;

    while(task != NULL) {
        next_task = _lv_ll_get_next(draw_task_list->task_list, task);
        if(task->path_impl) {
            lv_vector_dump_path_info(task->path_impl);
            lv_vector_dump_dsc_info(task->dsc);
        }
        task = next_task;
    }
}
#endif

void _lv_vector_for_each_destroy_tasks(lv_vector_draw_task_list_t * draw_task_list, vector_draw_task_cb cb, void * data)
{
    if(!draw_task_list || !draw_task_list->task_list) {
        LV_LOG_WARN("Invalid task list");
        return;
    }

    _lv_vector_draw_task * task = _lv_ll_get_head(draw_task_list->task_list);
    _lv_vector_draw_task * next_task = NULL;

    while(task != NULL) {
        next_task = _lv_ll_get_next(draw_task_list->task_list, task);
        _lv_ll_remove(draw_task_list->task_list, task);

        if(cb) {
            cb(data, task->path_impl, task->dsc);
        }

        if(task->path_impl) {
            if(task->path_impl->ref_count <= 0) {
                LV_LOG_ERROR("Invalid path refcount: %d", task->path_impl->ref_count);
            }
            else {
                lv_vector_path_unref(task->path_impl);
            }
        }

        lv_free(task);
        task = next_task;
    }

    if(draw_task_list->allocator) {
        lv_linear_allocator_delete(draw_task_list->allocator);
        draw_task_list->allocator = NULL;
    }
    lv_free(draw_task_list->task_list);
}

#endif
