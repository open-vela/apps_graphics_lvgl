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

/* Test rectangle intersection */
void test_area_intersect(void)
{
    /* Normal intersection */
    lv_area_t a1 = {10, 20, 100, 200};
    lv_area_t a2 = {50, 50, 150, 250};
    lv_area_t res;
    bool intersect = _lv_area_intersect(&res, &a1, &a2);
    TEST_ASSERT_TRUE(intersect);
    TEST_ASSERT_EQUAL_INT32(50, res.x1);
    TEST_ASSERT_EQUAL_INT32(50, res.y1);
    TEST_ASSERT_EQUAL_INT32(100, res.x2);
    TEST_ASSERT_EQUAL_INT32(200, res.y2);

    /* No intersection */
    lv_area_t a3 = {200, 300, 300, 400};
    intersect = _lv_area_intersect(&res, &a1, &a3);
    TEST_ASSERT_FALSE(intersect);
}

/* Test rectangle union */
void test_area_join(void)
{
    lv_area_t a1 = {10, 20, 100, 200};
    lv_area_t a2 = {50, 50, 150, 250};
    lv_area_t res;

    _lv_area_join(&res, &a1, &a2);
    TEST_ASSERT_EQUAL_INT32(10, res.x1);
    TEST_ASSERT_EQUAL_INT32(20, res.y1);
    TEST_ASSERT_EQUAL_INT32(150, res.x2);
    TEST_ASSERT_EQUAL_INT32(250, res.y2);

    /* One area inside another */
    lv_area_t a3 = {20, 30, 80, 180};
    _lv_area_join(&res, &a1, &a3);
    TEST_ASSERT_EQUAL_INT32(10, res.x1);
    TEST_ASSERT_EQUAL_INT32(20, res.y1);
    TEST_ASSERT_EQUAL_INT32(100, res.x2);
    TEST_ASSERT_EQUAL_INT32(200, res.y2);
}

/* Test rectangle difference */
void test_area_diff(void)
{
    lv_area_t a1 = {10, 20, 100, 200};
    lv_area_t a2 = {50, 50, 80, 180};
    lv_area_t res[4];

    int8_t res_c = _lv_area_diff(res, &a1, &a2);
    TEST_ASSERT_EQUAL_INT8(4, res_c);

    /* Top rectangle */
    TEST_ASSERT_EQUAL_INT32(10, res[0].x1);
    TEST_ASSERT_EQUAL_INT32(20, res[0].y1);
    TEST_ASSERT_EQUAL_INT32(100, res[0].x2);
    TEST_ASSERT_EQUAL_INT32(50, res[0].y2);

    /* Bottom rectangle */
    TEST_ASSERT_EQUAL_INT32(10, res[1].x1);
    TEST_ASSERT_EQUAL_INT32(180, res[1].y1);
    TEST_ASSERT_EQUAL_INT32(100, res[1].x2);
    TEST_ASSERT_EQUAL_INT32(200, res[1].y2);

    /* Left rectangle */
    TEST_ASSERT_EQUAL_INT32(10, res[2].x1);
    TEST_ASSERT_EQUAL_INT32(50, res[2].y1);
    TEST_ASSERT_EQUAL_INT32(50, res[2].x2);
    TEST_ASSERT_EQUAL_INT32(180, res[2].y2);

    /* Right rectangle */
    TEST_ASSERT_EQUAL_INT32(80, res[3].x1);
    TEST_ASSERT_EQUAL_INT32(50, res[3].y1);
    TEST_ASSERT_EQUAL_INT32(100, res[3].x2);
    TEST_ASSERT_EQUAL_INT32(180, res[3].y2);
}

/* Test point on rectangle */
void test_area_is_point_on(void)
{
    lv_area_t a = {10, 20, 100, 200};
    lv_point_t p1 = {50, 50};
    TEST_ASSERT_TRUE(_lv_area_is_point_on(&a, &p1, 0));

    lv_point_t p2 = {5, 5};
    TEST_ASSERT_FALSE(_lv_area_is_point_on(&a, &p2, 0));

    /* Test with radius */
    lv_point_t p3 = {15, 25};
    TEST_ASSERT_TRUE(_lv_area_is_point_on(&a, &p3, 4));

    /* Test point on edge without radius */
    lv_point_t p4 = {10, 100};  /* Left edge */
    TEST_ASSERT_TRUE(_lv_area_is_point_on(&a, &p4, 0));

    lv_point_t p5 = {100, 20};  /* Top edge */
    TEST_ASSERT_TRUE(_lv_area_is_point_on(&a, &p5, 0));

    /* Test point on corner */
    lv_point_t p6 = {100, 200};  /* Bottom-right corner */
    TEST_ASSERT_TRUE(_lv_area_is_point_on(&a, &p6, 0));

}

