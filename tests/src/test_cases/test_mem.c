#if LV_BUILD_TEST
#include "../lvgl.h"
#include "unity/unity.h"

void setUp(void)
{
    /* Function run before every test */
}

void tearDown(void)
{
    /* Function run after every test */
}

/* #3324 */
void test_mem_buf_realloc(void)
{
#ifdef LVGL_CI_USING_DEF_HEAP
    void * buf1 = lv_malloc(20);
    void * buf2 = lv_realloc(buf1, LV_MEM_SIZE + 16384);
    TEST_ASSERT_NULL(buf2);
#endif
}

#define ITERATIONS 10000

void test_mem_performance_single_alloc(void)
{
    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        void * ptr = lv_malloc(16);
        TEST_ASSERT_NOT_NULL(ptr);
        lv_free(ptr);
    }
    double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
    LV_LOG_USER("Single allocation (16 bytes) %d times,total time: %.6f s", ITERATIONS, duration);
}

void test_mem_performance_throughput(void)
{
    const int block_size = 16;
    clock_t start = clock();
    for(int i = 0; i < ITERATIONS; i++) {
        void * ptr = lv_malloc(block_size);
        TEST_ASSERT_NOT_NULL(ptr);
        lv_free(ptr);
    }
    double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
    float throughput = (float)ITERATIONS / duration;
    LV_LOG_USER("Throughput: %.2f allocations/s", throughput);
}

void test_mem_performance_various_sizes(void)
{
    const int sizes[] = {16, 64, 256, 1024, 4096};
    const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for(int s = 0; s < num_sizes; s++) {
        clock_t start = clock();
        for(int i = 0; i < ITERATIONS; i++) {
            void * ptr = lv_malloc(sizes[s]);
            TEST_ASSERT_NOT_NULL(ptr);
            lv_free(ptr);
        }
        double duration = (double)(clock() - start) / CLOCKS_PER_SEC;
        LV_LOG_USER("Allocation size %4d bytes %d times,total time: %.6f s",
                    sizes[s], ITERATIONS, duration);
    }
}

#endif
