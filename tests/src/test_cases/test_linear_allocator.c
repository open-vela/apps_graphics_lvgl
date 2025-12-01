#if LV_BUILD_TEST
#include "../lvgl.h"
#include "unity/unity.h"

#define ITERATIONS 10
#define MAX_BLOCK_SIZE 65536  /* 64KB */

void setUp(void)
{
    /* Function run before every test */
}

void tearDown(void)
{
    /* Function run after every test */
}

void test_linear_allocator_create_delete(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    TEST_ASSERT_NOT_NULL(allocator);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_basic_alloc(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    uint8_t * ptr1 = allocator->alloc(allocator, 100);
    uint8_t * ptr2 = allocator->alloc(allocator, 100);
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_alignment(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_16, 256);
    uint8_t * ptr = allocator->alloc(allocator, 1);
    TEST_ASSERT_EQUAL_UINT32(0, (uintptr_t)ptr % 16);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_block_expansion(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    uint8_t * ptr1 = allocator->alloc(allocator, 200);
    uint8_t * ptr2 = allocator->alloc(allocator, 200); /* Should trigger block expansion */
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_reset(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);

    uint8_t * ptr1 = allocator->alloc(allocator, 100);
    uint8_t * ptr2 = allocator->alloc(allocator, 100);
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);

    lv_linear_allocator_reset(allocator);

    /* Allocate after reset */
    uint8_t * ptr3 = allocator->alloc(allocator, 100);
    TEST_ASSERT_NOT_NULL(ptr3);
    TEST_ASSERT_EQUAL_PTR(ptr1, ptr3);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_edge_cases(void)
{
    /* Test with minimum block size */
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 64);
    TEST_ASSERT_NOT_NULL(allocator);

    uint8_t * ptr1 = allocator->alloc(allocator, 1);
    TEST_ASSERT_NOT_NULL(ptr1);

    /* Test allocation of size 0 */
    uint8_t * ptr2 = allocator->alloc(allocator, 0);
    TEST_ASSERT_NOT_NULL(ptr2);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_different_alignments(void)
{
    /* Test all alignment types */
    lv_mem_align_type_t alignments[] = {
        LV_MEM_ALIGN_1, LV_MEM_ALIGN_4, LV_MEM_ALIGN_8, LV_MEM_ALIGN_16, LV_MEM_ALIGN_32, LV_MEM_ALIGN_64
    };

    for(size_t i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
        lv_linear_allocator * allocator = lv_linear_allocator_create(alignments[i], 256);
        TEST_ASSERT_NOT_NULL(allocator);

        uint8_t * ptr = allocator->alloc(allocator, 50);
        TEST_ASSERT_NOT_NULL(ptr);
        TEST_ASSERT_EQUAL_UINT32(0, (uintptr_t)ptr % alignments[i]);

        lv_linear_allocator_delete(allocator);
    }
}

void test_linear_allocator_after_reset(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);

    uint8_t * ptrs[5];
    for(int i = 0; i < 5; i++) {
        ptrs[i] = allocator->alloc(allocator, 50);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    lv_linear_allocator_reset(allocator);

    uint8_t * new_ptrs[5];
    for(int i = 0; i < 5; i++) {
        new_ptrs[i] = allocator->alloc(allocator, 50);
        TEST_ASSERT_NOT_NULL(new_ptrs[i]);
    }

    /* After reset, allocations start from beginning */
    TEST_ASSERT_EQUAL_PTR(ptrs[0], new_ptrs[0]);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_max_size(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 1024);

    /* Test allocation near maximum block size */
    uint8_t * ptr1 = allocator->alloc(allocator, MAX_BLOCK_SIZE - 100);
    TEST_ASSERT_NOT_NULL(ptr1);

    /* Test allocation at boundary values */
    uint8_t * ptr2 = allocator->alloc(allocator, MAX_BLOCK_SIZE - 1);
    TEST_ASSERT_NOT_NULL(ptr2);

    uint8_t * ptr3 = allocator->alloc(allocator, MAX_BLOCK_SIZE);
    TEST_ASSERT_NOT_NULL(ptr3);

    /* Test allocation exceeding maximum block size */
    uint8_t * ptr4 = allocator->alloc(allocator, MAX_BLOCK_SIZE + 1);
    TEST_ASSERT_NULL(ptr4);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_performance_single_alloc(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 1024);

    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        uint8_t * ptr = allocator->alloc(allocator, 16);
        TEST_ASSERT_NOT_NULL(ptr);
    }
    double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
    LV_LOG_USER("Single allocation (16 bytes) %d times,total time: %.6f s", ITERATIONS, duration);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_performance_throughput(void)
{
    const int block_size = 16;
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 1024);

    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        uint8_t * ptr = allocator->alloc(allocator, block_size);
        TEST_ASSERT_NOT_NULL(ptr);
    }
    double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
    LV_LOG_USER("Throughput: %.6f allocations/s", (float)ITERATIONS / duration);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_performance_various_sizes(void)
{
    const int sizes[] = {16, 64, 256, 1024, 4096};
    const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 8192);

    for(int s = 0; s < num_sizes; s++) {
        clock_t start = clock();
        for(int i = 0; i < ITERATIONS; i++) {
            uint8_t * ptr = allocator->alloc(allocator, sizes[s]);
            TEST_ASSERT_NOT_NULL(ptr);
        }
        double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
        LV_LOG_USER("Allocation size %4d bytes %d times,total time: %.6f s",
                    sizes[s], ITERATIONS, duration);
    }

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_memory_integrity(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    TEST_ASSERT_NOT_NULL(allocator);

    /* Allocate 3 blocks with different sizes */
    uint8_t * block1 = allocator->alloc(allocator, 64);
    uint8_t * block2 = allocator->alloc(allocator, 128);
    uint8_t * block3 = allocator->alloc(allocator, 32);
    TEST_ASSERT_NOT_NULL(block1);
    TEST_ASSERT_NOT_NULL(block2);
    TEST_ASSERT_NOT_NULL(block3);

    /* Fill each block with unique pattern */
    lv_memset(block1, 0xAA, 64);
    lv_memset(block2, 0xBB, 128);
    lv_memset(block3, 0xCC, 32);

    /* Verify patterns are intact */
    for(int i = 0; i < 64; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xAA, (block1)[i]);
    }
    for(int i = 0; i < 128; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xBB, (block2)[i]);
    }
    for(int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xCC, (block3)[i]);
    }

    /* Reset and test again */
    lv_linear_allocator_reset(allocator);

    /* Reallocate and verify */
    uint8_t * new_block1 = allocator->alloc(allocator, 64);
    uint8_t * new_block2 = allocator->alloc(allocator, 128);
    uint8_t * new_block3 = allocator->alloc(allocator, 32);
    TEST_ASSERT_NOT_NULL(new_block1);
    TEST_ASSERT_NOT_NULL(new_block2);
    TEST_ASSERT_NOT_NULL(new_block3);

    lv_memset(new_block1, 0xDD, 64);
    lv_memset(new_block2, 0xEE, 128);
    lv_memset(new_block3, 0xFF, 32);

    for(int i = 0; i < 64; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xDD, (new_block1)[i]);
    }
    for(int i = 0; i < 128; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xEE, (new_block2)[i]);
    }
    for(int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, (new_block3)[i]);
    }

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_drops_empty_block_after_reset(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    TEST_ASSERT_NOT_NULL(allocator);

    /* Allocate enough to build three blocks of increasing size */
    uint8_t * block1_use = allocator->alloc(allocator, 16);
    TEST_ASSERT_NOT_NULL(block1_use);
    size_t after_block1 = allocator->total_memory;

    uint8_t * block2_use = allocator->alloc(allocator, 400);
    TEST_ASSERT_NOT_NULL(block2_use);
    size_t after_block2 = allocator->total_memory;
    TEST_ASSERT_TRUE(after_block2 > after_block1);

    uint8_t * block3_use = allocator->alloc(allocator, 900);
    TEST_ASSERT_NOT_NULL(block3_use);
    size_t after_block3 = allocator->total_memory;
    TEST_ASSERT_TRUE(after_block3 > after_block2);

    size_t block2_size = after_block2 - after_block1;

    /* Reset so every block appears unused from the allocator's perspective */
    lv_linear_allocator_reset(allocator);
    TEST_ASSERT_EQUAL_size_t(after_block3, allocator->total_memory);

    /* Touch block1 so it is no longer eligible for dropping */
    TEST_ASSERT_NOT_NULL(allocator->alloc(allocator, 32));

    /* Request a chunk that cannot fit in the next (still unused) block2 */
    uint8_t * drop_target = allocator->alloc(allocator, 900);
    TEST_ASSERT_NOT_NULL(drop_target);

    /* The allocator should have reclaimed the empty block2 before moving on */
    TEST_ASSERT_EQUAL_size_t(after_block3 - block2_size, allocator->total_memory);

    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_drops_head_block_after_reset(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    TEST_ASSERT_NOT_NULL(allocator);

    /* Build at least two blocks so the first one can be dropped */
    TEST_ASSERT_NOT_NULL(allocator->alloc(allocator, 16));
    size_t after_block1 = allocator->total_memory;

    TEST_ASSERT_NOT_NULL(allocator->alloc(allocator, 400));
    size_t after_block2 = allocator->total_memory;
    TEST_ASSERT_TRUE(after_block2 > after_block1);

    lv_linear_allocator_reset(allocator);

    TEST_ASSERT_NOT_NULL(allocator->alloc(allocator, 32));

    TEST_ASSERT_EQUAL_size_t(after_block2, allocator->total_memory);

    lv_linear_allocator_delete(allocator);
}

#endif /* LV_BUILD_TEST */
