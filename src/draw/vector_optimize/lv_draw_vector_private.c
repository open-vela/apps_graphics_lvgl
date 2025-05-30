
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
