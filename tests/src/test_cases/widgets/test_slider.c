#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"
#include "lv_test_indev.h"

static lv_obj_t * test_slider_active_screen = NULL;
static lv_obj_t * _test_slider = NULL;
static lv_obj_t * sliderRangeMode = NULL;
static lv_obj_t * sliderNormalMode = NULL;
static lv_obj_t * sliderSymmetricalMode = NULL;
static lv_group_t * gs = NULL;

void setUp(void)
{
    test_slider_active_screen = lv_screen_active();
    _test_slider = lv_slider_create(test_slider_active_screen);
    sliderRangeMode = lv_slider_create(test_slider_active_screen);
    sliderNormalMode = lv_slider_create(test_slider_active_screen);
    sliderSymmetricalMode = lv_slider_create(test_slider_active_screen);

    lv_slider_set_mode(sliderRangeMode, LV_SLIDER_MODE_RANGE);
    lv_slider_set_mode(sliderNormalMode, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_mode(sliderSymmetricalMode, LV_SLIDER_MODE_SYMMETRICAL);

    gs = lv_group_create();
    lv_indev_set_group(lv_test_encoder_indev, gs);
}

void tearDown(void)
{
    lv_obj_clean(test_slider_active_screen);
}

void test_slider_should_have_valid_documented_default_values(void)
{
    int32_t objw = lv_obj_get_width(_test_slider);
    int32_t objh = lv_obj_get_height(_test_slider);

    /* Horizontal _test_slider */
    TEST_ASSERT_TRUE(objw >= objh);
    TEST_ASSERT_FALSE(lv_obj_has_flag(_test_slider, LV_OBJ_FLAG_SCROLL_CHAIN));
    TEST_ASSERT_FALSE(lv_obj_has_flag(_test_slider, LV_OBJ_FLAG_SCROLLABLE));
}

void test_slider_event_keys_right_and_up_increment_value_by_one(void)
{
    uint32_t key = LV_KEY_RIGHT;
    lv_slider_set_value(_test_slider, 10, LV_ANIM_OFF);
    int32_t value = lv_slider_get_value(_test_slider);

    lv_obj_send_event(_test_slider, LV_EVENT_KEY, (void *) &key);

    int32_t new_value = lv_slider_get_value(_test_slider);
    TEST_ASSERT_EQUAL_INT32(value + 1, new_value);

    key = LV_KEY_UP;
    lv_obj_send_event(_test_slider, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_INT32(new_value + 1, lv_slider_get_value(_test_slider));
}

void test_slider_event_keys_left_and_down_decrement_value_by_one(void)
{
    uint32_t key = LV_KEY_LEFT;
    lv_slider_set_value(_test_slider, 10, LV_ANIM_OFF);
    int32_t value = lv_slider_get_value(_test_slider);

    lv_obj_send_event(_test_slider, LV_EVENT_KEY, (void *) &key);

    int32_t new_value = lv_slider_get_value(_test_slider);
    TEST_ASSERT_EQUAL_INT32(value - 1, new_value);

    key = LV_KEY_DOWN;
    lv_obj_send_event(_test_slider, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_INT32(new_value - 1, lv_slider_get_value(_test_slider));
}

void test_slider_event_invalid_key_should_not_change_values(void)
{
    uint32_t key = LV_KEY_ENTER;
    lv_slider_set_value(_test_slider, 10, LV_ANIM_OFF);
    int32_t value = lv_slider_get_value(_test_slider);

    lv_obj_send_event(_test_slider, LV_EVENT_KEY, (void *) &key);

    TEST_ASSERT_EQUAL_INT32(value, lv_slider_get_value(_test_slider));
}

void test_slider_range_mode_should_leave_edit_mode_if_released(void)
{
    lv_slider_t * ptr = (lv_slider_t *) sliderRangeMode;

    /* Setup group and encoder indev */
    lv_group_add_obj(gs, sliderNormalMode);
    lv_group_set_editing(gs, true);

    lv_test_encoder_click();

    /* Always executed when handling LV_EVENT_RELEASED or
     * LV_EVENT_PRESS_LOST */
    TEST_ASSERT_FALSE(ptr->dragging);
    TEST_ASSERT_NULL(ptr->value_to_set);
    TEST_ASSERT_EQUAL(0U, ptr->left_knob_focus);

    /* Group leaved edit mode */
    TEST_ASSERT_FALSE(lv_group_get_editing(gs));
}

void test_slider_range_mode_should_not_leave_edit_mode_if_released_with_no_left_knob_focus(void)
{
    lv_slider_t * ptr = (lv_slider_t *) sliderRangeMode;

    /* Setup group and encoder indev */
    lv_group_add_obj(gs, sliderRangeMode);
    lv_group_set_editing(gs, true);

    lv_test_encoder_release();
    lv_test_indev_wait(50);

    /* Always executed when handling LV_EVENT_RELEASED or
     * LV_EVENT_PRESS_LOST */
    TEST_ASSERT_FALSE(ptr->dragging);
    TEST_ASSERT_NULL(ptr->value_to_set);

    TEST_ASSERT(lv_group_get_editing(gs));
}

void test_slider_normal_mode_should_leave_edit_mode_if_released(void)
{
    lv_slider_t * ptr = (lv_slider_t *) sliderNormalMode;
    ptr->left_knob_focus = 1;

    /* Setup group and encoder indev */
    lv_group_add_obj(gs, sliderNormalMode);
    lv_group_set_editing(gs, true);

    lv_test_encoder_click();

    /* Always executed when handling LV_EVENT_RELEASED or
     * LV_EVENT_PRESS_LOST */
    TEST_ASSERT_FALSE(ptr->dragging);
    TEST_ASSERT_NULL(ptr->value_to_set);
    TEST_ASSERT_EQUAL(0U, ptr->left_knob_focus);

    /* Group leaved edit mode */
    TEST_ASSERT_FALSE(lv_group_get_editing(gs));
}

void test_ranged_mode_adjust_with_encoder(void)
{
    lv_slider_set_value(sliderRangeMode, 90, LV_ANIM_OFF);
    lv_slider_set_left_value(sliderRangeMode, 10, LV_ANIM_OFF);

    /* Setup group and encoder indev */
    lv_group_add_obj(gs, sliderRangeMode);
    lv_group_set_editing(gs, false);

    /*Go the edit mode*/
    lv_test_encoder_click();

    /*Adjust the right knob*/
    lv_test_encoder_turn(-10);
    TEST_ASSERT_EQUAL(80, lv_slider_get_value(sliderRangeMode));  /*Updated?*/
    TEST_ASSERT_EQUAL(10, lv_slider_get_left_value(sliderRangeMode));     /*Maintained?*/

    /*Focus the left knob*/
    lv_test_encoder_click();

    /*Adjust the left knob*/
    lv_test_encoder_turn(5);
    TEST_ASSERT_EQUAL(80, lv_slider_get_value(sliderRangeMode));  /*Maintained?*/
    TEST_ASSERT_EQUAL(15, lv_slider_get_left_value(sliderRangeMode));  /*Updated?*/

}

void test_normal_mode_slider_hit_test(void)
{
    /* Validate if point 0,0 can click in the _test_slider */
    lv_point_t point = {
        .x = 0,
        .y = 0
    };

    lv_hit_test_info_t info = {
        .res = false,
        .point = &point
    };

    lv_slider_set_value(sliderNormalMode, 100, LV_ANIM_OFF);
    lv_obj_send_event(sliderNormalMode, LV_EVENT_HIT_TEST, (void *) &info);

    /* point can click _test_slider */
    TEST_ASSERT(info.res);
}

void test_slider_range_event_hit_test(void)
{
    /* Validate if point 0,0 can click in the _test_slider */
    lv_point_t point = {
        .x = 0,
        .y = 0
    };

    lv_hit_test_info_t info = {
        .res = false,
        .point = &point
    };
    lv_obj_send_event(sliderRangeMode, LV_EVENT_HIT_TEST, (void *) &info);

    /* point can click _test_slider in the left knob */
    TEST_ASSERT(info.res);
}

#endif
