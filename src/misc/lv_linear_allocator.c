/**
 * @file lv_linear_allocator.c
 * Linear allocator. The memory allocated by 'lv_mem' module.
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_linear_allocator.h"

#include "../stdlib/lv_mem.h"
#include "../stdlib/lv_string.h"

#include "lv_assert.h"
#include "lv_math.h"

/*********************
 *      DEFINES
 *********************/
#define MAX_BLOCK_SIZE   ((size_t)65536) // 64kb

#define MEM_ALIGN(x, shift) \
    (((x) + ((shift) - 1)) & ~(shift - 1))

#define LV_LINEAR_DEFAULT_BLOCK_SIZE    256
#define LV_MEM_DEFAULT_ALIGN_4     4

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _mem_block {
    struct _mem_block * next;
    size_t block_size;
    uint8_t data[1];
} lv_mem_block_t;

typedef struct {
    lv_linear_allocator base;
    lv_mem_block_t * blocks;
    lv_mem_block_t * pre_block;
    lv_mem_block_t * cur_block;
    void * next_ptr;
    lv_mem_align_type_t align;
    size_t init_block_size;
} lv_linear_allocator_private;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/
static inline bool fits_block(lv_linear_allocator_private * mem, size_t size)
{
    return mem->next_ptr &&
           (((uint8_t *)mem->next_ptr) + size) <= (((uint8_t *)mem->cur_block) + mem->cur_block->block_size);
}

static bool ensure_next(lv_linear_allocator_private * mem, size_t size)
{
    while(!fits_block(mem, size) && mem->cur_block && mem->cur_block->next) {
        if(mem->next_ptr == (void *)(&mem->cur_block->data)) {
            lv_mem_block_t * drop_block = mem->cur_block;
            size_t drop_block_size = drop_block->block_size;
            mem->cur_block = mem->cur_block->next;
            if(mem->pre_block) {
                mem->pre_block->next = mem->cur_block;
            }
            else {
                mem->blocks = mem->cur_block;
            }
            lv_free(drop_block);
            mem->base.total_memory -= drop_block_size;
        }
        else {
            mem->pre_block = mem->cur_block;
            mem->cur_block = mem->cur_block->next;
        }
        mem->next_ptr = (void *)(&mem->cur_block->data);
        mem->next_ptr = (void *)MEM_ALIGN((uintptr_t)mem->next_ptr, (uintptr_t)mem->align);
    }

    if(fits_block(mem, size)) {
        return true;
    }

    size_t block_size = MEM_ALIGN(mem->init_block_size, LV_MEM_DEFAULT_ALIGN_4);

    size_t block_hdr_size = sizeof(lv_mem_block_t *) + sizeof(size_t);

    if(mem->blocks == NULL) {
        block_size = LV_MAX(block_size, size + block_hdr_size);
    }

    while((block_size - block_hdr_size) < size) {
        block_size *= 2;
    }

    lv_mem_block_t * block = lv_malloc(block_size);
    LV_ASSERT_MALLOC(block);
    if(block == NULL) {
        LV_LOG_ERROR("allocate memory failed for linear allocator");
        return false;
    }

    mem->base.total_memory += block_size;

    block->next = NULL;
    block->block_size = block_size;

    if(mem->cur_block) {
        mem->cur_block->next = block;
        mem->pre_block = mem->cur_block;
    }

    mem->cur_block = block;

    if(!mem->blocks) {
        mem->blocks = mem->cur_block;
    }

    mem->next_ptr = (void *)(&block->data);
    mem->next_ptr = (void *)MEM_ALIGN((uintptr_t)mem->next_ptr, (uintptr_t)mem->align);
    return true;
}

static void * _linear_alloc(struct _linear_allocator * mem, size_t size)
{
    if(size > MAX_BLOCK_SIZE) {
        LV_LOG_ERROR("Allocating more than 64kb of memory using the linear allocator is not allowed!");
        return NULL;
    }

    lv_linear_allocator_private * p = (lv_linear_allocator_private *)mem;
    size = MEM_ALIGN(size, p->align);

    if(ensure_next(p, size) == false) {
        LV_LOG_ERROR("allocate memory failed for linear allocator");
        return NULL;
    }

    void * ptr = p->next_ptr;
    p->next_ptr = ((uint8_t *)p->next_ptr) + size;
    return ptr;
}

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_linear_allocator * lv_linear_allocator_create(lv_mem_align_type_t align, size_t block_size)
{
    block_size = LV_MAX(block_size, LV_LINEAR_DEFAULT_BLOCK_SIZE);

    lv_linear_allocator_private * mem = lv_zalloc(sizeof(lv_linear_allocator_private));
    LV_ASSERT_MALLOC(mem);
    if(mem == NULL) {
        LV_LOG_ERROR("allocate memory failed for linear allocator");
        return NULL;
    }

    mem->base.alloc = _linear_alloc;
    mem->base.total_memory = 0;
    mem->align = align;
    mem->init_block_size = MEM_ALIGN(block_size, LV_MEM_DEFAULT_ALIGN_4);

    return (lv_linear_allocator *)mem;
}

void lv_linear_allocator_reset(lv_linear_allocator * mem)
{
    LV_ASSERT_NULL(mem);

    lv_linear_allocator_private * p = (lv_linear_allocator_private *)mem;
    if(p->blocks) {
        p->cur_block = p->blocks;
        p->next_ptr = (void *)(p->blocks->data);
        p->next_ptr = (void *)MEM_ALIGN((uintptr_t)p->next_ptr, (uintptr_t)p->align);
    }
}

void lv_linear_allocator_delete(lv_linear_allocator * mem)
{
    LV_ASSERT_NULL(mem);

    lv_mem_block_t * b = ((lv_linear_allocator_private *)mem)->blocks;

    while(b) {
        lv_mem_block_t * next = b->next;
        lv_free(b);
        b = next;
    }
    lv_free(mem);
}
