#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"
#include "lv_test_indev.h"

/* This function runs before each test */
void setUp(void);

void test_arc_creation_successful(void);
void test_arc_should_truncate_to_max_range_when_new_value_exceeds_it(void);
void test_arc_should_truncate_to_min_range_when_new_value_is_inferior(void);
void test_arc_should_update_value_after_updating_range(void);
void test_arc_should_update_angles_when_changing_to_symmetrical_mode(void);
void test_arc_should_update_angles_when_changing_to_symmetrical_mode_value_more_than_middle_range(void);
void test_arc_angles_when_reversed(void);

static lv_obj_t * test_arc_active_screen = NULL;
static lv_obj_t * _test_arc = NULL;
static uint32_t test_arc_event_cnt;

static void dummy_event_cb(lv_event_t * e);

void setUp(void)
{
    test_arc_active_screen = lv_screen_active();
}

void tearDown(void)
{
    lv_obj_clean(lv_screen_active());
}

void test_arc_creation_successful(void)
{
    _test_arc = lv_arc_create(test_arc_active_screen);

    TEST_ASSERT_NOT_NULL(_test_arc);
}

void test_arc_should_truncate_to_max_range_when_new_value_exceeds_it(void)
{
    /* Default max range is 100 */
    int16_t value_after_truncation = 100;

    _test_arc = lv_arc_create(test_arc_active_screen);

    lv_arc_set_value(_test_arc, 200);

    TEST_ASSERT_EQUAL_INT16(value_after_truncation, lv_arc_get_value(_test_arc));
}

void test_arc_should_truncate_to_min_range_when_new_value_is_inferior(void)
{
    /* Default min range is 100 */
    int16_t value_after_truncation = 0;

    _test_arc = lv_arc_create(test_arc_active_screen);

    lv_arc_set_value(_test_arc, 0);

    TEST_ASSERT_EQUAL_INT16(value_after_truncation, lv_arc_get_value(_test_arc));
}

void test_arc_should_update_value_after_updating_range(void)
{
    int16_t value_after_updating_max_range = 50;
    int16_t value_after_updating_min_range = 30;

    _test_arc = lv_arc_create(test_arc_active_screen);

    lv_arc_set_value(_test_arc, 80);
    lv_arc_set_range(_test_arc, 1, 50);

    TEST_ASSERT_EQUAL_INT16(value_after_updating_max_range, lv_arc_get_value(_test_arc));

    lv_arc_set_value(_test_arc, 10);
    lv_arc_set_range(_test_arc, 30, 50);

    TEST_ASSERT_EQUAL_INT16(value_after_updating_min_range, lv_arc_get_value(_test_arc));
}

void test_arc_should_update_angles_when_changing_to_symmetrical_mode(void)
{
    int16_t expected_angle_start = 135;
    int16_t expected_angle_end = 270;

    /* start angle is 135, end angle is 45 at creation */
    _test_arc = lv_arc_create(test_arc_active_screen);
    lv_arc_set_mode(_test_arc, LV_ARC_MODE_SYMMETRICAL);

    TEST_ASSERT_EQUAL_INT16(expected_angle_start, lv_arc_get_angle_start(_test_arc));
    TEST_ASSERT_EQUAL_INT16(expected_angle_end, lv_arc_get_angle_end(_test_arc));
}

void test_arc_should_update_angles_when_changing_to_symmetrical_mode_value_more_than_middle_range(void)
{
    int16_t expected_angle_start = 270;
    int16_t expected_angle_end = 45;

    /* start angle is 135, end angle is 45 at creation */
    _test_arc = lv_arc_create(test_arc_active_screen);
    lv_arc_set_value(_test_arc, 100);
    lv_arc_set_mode(_test_arc, LV_ARC_MODE_SYMMETRICAL);

    TEST_ASSERT_EQUAL_INT16(expected_angle_start, lv_arc_get_angle_start(_test_arc));
    TEST_ASSERT_EQUAL_INT16(expected_angle_end, lv_arc_get_angle_end(_test_arc));
}

