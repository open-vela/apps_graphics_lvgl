/**
 * @file lv_draw_task_linear_alloc.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_task_linear_alloc.h"

#if LV_DRAW_TASK_USE_ALLOCATOR == LV_DRAW_TASK_LINEAR_ALLOCATOR

#include "../core/lv_global.h"
#include <dirent.h>
#include <string.h>

#include "../misc/lv_linear_allocator.h"

/*********************
 *      DEFINES
 *********************/

#define lv_draw_task_allocator_ctx LV_GLOBAL_DEFAULT()->draw_task_allocator_ctx

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_task_linear_allocator_init(void)
{
    LV_ASSERT(lv_draw_task_allocator_ctx == NULL);

    lv_draw_task_allocator_ctx = lv_linear_allocator_create(LV_MEM_ALIGN_8, LV_DRAW_TASK_LINEAR_ALLOCATOR_SIZE);
}

void * lv_draw_task_linear_allocator_zalloc(size_t size)
{
    LV_ASSERT_NULL(lv_draw_task_allocator_ctx);
    LV_ASSERT(size > 0);

    void * ptr = ((lv_linear_allocator *)lv_draw_task_allocator_ctx)->alloc((lv_linear_allocator *)
                                                                            lv_draw_task_allocator_ctx, size);

    if(ptr != NULL) {
        lv_memzero(ptr, size);
        return ptr;
    }

    return NULL;
}

void lv_draw_task_linear_allocator_reset(void)
{
    LV_ASSERT_NULL(lv_draw_task_allocator_ctx);

    lv_linear_allocator_reset((lv_linear_allocator *)lv_draw_task_allocator_ctx);
}

void lv_draw_task_linear_allocator_destroy(void)
{
    LV_ASSERT_NULL(lv_draw_task_allocator_ctx);

    lv_linear_allocator_delete((lv_linear_allocator *)lv_draw_task_allocator_ctx);
    lv_draw_task_allocator_ctx = NULL;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /*LV_DRAW_TASK_LINEAR_ALLOCATOR*/