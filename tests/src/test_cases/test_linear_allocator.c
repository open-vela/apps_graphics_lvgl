#if LV_BUILD_TEST
#include "../lvgl.h"
#include "unity/unity.h"

#define ITERATIONS 10

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
    void * ptr1 = allocator->alloc(allocator, 100);
    void * ptr2 = allocator->alloc(allocator, 100);
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_alignment(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_16, 256);
    void * ptr = allocator->alloc(allocator, 1);
    TEST_ASSERT_EQUAL(0, (uintptr_t)ptr % 16);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_block_expansion(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 256);
    void * ptr1 = allocator->alloc(allocator, 200);
    void * ptr2 = allocator->alloc(allocator, 200); /* Should trigger block expansion */
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    lv_linear_allocator_delete(allocator);
}

void test_linear_allocator_performance_single_alloc(void)
{
    lv_linear_allocator * allocator = lv_linear_allocator_create(LV_MEM_ALIGN_4, 1024);

    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        void * ptr = allocator->alloc(allocator, 16);
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
        void * ptr = allocator->alloc(allocator, block_size);
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
            void * ptr = allocator->alloc(allocator, sizes[s]);
            TEST_ASSERT_NOT_NULL(ptr);
        }
        double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
        LV_LOG_USER("Allocation size %4d bytes %d times,total time: %.6f s",
                    sizes[s], ITERATIONS, duration);
    }

    lv_linear_allocator_delete(allocator);
}

#endif /* LV_BUILD_TEST */