/* See #2522 for more information */
void test_arc_angles_when_reversed(void)
{
    uint16_t expected_start_angle = 54;
    uint16_t expected_end_angle = 90;
    int16_t expected_value = 40;

    lv_obj_t * arcBlack;
    arcBlack = lv_arc_create(lv_screen_active());

    lv_arc_set_mode(arcBlack, LV_ARC_MODE_REVERSE);

    lv_arc_set_bg_angles(arcBlack, 0, 90);

    lv_arc_set_value(arcBlack, expected_value);

    TEST_ASSERT_EQUAL_UINT16(expected_start_angle, lv_arc_get_angle_start(arcBlack));
    TEST_ASSERT_EQUAL_UINT16(expected_end_angle, lv_arc_get_angle_end(arcBlack));
    TEST_ASSERT_EQUAL_INT16(expected_value, lv_arc_get_value(arcBlack));
}

void test_arc_click_area_with_adv_hittest(void)
{
    _test_arc = lv_arc_create(lv_screen_active());
    lv_obj_set_size(_test_arc, 100, 100);
    lv_obj_set_style_arc_width(_test_arc, 10, 0);
    lv_obj_add_flag(_test_arc, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(_test_arc, dummy_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_set_ext_click_area(_test_arc, 5);

    /*No click detected at the middle*/
    test_arc_event_cnt = 0;
    lv_test_mouse_click_at(50, 50);
    TEST_ASSERT_EQUAL_UINT32(0, test_arc_event_cnt);

    /*No click close to the radius - bg_arc - ext_click_area*/
    test_arc_event_cnt = 0;
    lv_test_mouse_click_at(83, 50);
    TEST_ASSERT_EQUAL_UINT32(0, test_arc_event_cnt);

    /*Click on the radius - bg_arc - ext_click_area*/
    test_arc_event_cnt = 0;
    lv_test_mouse_click_at(86, 50);
    TEST_ASSERT_GREATER_THAN(0, test_arc_event_cnt);

    /*Click on the radius + ext_click_area*/
    test_arc_event_cnt = 0;
    lv_test_mouse_click_at(104, 50);
    TEST_ASSERT_GREATER_THAN(0, test_arc_event_cnt);

    /*No click beyond to the radius + ext_click_area*/
    test_arc_event_cnt = 0;
    lv_test_mouse_click_at(106, 50);
    TEST_ASSERT_EQUAL_UINT32(0, test_arc_event_cnt);
}

/* Check value doesn't go to max when clicking on the other side of the _test_arc */
void test_arc_click_sustained_from_start_to_end_does_not_set_value_to_max(void)
{
    _test_arc = lv_arc_create(lv_screen_active());
    lv_arc_set_value(_test_arc, 0);

    lv_obj_set_size(_test_arc, 100, 100);
    lv_obj_center(_test_arc);
    lv_obj_add_event_cb(_test_arc, dummy_event_cb, LV_EVENT_PRESSED, NULL);
    test_arc_event_cnt = 0;

    /* Click close to start angle */
    test_arc_event_cnt = 0;
    lv_test_mouse_move_to(376, 285);
    lv_test_mouse_press();
    lv_test_indev_wait(500);
    lv_test_mouse_release();
    lv_test_indev_wait(50);

    TEST_ASSERT_EQUAL_UINT32(1, test_arc_event_cnt);
    TEST_ASSERT_EQUAL_INT32(lv_arc_get_min_value(_test_arc), lv_arc_get_value(_test_arc));

    /* Click close to end angle */
    test_arc_event_cnt = 0;

    lv_test_mouse_move_to(376, 285);
    lv_test_mouse_press();
    lv_test_indev_wait(500);
    lv_test_mouse_move_to(415, 281);
    lv_test_indev_wait(500);
    lv_test_mouse_release();
    lv_test_indev_wait(50);

    TEST_ASSERT_EQUAL_UINT32(1, test_arc_event_cnt);
    TEST_ASSERT_EQUAL_INT32(lv_arc_get_min_value(_test_arc), lv_arc_get_value(_test_arc));

    TEST_ASSERT_EQUAL_SCREENSHOT("widgets/arc_2.png");
}

void test_arc_basic_render(void)
{
    _test_arc = lv_arc_create(lv_screen_active());
    lv_obj_set_size(_test_arc, 100, 100);
    lv_obj_center(_test_arc);
    TEST_ASSERT_EQUAL_SCREENSHOT("widgets/arc_1.png");
}

static void dummy_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    test_arc_event_cnt++;
}

#endif
