/**
 * @file lv_linear_allocator.h
 * Linear allocator. The memory allocated by 'lv_mem' module.
 */

#ifndef LV_LINEAR_ALLOCATOR_H
#define LV_LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"

#include "lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    LV_MEM_ALIGN_1 = 1,
    LV_MEM_ALIGN_4 = 4,
    LV_MEM_ALIGN_8 = 8,
    LV_MEM_ALIGN_16 = 16,
    LV_MEM_ALIGN_32 = 32,
    LV_MEM_ALIGN_64 = 64,
} lv_mem_align_type_t;

typedef struct _linear_allocator {
    size_t total_memory;
    void * (* alloc)(struct _linear_allocator * mem, size_t size);
} lv_linear_allocator;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Create a linear memory allocator with specified alignment
 * @param align Memory alignment type, must be between
 *              LV_MEM_ALIGN_2(2^1) to LV_MEM_ALIGN_32(2^5)
 * @param block_size Initial memory block size in bytes.
 *              - 0 will use LV_LINEAR_DEFAULT_BLOCK_SIZE
 *              - Values < default automatically upgrade to default
 * @return Pointer to new allocator instance, NULL indicates failure
 */
lv_linear_allocator * lv_linear_allocator_create(lv_mem_align_type_t align, size_t block_size);

/**
 * @brief Reset linear allocator to the initial state
 * @param mem Target allocator instance pointer. After reset,
 *            the pointer should be explicitly set to the head of the first block
 */
void lv_linear_allocator_reset(lv_linear_allocator * mem);

/**
 * @brief Destroy linear allocator and release all memory blocks
 * @param mem Target allocator instance pointer. After deletion,
 *            the pointer should be explicitly set to NULL
 */
void lv_linear_allocator_delete(lv_linear_allocator * mem);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_LINEAR_ALLOCATOR_H */
