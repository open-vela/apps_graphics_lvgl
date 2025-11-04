#if LV_BUILD_TEST
#include "../lvgl.h"
#include "lv_test_helpers.h"
#include "unity/unity.h"

#define TEST_EXPAND_SIZE 20

void setUp(void)
{
    /* Function run before every test */
}

void tearDown(void)
{
    /* Function run after every test */
    lv_obj_clean(lv_screen_active());
}

static void get_original_image_size(const char * src, lv_image_header_t * header)
{
    uint32_t saved = lv_image_decoder_get_size_expand();
    lv_image_decoder_set_size_expand(0);
    TEST_ASSERT_EQUAL(LV_RESULT_OK, lv_image_decoder_get_info(src, header));
    lv_image_decoder_set_size_expand(saved);
}

static void decode_with_expand_flag(const char * src, bool no_expand,
                                    int32_t * out_w, int32_t * out_h)
{
    lv_image_decoder_dsc_t dsc;
    lv_image_decoder_args_t args;
    lv_memzero(&args, sizeof(args));
    args.no_size_expand = no_expand;
    args.no_cache = true;

    TEST_ASSERT_EQUAL(LV_RESULT_OK, lv_image_decoder_open(&dsc, src, &args));
    TEST_ASSERT_NOT_NULL(dsc.decoded);
    *out_w = dsc.decoded->header.w;
    *out_h = dsc.decoded->header.h;
    lv_image_decoder_close(&dsc);
}

static void check_no_size_expand_for(const char * src)
{
    lv_image_header_t header;
    get_original_image_size(src, &header);
    const int32_t orig_w = header.w;
    const int32_t orig_h = header.h;

    int32_t w_noexp, h_noexp, w_exp, h_exp;
    /* Ensure no_size_expand=true really prevents expansion even when global expand>0 */
    uint32_t saved_expand = lv_image_decoder_get_size_expand();
    lv_image_decoder_set_size_expand(TEST_EXPAND_SIZE);
    decode_with_expand_flag(src, true, &w_noexp, &h_noexp);

    decode_with_expand_flag(src, false, &w_exp, &h_exp);
    lv_image_decoder_set_size_expand(saved_expand);

    TEST_ASSERT_EQUAL_INT32(orig_w, w_noexp);
    TEST_ASSERT_EQUAL_INT32(orig_h, h_noexp);
    TEST_ASSERT_EQUAL_INT32(orig_w + TEST_EXPAND_SIZE * 2, w_exp);
    TEST_ASSERT_EQUAL_INT32(orig_h + TEST_EXPAND_SIZE * 2, h_exp);
}

void test_image_decoder_no_size_expand_png(void)
{
    check_no_size_expand_for("A:src/test_assets/test_img_lvgl_logo.png");
}

void test_image_decoder_no_size_expand_jpeg(void)
{
    check_no_size_expand_for("A:src/test_assets/test_img_lvgl_logo.jpg");
}

static void test_image_expand(const char * src, const char * ref_path, int32_t expand_size)
{
    lv_image_cache_drop(src);

    lv_image_decoder_set_size_expand(expand_size);

    lv_obj_clean(lv_screen_active());
    lv_obj_t * img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, src);
    lv_obj_center(img);
    lv_obj_set_style_outline_width(img, 1, 0);
    lv_obj_set_style_outline_color(img, lv_color_black(), 0);

    TEST_ASSERT_EQUAL_SCREENSHOT(ref_path);
}

void test_image_decoder_screenshot(void)
{
    test_image_expand("A:src/test_assets/test_img_lvgl_logo.png", "draw/image_decoder_png_no_expand.png", 0);
    test_image_expand("A:src/test_assets/test_img_lvgl_logo.png", "draw/image_decoder_png_20px_expand.png",
                      TEST_EXPAND_SIZE);

    test_image_expand("A:src/test_assets/test_img_lvgl_logo.jpg", "draw/image_decoder_jpg_no_expand.png", 0);
    test_image_expand("A:src/test_assets/test_img_lvgl_logo.jpg", "draw/image_decoder_jpg_20px_expand.png",
                      TEST_EXPAND_SIZE);
}

#endif
