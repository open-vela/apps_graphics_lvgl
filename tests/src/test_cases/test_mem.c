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

void test_malloc(void)
{
    uint32_t mem = lv_test_get_free_mem();
    void * buf = lv_malloc(32);
    TEST_ASSERT_NOT_NULL(buf);
    lv_free(buf);
    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
}

static void check_zero_mem(const void * data, size_t size)
{
    const uint8_t * p = data;
    for(size_t i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, p[i]);
    }
}

void test_calloc(void)
{
    uint32_t mem = lv_test_get_free_mem();
    void * buf = lv_calloc(4, 8);
    TEST_ASSERT_NOT_NULL(buf);

    check_zero_mem(buf, 32);

    lv_free(buf);
    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
}

void test_zalloc(void)
{
    uint32_t mem = lv_test_get_free_mem();
    void * buf = lv_zalloc(32);
    TEST_ASSERT_NOT_NULL(buf);

    check_zero_mem(buf, 32);

    lv_free(buf);
    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
}

void test_realloc(void)
{
    uint32_t mem = lv_test_get_free_mem();
    void * buf = lv_malloc(16);
    TEST_ASSERT_NOT_NULL(buf);

    buf = lv_realloc(buf, 32);
    TEST_ASSERT_NOT_NULL(buf);

    buf = lv_realloc(buf, 8);
    TEST_ASSERT_NOT_NULL(buf);

    lv_free(buf);

    /* Should behave like malloc */
    buf = lv_realloc(NULL, 16);
    TEST_ASSERT_NOT_NULL(buf);
    lv_free(buf);

    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
}

/* #3324 */
void test_realloc_failed(void)
{
#ifdef LVGL_CI_USING_DEF_HEAP
    uint32_t mem = lv_test_get_free_mem();

    void * buf1 = lv_malloc(20);

    void * buf2 = lv_realloc(buf1, LV_MEM_SIZE + 1);
    TEST_ASSERT_NULL(buf2);

    /* Realloc failed, but should free buf1 */
    void * buf3 = lv_reallocf(buf1, LV_MEM_SIZE + 1);
    TEST_ASSERT_NULL(buf3);

    void * buf4 = lv_reallocf(NULL, 30);
    TEST_ASSERT_NOT_NULL(buf4);
    lv_free(buf4);

    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
#endif
}

void test_malloc_failed(void)
{
#ifdef LVGL_CI_USING_DEF_HEAP
    uint32_t mem = lv_test_get_free_mem();
    TEST_ASSERT_NULL(lv_malloc(LV_MEM_SIZE + 1));
    TEST_ASSERT_NULL(lv_malloc_zeroed(LV_MEM_SIZE + 1));
    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
#endif
}

void test_mem_test(void)
{
#ifdef LVGL_CI_USING_DEF_HEAP
    uint32_t mem = lv_test_get_free_mem();
    uint32_t * zero_mem = lv_malloc_zeroed(0);
    TEST_ASSERT_NOT_NULL(zero_mem);

    /* Test magic value */
    TEST_ASSERT_EQUAL_UINT32(ZERO_MEM_SENTINEL, *zero_mem);
    TEST_ASSERT_EQUAL(LV_RESULT_OK, lv_mem_test());

    /* Test wrong memory, test should fail */
    *zero_mem = 0;
    TEST_ASSERT_EQUAL(LV_RESULT_INVALID, lv_mem_test());

    /* Restore magic value */
    *zero_mem = ZERO_MEM_SENTINEL;
    lv_free(zero_mem);

    /* Re-verify zero memory */
    uint32_t * new_zero_mem = lv_malloc_zeroed(0);
    TEST_ASSERT_EQUAL_UINT32(ZERO_MEM_SENTINEL, *new_zero_mem);
    TEST_ASSERT_EQUAL(LV_RESULT_OK, lv_mem_test());
    lv_free(new_zero_mem);

    TEST_ASSERT_MEM_LEAK_LESS_THAN(mem, 0);
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
    float duration = (float)(clock() - start) / CLOCKS_PER_SEC;
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
