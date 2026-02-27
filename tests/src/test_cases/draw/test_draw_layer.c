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
    lv_obj_clean(lv_screen_active());
}

void test_draw_layer_bitmap_mask(void)
{
    LV_IMAGE_DECLARE(test_image_cogwheel_a8);

    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj, 200, 200);
    lv_obj_set_style_bg_color(obj, lv_color_hex3(0xf88), 0);
    lv_obj_set_style_bitmap_mask_src(obj, &test_image_cogwheel_a8, 0);
    lv_obj_center(obj);

    lv_obj_t * label = lv_label_create(obj);
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_text(label,
                      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque suscipit risus nec pharetra pulvinar. In hac habitasse platea dictumst. Proin placerat congue massa eu luctus. Suspendisse risus nulla, consectetur eget odio ut, mollis sollicitudin magna. Suspendisse volutpat consequat laoreet. Aenean sodales suscipit leo, vitae pulvinar lorem pulvinar eu. Nullam molestie hendrerit est sit amet imperdiet.");
    lv_obj_center(label);

    TEST_ASSERT_EQUAL_SCREENSHOT("draw/draw_layer_bitmap_mask.png");

}

/**
 * Test that layer opa is properly restored when layer_get_area fails
 * (e.g., widget is outside clip area). Without the fix, the layer->opa
 * would leak to sibling widgets, causing cascading opacity corruption.
 *
 * Regression test for: "fix(lvgl): restore layer opa on early return
 * in lv_obj_refr to prevent sibling opacity corruption"
 */
void test_draw_layer_opa_restore_on_early_return(void)
{
    lv_obj_t * parent = lv_obj_create(lv_screen_active());
    lv_obj_set_size(parent, 200, 200);
    lv_obj_center(parent);
    lv_obj_set_style_clip_corner(parent, true, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);

    /*
     * Child 1: has opa_layered set (triggers layer creation in lv_obj_refr),
     * and is positioned completely outside the parent's clip area.
     * This causes layer_get_area() to return LV_RESULT_INVALID.
     * Before the fix, layer->opa was NOT restored on this early return path.
     */
    lv_obj_t * child_offscreen = lv_obj_create(parent);
    lv_obj_set_size(child_offscreen, 50, 50);
    lv_obj_set_pos(child_offscreen, -500, -500);  /* Way outside clip area */
    lv_obj_set_style_opa(child_offscreen, 51, 0);  /* Low opa to make corruption obvious */
    lv_obj_set_style_opa_layered(child_offscreen, 128, 0);  /* Forces layer path */
    lv_obj_set_style_bg_color(child_offscreen, lv_color_hex(0xff0000), 0);

    /*
     * Child 2: a normal visible sibling. Its effective opa should be
     * determined only by its own style, not corrupted by child_offscreen's opa.
     */
    lv_obj_t * child_visible = lv_obj_create(parent);
    lv_obj_set_size(child_visible, 80, 80);
    lv_obj_center(child_visible);
    lv_obj_set_style_opa(child_visible, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(child_visible, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_bg_opa(child_visible, LV_OPA_COVER, 0);

    /* Force a full refresh */
    lv_refr_now(NULL);

    /*
     * After refresh, verify the visible child's resolved opa is correct.
     * If the bug were present, child_visible would inherit the corrupted
     * layer->opa from child_offscreen's early return (opa ~51 instead of 255).
     */
    lv_opa_t visible_opa = lv_obj_get_style_opa(child_visible, LV_PART_MAIN);
    TEST_ASSERT_EQUAL(LV_OPA_COVER, visible_opa);

    /*
     * Also verify the parent's opa is not corrupted.
     * The parent should still report its own opa, unaffected by children.
     */
    lv_opa_t parent_opa = lv_obj_get_style_opa(parent, LV_PART_MAIN);
    TEST_ASSERT_EQUAL(LV_OPA_COVER, parent_opa);
}

#endif
