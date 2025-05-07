/**
 * @file lv_draw_task_linear_alloc.h
 *
 */

#ifndef LV_DRAW_TASK_LINEAR_ALLOC_H
#define LV_DRAW_TASK_LINEAR_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"

#if LV_DRAW_TASK_USE_ALLOCATOR == LV_DRAW_TASK_LINEAR_ALLOCATOR

#include "../misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the linear allocator for the draw task
 */
void lv_draw_task_linear_allocator_init(void);

/**
 * Allocate memory from the linear allocator
 * @param size The size of the memory to allocate
 * @return void* The pointer to the allocated memory
 */
void * lv_draw_task_linear_allocator_zalloc(size_t size);

/**
 * Reset the linear allocator for the draw task
 */
void lv_draw_task_linear_allocator_reset(void);

/**
 * Destroy the linear allocator for the draw task
 */
void lv_draw_task_linear_allocator_destroy(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_DRAW_TASK_LINEAR_ALLOCATOR*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_TASK_LINEAR_ALLOC_H*/