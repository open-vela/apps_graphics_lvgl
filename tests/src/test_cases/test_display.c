#if LV_BUILD_TEST
#include "../lvgl.h"
#include "../../lvgl_private.h"
#include "unity/unity.h"

/*Bypassing resolution check*/
#define TEST_DISPLAY_ASSERT_EQUAL_SCREENSHOT(path) TEST_ASSERT_MESSAGE(lv_test_assert_image_eq(path), path);

void setUp(void)
{
    /* Function run before every test */
}

void tearDown(void)
{
    lv_display_set_matrix_rotation(NULL, false);
    lv_obj_clean(lv_screen_active());
}

#if LV_DRAW_TRANSFORM_USE_MATRIX
static void test_matrix_transform_area(lv_display_t * display)
{
    lv_area_t ori_area = { 0 };
    lv_area_set_width(&ori_area, 100);
    lv_area_set_height(&ori_area, 100);

    lv_matrix_t matrix;
    lv_matrix_identity(&matrix);

    const int32_t hor_res = lv_display_get_original_horizontal_resolution(display);
    const int32_t ver_res = lv_display_get_original_vertical_resolution(display);

    switch(lv_display_get_rotation(display)) {
        case LV_DISPLAY_ROTATION_0:
            break;
        case LV_DISPLAY_ROTATION_90:
            lv_matrix_rotate(&matrix, 270);
            lv_matrix_translate(&matrix, -ver_res, 0);
            break;
        case LV_DISPLAY_ROTATION_180:
            lv_matrix_rotate(&matrix, 180);
            lv_matrix_translate(&matrix, -hor_res, -ver_res);
            break;
        case LV_DISPLAY_ROTATION_270:
            lv_matrix_rotate(&matrix, 90);
            lv_matrix_translate(&matrix, 0, -hor_res);
            break;
    }

    lv_area_t disp_area = ori_area;
    lv_display_rotate_area(display, &disp_area);

    lv_area_t matrix_area = lv_matrix_transform_area(&matrix, &ori_area);

    TEST_ASSERT_TRUE(_lv_area_is_equal(&disp_area, &matrix_area));
}
#endif

void test_display_matrix_rotation(void)
{
#if LV_DRAW_TRANSFORM_USE_MATRIX
    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj, 300, 200);
    lv_obj_set_pos(obj, 30, 20);
    lv_obj_t * label = lv_label_create(obj);

    lv_display_t * disp = lv_obj_get_display(obj);
    lv_display_set_matrix_rotation(disp, true);
    TEST_ASSERT_TRUE(lv_display_get_matrix_rotation(disp));

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
    test_matrix_transform_area(disp);
    lv_label_set_text(label, "Rotation: 0 degrees");
    TEST_DISPLAY_ASSERT_EQUAL_SCREENSHOT("display_matrix_rotation_0.png");

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    test_matrix_transform_area(disp);
    lv_label_set_text(label, "Rotation: 90 degrees");
    TEST_DISPLAY_ASSERT_EQUAL_SCREENSHOT("display_matrix_rotation_90.png");

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
    test_matrix_transform_area(disp);
    lv_label_set_text(label, "Rotation: 180 degrees");
    TEST_DISPLAY_ASSERT_EQUAL_SCREENSHOT("display_matrix_rotation_180.png");

    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    test_matrix_transform_area(disp);
    lv_label_set_text(label, "Rotation: 270 degrees");
    TEST_DISPLAY_ASSERT_EQUAL_SCREENSHOT("display_matrix_rotation_270.png");

    lv_display_set_matrix_rotation(disp, false);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
    test_matrix_transform_area(disp);
    lv_label_set_text(label, "Rotation: 0 degrees");
    TEST_DISPLAY_ASSERT_EQUAL_SCREENSHOT("display_matrix_rotation_0.png");
#else
    TEST_PASS();
#endif
}

#endif
