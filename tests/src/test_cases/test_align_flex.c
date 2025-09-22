#if LV_BUILD_TEST
#include "../lvgl.h"
#include "../../lvgl_private.h"

#include "unity/unity.h"

static lv_obj_t * active_screen = NULL;

void setUp(void)
{
    active_screen = lv_screen_active();
}

void tearDown(void)
{
    lv_obj_clean(active_screen);
}

void test_flex_use_rtl(void)
{
    lv_obj_t * test = lv_obj_create(lv_screen_active());
    lv_obj_center(test);
    lv_obj_set_width(test, LV_PCT(40));
    lv_obj_set_height(test, LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(test, 2, 0);
    lv_obj_set_style_base_dir(test, LV_BASE_DIR_RTL, 0);

    lv_obj_set_flex_flow(test, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(test, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    for(int i = 0; i < 8; i++) {
        lv_obj_t * obj = lv_obj_create(test);
        lv_obj_set_size(obj, 70, 70);
        lv_obj_set_style_border_width(obj, 2, 0);
    }

    TEST_ASSERT_EQUAL_SCREENSHOT("flex_use_rtl.png");
}

#endif
