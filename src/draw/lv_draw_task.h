/**
 * @file lv_draw_task.h
 *
 */

#ifndef LV_DRAW_TASK_H
#define LV_DRAW_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../lv_conf_internal.h"

#if LV_DRAW_TASK_USE_ALLOCATOR == LV_DRAW_TASK_DEFAULT_ALLOCATOR
#include "../stdlib/lv_mem.h"
#elif LV_DRAW_TASK_USE_ALLOCATOR == LV_DRAW_TASK_LINEAR_ALLOCATOR
#include "lv_draw_task_linear_alloc.h"
#endif

/*********************
 *      DEFINES
 *********************/

#if LV_DRAW_TASK_USE_ALLOCATOR == LV_DRAW_TASK_DEFAULT_ALLOCATOR
#define LV_DRAW_TASK_INIT_ALLOCATOR()
#define LV_DRAW_TASK_ZALLOC lv_malloc_zeroed
#define LV_DRAW_TASK_FREE lv_free
#define LV_DRAW_TASK_RESET()
#define LV_DRAW_TASK_DESTROY_ALLOCATOR()
#elif LV_DRAW_TASK_USE_ALLOCATOR == LV_DRAW_TASK_LINEAR_ALLOCATOR
#define LV_DRAW_TASK_INIT_ALLOCATOR lv_draw_task_linear_allocator_init
#define LV_DRAW_TASK_ZALLOC lv_draw_task_linear_allocator_zalloc
#define LV_DRAW_TASK_FREE(ptr)
#define LV_DRAW_TASK_RESET lv_draw_task_linear_allocator_reset
#define LV_DRAW_TASK_DESTROY_ALLOCATOR lv_draw_task_linear_allocator_destroy
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_TASK_H*/