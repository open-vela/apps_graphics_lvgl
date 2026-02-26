#if LV_BUILD_TEST
#include "../lvgl.h"

#if LV_USE_SNAPSHOT

#include "unity/unity.h"

#define NUM_SNAPSHOTS 10

void test_snapshot_should_not_leak_memory(void)
{
    uint32_t idx = 0;
    size_t initial_available_memory = 0;
    size_t final_available_memory = 0;
    lv_mem_monitor_t monitor;

    lv_draw_buf_t * snapshots[NUM_SNAPSHOTS] = {NULL};

    lv_mem_monitor(&monitor);
    initial_available_memory = monitor.free_size;

    for(idx = 0; idx < NUM_SNAPSHOTS; idx++) {
        snapshots[idx] = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_NATIVE_WITH_ALPHA);
        TEST_ASSERT_NOT_NULL(snapshots[idx]);
    }

    for(idx = 0; idx < NUM_SNAPSHOTS; idx++) {
        lv_draw_buf_destroy(snapshots[idx]);
    }

    lv_mem_monitor(&monitor);
    final_available_memory = monitor.free_size;

    TEST_ASSERT_EQUAL(initial_available_memory, final_available_memory);
}

void test_snapshot_with_transform_should_not_leak_memory(void)
{
    uint32_t idx = 0;
    size_t initial_available_memory = 0;
    size_t final_available_memory = 0;
    lv_mem_monitor_t monitor;

    lv_draw_buf_t * snapshots[NUM_SNAPSHOTS] = {NULL};
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_label_set_text(label, "Wubba lubba dub dub!");
    lv_obj_set_style_transform_rotation(label, 450, 0);

    lv_mem_monitor(&monitor);
    initial_available_memory = monitor.free_size;

    for(idx = 0; idx < NUM_SNAPSHOTS; idx++) {
        snapshots[idx] = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_NATIVE_WITH_ALPHA);
        TEST_ASSERT_NOT_NULL(snapshots[idx]);
    }

    for(idx = 0; idx < NUM_SNAPSHOTS; idx++) {
        lv_draw_buf_destroy(snapshots[idx]);
    }

    lv_mem_monitor(&monitor);
    final_available_memory = monitor.free_size;
    lv_obj_delete(label);

    TEST_ASSERT_EQUAL(initial_available_memory, final_available_memory);
}

void test_snapshot_take_snapshot_immidiately_after_obj_create(void)
{
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_label_set_text(label, "Wubba lubba dub dub!");

    lv_draw_buf_t * draw_dsc = lv_snapshot_take(label, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_t * img_obj = lv_image_create(lv_screen_active());
    lv_image_set_src(img_obj, draw_dsc);

    lv_obj_delete(label);

    TEST_ASSERT_EQUAL_SCREENSHOT("snapshot_0.png");

    lv_obj_center(img_obj);
    lv_image_set_inner_align(img_obj, LV_IMAGE_ALIGN_CENTER);
    lv_image_set_rotation(img_obj, 450);

    TEST_ASSERT_EQUAL_SCREENSHOT("snapshot_1.png");

    lv_obj_delete(img_obj);
    lv_draw_buf_destroy(draw_dsc);
}

void test_snapshot_take_snapshot_with_transform(void)
{
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_label_set_text(label, "Wubba lubba dub dub!");
    lv_obj_set_style_transform_rotation(label, 450, 0);

    lv_draw_buf_t * draw_dsc = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);

    lv_obj_delete(label);

    lv_obj_t * img_obj = lv_image_create(lv_screen_active());
    lv_image_set_src(img_obj, draw_dsc);

    TEST_ASSERT_EQUAL_SCREENSHOT("snapshot_2.png");

    lv_obj_delete(img_obj);
    lv_draw_buf_destroy(draw_dsc);
}

/* Regression test: rotated image with large bounding box must not
 * cause heap-buffer-overflow in transform_argb8888.
 *
 * The bug: in img_draw_core, buf_h = MAX_BUF_SIZE / buf_stride.
 * MAX_BUF_SIZE depends on _lv_refr_get_disp_refreshing(). When
 * the refreshing display is small but the rotated image bounding
 * box is large, buf_stride > MAX_BUF_SIZE, buf_h = 0, malloc(0),
 * and OOB write in transform_argb8888.
 *
 * We reproduce this by temporarily shrinking the default display
 * to 10x10 (making MAX_BUF_SIZE tiny), placing a large rotated
 * image, and taking a snapshot. The snapshot renders into a large
 * off-screen layer while MAX_BUF_SIZE stays small.
 * Without fix: buf_h=0 -> infinite loop in img_draw_core.
 * With fix: buf_h clamped to 1 -> completes normally. */
void test_snapshot_rotated_large_bbox_no_overflow(void)
{
    lv_display_t * disp = lv_display_get_default();

    /* Save original resolution and shrink to 10x10.
     *   MAX_BUF_SIZE = 4 * 10 * 4(ARGB8888) = 160 bytes
     *
     * A 200x200 image rotated 45° has bounding box ~283px.
     *   buf_stride = 283 * 4 = 1132 >> 160 = MAX_BUF_SIZE
     *   buf_h = 160 / 1132 = 0  (BUG without fix) */
    int32_t orig_hor = lv_display_get_horizontal_resolution(disp);
    int32_t orig_ver = lv_display_get_vertical_resolution(disp);
    lv_display_set_resolution(disp, 10, 10);

    /* Create a 200x200 ARGB8888 draw_buf as image source */
    lv_draw_buf_t * src_buf = lv_draw_buf_create(200, 200, LV_COLOR_FORMAT_ARGB8888, 0);
    TEST_ASSERT_NOT_NULL(src_buf);
    lv_draw_buf_clear(src_buf, NULL);

    /* Place a rotated image on the current screen */
    lv_obj_t * img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, src_buf);
    lv_image_set_rotation(img, 450);  /* 45 degrees */
    lv_obj_center(img);
    lv_obj_update_layout(img);

    /* Take a snapshot. Internally snapshot sets this display as
     * refreshing (small MAX_BUF_SIZE), but renders into a layer
     * sized to the object's bounding box (~283x283).
     * Without fix: buf_h=0 -> infinite loop.
     * With fix: buf_h clamped to 1 -> safe. */
    lv_draw_buf_t * snapshot = lv_snapshot_take(img, LV_COLOR_FORMAT_ARGB8888);
    TEST_ASSERT_NOT_NULL(snapshot);

    /* Cleanup and restore original resolution */
    lv_draw_buf_destroy(snapshot);
    lv_obj_delete(img);
    lv_draw_buf_destroy(src_buf);
    lv_display_set_resolution(disp, orig_hor, orig_ver);
}

#else /*LV_USE_SNAPSHOT*/

void test_snapshot_should_not_leak_memory(void)
{

}

void test_snapshot_with_transform_should_not_leak_memory(void)
{

}

void test_snapshot_take_snapshot_immidiately_after_obj_create(void)
{

}

void test_snapshot_take_snapshot_with_transform(void)
{

}

void test_snapshot_rotated_large_bbox_no_overflow(void)
{

}

#endif

#endif
