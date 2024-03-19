#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"

#include "lv_test_helpers.h"
#include "lv_test_indev.h"

void test_checkbox_creation_successful(void);
void test_checkbox_should_call_event_handler_on_click_when_enabled(void);
void test_checkbox_should_have_default_text_when_created(void);
void test_checkbox_should_return_dynamically_allocated_text(void);
void test_checkbox_should_allocate_memory_for_static_text(void);

static lv_obj_t * test_checkbox_active_screen = NULL;
static lv_obj_t * _test_checkbox = NULL;

static volatile bool test_checkbox_event_called = false;

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(LV_EVENT_VALUE_CHANGED == code) {
        test_checkbox_event_called = true;
    }
}

void test_checkbox_creation_successful(void)
{
    test_checkbox_active_screen = lv_screen_active();
    _test_checkbox = lv_checkbox_create(test_checkbox_active_screen);

    TEST_ASSERT_NOT_NULL(_test_checkbox);
}

void test_checkbox_should_call_event_handler_on_click_when_enabled(void)
{
    test_checkbox_active_screen = lv_screen_active();
    _test_checkbox = lv_checkbox_create(test_checkbox_active_screen);

    lv_obj_add_state(_test_checkbox, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_test_checkbox, event_handler, LV_EVENT_ALL, NULL);

    lv_test_mouse_click_at(_test_checkbox->coords.x1, _test_checkbox->coords.y1);

    TEST_ASSERT_TRUE(test_checkbox_event_called);

    test_checkbox_event_called = false;
}

void test_checkbox_should_have_default_text_when_created(void)
{
    const char * default_text = "Check box";

    test_checkbox_active_screen = lv_screen_active();
    _test_checkbox = lv_checkbox_create(test_checkbox_active_screen);

    TEST_ASSERT_EQUAL_STRING(default_text, lv_checkbox_get_text(_test_checkbox));
    TEST_ASSERT_NOT_NULL(lv_checkbox_get_text(_test_checkbox));
}

void test_checkbox_should_return_dynamically_allocated_text(void)
{
    const char * message = "Hello World!";

    test_checkbox_active_screen = lv_screen_active();
    _test_checkbox = lv_checkbox_create(test_checkbox_active_screen);

    lv_checkbox_set_text(_test_checkbox, message);

    TEST_ASSERT_EQUAL_STRING(message, lv_checkbox_get_text(_test_checkbox));
    TEST_ASSERT_NOT_NULL(lv_checkbox_get_text(_test_checkbox));
}

void test_checkbox_should_allocate_memory_for_static_text(void)
{
    size_t initial_available_memory = 0;
    const char * static_text = "Keep me while you exist";

    lv_mem_monitor_t m1;
    lv_mem_monitor(&m1);

    test_checkbox_active_screen = lv_screen_active();
    _test_checkbox = lv_checkbox_create(test_checkbox_active_screen);

    initial_available_memory = m1.free_size;

    lv_checkbox_set_text_static(_test_checkbox, static_text);

    lv_mem_monitor(&m1);

    LV_HEAP_CHECK(TEST_ASSERT_LESS_OR_EQUAL(initial_available_memory, m1.free_size));
}

#if LV_FONT_DEJAVU_16_PERSIAN_HEBREW
void test_checkbox_rtl(void)
{
    const char * message =
        "מעבד, או בשמו המלא יחידת עיבוד מרכזית (באנגלית: CPU - Central Processing Unit).";

    lv_obj_t * screen = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, 800, 480);
    lv_obj_center(screen);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_100, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t * test_checkbox = lv_checkbox_create(test_checkbox_active_screen);

    lv_checkbox_set_text(test_checkbox, message);
    lv_obj_set_style_text_font(test_checkbox, &lv_font_dejavu_16_persian_hebrew, 0);
    lv_obj_center(test_checkbox);
    lv_obj_set_style_base_dir(test_checkbox, LV_BASE_DIR_RTL, 0);

    TEST_ASSERT_EQUAL_SCREENSHOT("widgets/checkbox_rtl_1.png");
}
#else
void test_checkbox_rtl(void)
{
    LV_LOG_WARN("Font `lv_font_dejavu_16_persian_hebrew` not enabled");
}
#endif

#endif
