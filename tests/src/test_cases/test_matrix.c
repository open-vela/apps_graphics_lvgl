#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"

#include "sys/time.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static inline suseconds_t get_time(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    suseconds_t t1 = (suseconds_t)(t.tv_sec * 1000  + t.tv_usec / 1000);
    return t1;
}

static void _multiply_matrix(lv_matrix_t * matrix, const lv_matrix_t * mul)
{
    lv_matrix_t tmp;

    for(int y = 0; y < 3; y++) {
        for(int x = 0; x < 3; x++) {
            tmp.m[y][x] = (matrix->m[y][0] * mul->m[0][x])
                          + (matrix->m[y][1] * mul->m[1][x])
                          + (matrix->m[y][2] * mul->m[2][x]);
        }
    }

    lv_memcpy(matrix, &tmp, sizeof(lv_matrix_t));
}

void test_matrix_identity(void)
{
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    bool b = lv_matrix_is_identity_or_translation(&matrix);
    TEST_ASSERT_TRUE(b);
}

void test_matrix_translate(void)
{
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, 100, 100);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, matrix.m[0][2]);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, matrix.m[1][2]);
}

void test_matrix_rotate(void)
{
    lv_fpoint_t p1 = {.x = 100.0f, .y = 0.0f};
    lv_fpoint_t p2 = p1;

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_rotate(&matrix, -90.0f);

    lv_matrix_transform_point(&matrix, &p2);

    TEST_ASSERT_EQUAL_INT((int32_t)p1.x, (int32_t)p2.y);
    TEST_ASSERT_EQUAL_INT((int32_t)p1.y, (int32_t)p2.x);
}

void test_matrix_inverse(void)
{
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, 100, 100);

    lv_matrix_t matrix2;
    lv_matrix_inverse(&matrix2, &matrix);
    lv_matrix_inverse(&matrix2, &matrix2);

    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[0][0], matrix.m[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[0][1], matrix.m[0][1]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[0][2], matrix.m[0][2]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[1][0], matrix.m[1][0]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[1][1], matrix.m[1][1]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[1][2], matrix.m[1][2]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[2][0], matrix.m[2][0]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[2][1], matrix.m[2][1]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[2][2], matrix.m[2][2]);
}

void test_matrix_skew(void)
{
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_skew(&matrix, 10, 10);

    lv_matrix_t matrix2 = matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_skew(&matrix2, -10, -10);

    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[0][1], matrix.m[0][1]);
    TEST_ASSERT_EQUAL_FLOAT(matrix2.m[1][0], matrix.m[1][0]);
}

void test_matrix_scale(void)
{
    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_scale(&matrix, 0.5f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, matrix.m[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, matrix.m[1][1]);
}

static float new_matrix_multiply(int count)
{
    suseconds_t t1, t2;

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, 100, 100);

    lv_matrix_t matrix2;
    lv_matrix_identity(&matrix2);
    lv_matrix_scale(&matrix2, 0.95f, 0.95f);

    t1 = get_time();
    for(int i = 0; i < count; i++) {
        lv_matrix_multiply(&matrix, &matrix2);
    }
    t2 = get_time();

    return (float)(t2 - t1);
}

static float old_matrix_multiply(int count)
{
    suseconds_t t1, t2;

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);
    lv_matrix_translate(&matrix, 100, 100);

    lv_matrix_t matrix2;
    lv_matrix_identity(&matrix2);
    lv_matrix_scale(&matrix2, 0.95f, 0.95f);

    t1 = get_time();
    for(int i = 0; i < count; i++) {
        _multiply_matrix(&matrix, &matrix2);
    }
    t2 = get_time();

    return (float)(t2 - t1);
}

void test_matrix_performance(void)
{
    float t1 = new_matrix_multiply(10000);
    float t2 = old_matrix_multiply(10000);
    printf("new: %.2f ms  - old: %.2f ms\n", t1, t2);

    TEST_ASSERT_LESS_OR_EQUAL_INT32((int32_t)t2, (int32_t)t1);
}

#endif