/* Test rectangle relations */
void test_area_relations(void)
{
    lv_area_t a1 = {10, 20, 100, 200};
    lv_area_t a2 = {50, 50, 80, 180};
    lv_area_t a3 = {200, 300, 300, 400};

    /* Test is_on (overlap) */
    TEST_ASSERT_TRUE(_lv_area_is_on(&a1, &a2));
    TEST_ASSERT_FALSE(_lv_area_is_on(&a1, &a3));

    /* Test is_in (contain) */
    TEST_ASSERT_TRUE(_lv_area_is_in(&a2, &a1, 0));
    TEST_ASSERT_FALSE(_lv_area_is_in(&a1, &a2, 0));

    /* Test is_out */
    TEST_ASSERT_TRUE(_lv_area_is_out(&a3, &a1, 0));
    TEST_ASSERT_FALSE(_lv_area_is_out(&a2, &a1, 0));

    /* Test is_equal */
    TEST_ASSERT_TRUE(_lv_area_is_equal(&a1, &a1));
    TEST_ASSERT_FALSE(_lv_area_is_equal(&a1, &a2));
}

/* Test area alignment */
void test_area_align(void)
{
    lv_area_t base = {0, 0, 200, 200};
    lv_area_t to_align = {0, 0, 50, 50};

    /* Test center alignment */
    lv_area_align(&base, &to_align, LV_ALIGN_CENTER, 0, 0);
    TEST_ASSERT_EQUAL_INT32(75, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(75, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(125, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(125, to_align.y2);

    /* Test top left alignment with offset */
    lv_area_align(&base, &to_align, LV_ALIGN_TOP_LEFT, 10, 20);
    TEST_ASSERT_EQUAL_INT32(10, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(20, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(60, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(70, to_align.y2);

    /* Test top mid alignment */
    lv_area_align(&base, &to_align, LV_ALIGN_TOP_MID, 0, 0);
    TEST_ASSERT_EQUAL_INT32(75, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(0, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(125, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(50, to_align.y2);

    /* Test top right alignment with offset */
    lv_area_align(&base, &to_align, LV_ALIGN_TOP_RIGHT, -10, 5);
    TEST_ASSERT_EQUAL_INT32(140, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(5, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(190, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(55, to_align.y2);

    /* Test left mid alignment */
    lv_area_align(&base, &to_align, LV_ALIGN_LEFT_MID, 0, 0);
    TEST_ASSERT_EQUAL_INT32(0, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(75, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(50, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(125, to_align.y2);

    /* Test right mid alignment with offset */
    lv_area_align(&base, &to_align, LV_ALIGN_RIGHT_MID, 5, -5);
    TEST_ASSERT_EQUAL_INT32(155, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(70, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(205, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(120, to_align.y2);

    /* Test bottom left alignment */
    lv_area_align(&base, &to_align, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    TEST_ASSERT_EQUAL_INT32(0, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(150, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(50, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(200, to_align.y2);

    /* Test bottom mid alignment with offset */
    lv_area_align(&base, &to_align, LV_ALIGN_BOTTOM_MID, 0, -10);
    TEST_ASSERT_EQUAL_INT32(75, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(140, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(125, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(190, to_align.y2);

    /* Test bottom right alignment */
    lv_area_align(&base, &to_align, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    TEST_ASSERT_EQUAL_INT32(150, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(150, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(200, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(200, to_align.y2);

    /* Test outer alignments */
    lv_area_align(&base, &to_align, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
    TEST_ASSERT_EQUAL_INT32(0, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(-51, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(50, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(-1, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_TOP_MID, 0, 0);
    TEST_ASSERT_EQUAL_INT32(75, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(-51, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(125, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(-1, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_TOP_RIGHT, 0, 0);
    TEST_ASSERT_EQUAL_INT32(150, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(-51, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(200, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(-1, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    TEST_ASSERT_EQUAL_INT32(0, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(201, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(50, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(251, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    TEST_ASSERT_EQUAL_INT32(75, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(201, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(125, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(251, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
    TEST_ASSERT_EQUAL_INT32(150, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(201, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(200, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(251, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_LEFT_TOP, 0, 0);
    TEST_ASSERT_EQUAL_INT32(-51, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(0, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(-1, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(50, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_LEFT_MID, 0, 0);
    TEST_ASSERT_EQUAL_INT32(-51, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(75, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(-1, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(125, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);
    TEST_ASSERT_EQUAL_INT32(-51, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(150, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(-1, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(200, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    TEST_ASSERT_EQUAL_INT32(201, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(0, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(251, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(50, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    TEST_ASSERT_EQUAL_INT32(201, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(75, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(251, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(125, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);
    TEST_ASSERT_EQUAL_INT32(201, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(150, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(251, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(200, to_align.y2);

    /* Test with offsets */
    lv_area_align(&base, &to_align, LV_ALIGN_OUT_TOP_MID, 10, 20);
    TEST_ASSERT_EQUAL_INT32(85, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(-31, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(135, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(19, to_align.y2);

    lv_area_align(&base, &to_align, LV_ALIGN_OUT_LEFT_MID, -5, 10);
    TEST_ASSERT_EQUAL_INT32(-56, to_align.x1);
    TEST_ASSERT_EQUAL_INT32(85, to_align.y1);
    TEST_ASSERT_EQUAL_INT32(-6, to_align.x2);
    TEST_ASSERT_EQUAL_INT32(135, to_align.y2);

    /* Test default case with invalid alignment */
    lv_area_align(&base, &to_align, (lv_align_t)100, 10, 20);
    TEST_ASSERT_EQUAL_INT32(10, to_align.x1);  /* base.x1 + ofs_x = 0 + 10 */
    TEST_ASSERT_EQUAL_INT32(20, to_align.y1);  /* base.y1 + ofs_y = 0 + 20 */
    TEST_ASSERT_EQUAL_INT32(60, to_align.x2);  /* x1 + width - 1 = 10 + 51 - 1 */
    TEST_ASSERT_EQUAL_INT32(70, to_align.y2);  /* y1 + height - 1 = 20 + 51 - 1 */
}

void test_area_set_pos(void)
{
    lv_area_t area = {10, 20, 100, 200};  /* Original area */
    int32_t original_width = lv_area_get_width(&area);
    int32_t original_height = lv_area_get_height(&area);

    /* Test basic position setting */
    _lv_area_set_pos(&area, 50, 60);
    TEST_ASSERT_EQUAL_INT32(50, area.x1);
    TEST_ASSERT_EQUAL_INT32(60, area.y1);
    TEST_ASSERT_EQUAL_INT32(50 + original_width - 1, area.x2);
    TEST_ASSERT_EQUAL_INT32(60 + original_height - 1, area.y2);

    /* Test setting to minimum coordinates */
    _lv_area_set_pos(&area, 0, 0);
    TEST_ASSERT_EQUAL_INT32(0, area.x1);
    TEST_ASSERT_EQUAL_INT32(0, area.y1);
    TEST_ASSERT_EQUAL_INT32(original_width - 1, area.x2);
    TEST_ASSERT_EQUAL_INT32(original_height - 1, area.y2);

    /* Test setting to negative coordinates */
    _lv_area_set_pos(&area, -10, -20);
    TEST_ASSERT_EQUAL_INT32(-10, area.x1);
    TEST_ASSERT_EQUAL_INT32(-20, area.y1);
    TEST_ASSERT_EQUAL_INT32(-10 + original_width - 1, area.x2);
    TEST_ASSERT_EQUAL_INT32(-20 + original_height - 1, area.y2);

    /* Verify width and height remain unchanged */
    TEST_ASSERT_EQUAL_INT32(original_width, lv_area_get_width(&area));
    TEST_ASSERT_EQUAL_INT32(original_height, lv_area_get_height(&area));
}

#endif
