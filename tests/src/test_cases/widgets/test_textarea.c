#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"
#include "lv_test_indev.h"

static lv_obj_t * active_screen = NULL;
static lv_obj_t * textarea = NULL;

static const char * textarea_default_text = "";
static char insert_replacre_text[10] = "123";

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_INSERT) {
        const char * txt = lv_event_get_param(e);
        if(txt && txt[0] >= '0' && txt[0] <= '9') {
            /* insert */
        }
        else {
            lv_obj_t * obj = lv_event_get_target(e);
            lv_textarea_set_insert_replace(obj, insert_replacre_text);
        }
    }
}

static bool test_font_get_glyph_dsc(const lv_font_t * font,
                                    lv_font_glyph_dsc_t * dsc_out,
                                    uint32_t unicode_letter,
                                    uint32_t unicode_letter_next)
{
    LV_UNUSED(font);
    LV_UNUSED(unicode_letter_next);
    if(unicode_letter >= 0x20 && unicode_letter <= 0x7E) {
        dsc_out->adv_w = 10;
        dsc_out->box_w = 8;
        dsc_out->box_h = 12;
        return true;
    }
    return false;
}

static lv_font_t test_font_no_bullet = {
    .get_glyph_dsc = test_font_get_glyph_dsc,
    .line_height = 14,
    .base_line = 12,
};

void setUp(void)
{
    active_screen = lv_screen_active();
    textarea = lv_textarea_create(active_screen);
}

void tearDown(void)
{
    /* Function run after every test */
    lv_obj_clean(active_screen);
}

void test_textarea_should_have_valid_documented_default_values(void)
{
    TEST_ASSERT(lv_textarea_get_cursor_click_pos(textarea));
    TEST_ASSERT_EQUAL(0U, lv_textarea_get_one_line(textarea));
    /* No placeholder text should be set on widget creation */
    TEST_ASSERT_EQUAL_STRING(textarea_default_text, lv_textarea_get_placeholder_text(textarea));
    TEST_ASSERT_EQUAL_STRING(textarea_default_text, lv_textarea_get_text(textarea));
}

/* When in password mode the lv_textarea_get_text function returns
 * the actual text, not the bullet characters. */
void test_textarea_should_return_actual_text_when_password_mode_is_enabled(void)
{
    const char * text = "Hello LVGL!";

    lv_textarea_add_text(textarea, text);
    lv_textarea_set_password_mode(textarea, true);

    TEST_ASSERT_TRUE(lv_textarea_get_password_mode(textarea));
    TEST_ASSERT_EQUAL_STRING(text, lv_textarea_get_text(textarea));
}

void test_textarea_should_update_label_style_with_one_line_enabled(void)
{
    lv_textarea_t * txt_ptr = (lv_textarea_t *) textarea;

    lv_textarea_add_text(textarea, "Hi");
    lv_textarea_set_one_line(textarea, true);

    int32_t left_padding = lv_obj_get_style_pad_left(txt_ptr->label, LV_PART_MAIN);
    int32_t right_padding = lv_obj_get_style_pad_right(txt_ptr->label, LV_PART_MAIN);
    int32_t line_width = lv_obj_get_width(txt_ptr->label);
    int32_t expected_size = left_padding + right_padding + line_width;

    TEST_ASSERT(lv_textarea_get_one_line(textarea));
    TEST_ASSERT_EQUAL_UINT16(expected_size, lv_obj_get_width(txt_ptr->label));
    TEST_ASSERT_EQUAL_UINT16(lv_pct(100), lv_obj_get_style_min_width(txt_ptr->label, LV_PART_MAIN));
}

void test_textarea_cursor_click_pos_field_update(void)
{
    lv_textarea_set_cursor_click_pos(textarea, false);

    TEST_ASSERT_FALSE(lv_textarea_get_cursor_click_pos(textarea));
}

void test_textarea_should_update_placeholder_text(void)
{
    const char * new_placeholder = "LVGL Rocks!!!!!";
    const char * text = "Hello LVGL!";

    /* Allocating memory for placeholder text */
    lv_textarea_set_placeholder_text(textarea, text);
    TEST_ASSERT_EQUAL_STRING(text, lv_textarea_get_placeholder_text(textarea));

    /* Reallocating memory for the new placeholder text */
    lv_textarea_set_placeholder_text(textarea, new_placeholder);
    TEST_ASSERT_EQUAL_STRING(new_placeholder, lv_textarea_get_placeholder_text(textarea));

    /* Freeing allocated memory for placeholder text */
    lv_textarea_set_placeholder_text(textarea, "");
    TEST_ASSERT_EQUAL_STRING("", lv_textarea_get_placeholder_text(textarea));
}

void test_textarea_should_keep_only_accepted_chars(void)
{
    const char * accepted_list = "abcd";

    lv_textarea_set_accepted_chars(textarea, accepted_list);
    lv_textarea_set_text(textarea, "abcde");

    TEST_ASSERT_EQUAL_STRING(accepted_list, lv_textarea_get_text(textarea));
}

void test_textarea_in_one_line_mode_should_ignore_line_break_characters(void)
{
    lv_textarea_set_one_line(textarea, true);

    lv_textarea_add_char(textarea, '\n');
    TEST_ASSERT_EQUAL_STRING(textarea_default_text, lv_textarea_get_text(textarea));

    lv_textarea_add_char(textarea, '\r');
    TEST_ASSERT_EQUAL_STRING(textarea_default_text, lv_textarea_get_text(textarea));
}

void test_textarea_set_max_length(void)
{
    lv_textarea_set_max_length(textarea, 8);
    lv_textarea_add_text(textarea, "1234567890");
    TEST_ASSERT_EQUAL_STRING("12345678", lv_textarea_get_text(textarea));

    lv_textarea_set_password_mode(textarea, true);
    lv_textarea_set_text(textarea, "1234567890");
    TEST_ASSERT_EQUAL_STRING("1234567890", lv_textarea_get_text(textarea));
}

void test_textarea_set_insert_replace(void)
{
    lv_textarea_set_text(textarea, "1234567890");
    lv_obj_add_event_cb(textarea, event_handler, LV_EVENT_INSERT, NULL);

    lv_textarea_add_text(textarea, "abc");
    TEST_ASSERT_EQUAL_STRING("1234567890123", lv_textarea_get_text(textarea));

    lv_memset(insert_replacre_text, 0, sizeof(insert_replacre_text));
    lv_textarea_add_text(textarea, "abc");
    TEST_ASSERT_EQUAL_STRING("1234567890123", lv_textarea_get_text(textarea));
}

void test_textarea_placeholder_text_show_one_line(void)
{
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_placeholder_text(textarea, "1234567890");

    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_placeholder_show.png");
}

void test_textarea_password_mode(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_password_mode(textarea, true);

    lv_textarea_set_text(textarea, "123456");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode.png");

    lv_textarea_set_text(textarea, "123456789");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_update.png");

    lv_textarea_add_text(textarea, "abc");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_add_text.png");

    lv_textarea_add_char(textarea, 'a');
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_add_char.png");

    lv_textarea_delete_char(textarea);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_delete_char.png");

    lv_textarea_set_password_mode(textarea, false);
    lv_textarea_set_text(textarea, "1234567890");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_normal_mode.png");
}

void test_textarea_password_mode_hide_char(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_password_mode(textarea, true);

    lv_textarea_set_password_show_time(textarea, 0);
    lv_textarea_add_char(textarea, 'a');

    lv_test_wait(50);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_hide_char_one.png");

    lv_textarea_set_password_show_time(textarea, 500);
    lv_textarea_add_char(textarea, 'b');

    lv_test_wait(550);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_nide_char_tow.png");

    lv_textarea_add_char(textarea, 'c');
    lv_textarea_set_password_mode(textarea, false);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_password_mode_to_normal_mode.png");
}

void test_textarea_set_password_bullet(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_password_mode(textarea, true);
    lv_textarea_set_text(textarea, "1234567890");

    lv_obj_set_style_text_font(textarea, &test_font_no_bullet, 0);
    TEST_ASSERT_EQUAL_STRING("*", lv_textarea_get_password_bullet(textarea));

    lv_textarea_set_password_bullet(textarea, "*");
    TEST_ASSERT_EQUAL_STRING("*", lv_textarea_get_password_bullet(textarea));

    lv_textarea_set_password_bullet(textarea, NULL);
    TEST_ASSERT_EQUAL_STRING("*", lv_textarea_get_password_bullet(textarea));
}

void test_textarea_delete_char(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "1234567890");
    lv_textarea_delete_char(textarea);
}

void test_textarea_delete_char_forward(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "1234567890");
    lv_textarea_delete_char_forward(textarea);
}

void test_textarea_set_text_selection(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "1234567890");

    lv_textarea_set_text_selection(textarea, true);
    TEST_ASSERT_EQUAL(1U, lv_textarea_get_text_selection(textarea));

    lv_obj_t * label = lv_textarea_get_label(textarea);
    lv_label_set_text_selection_start(label, 0);
    lv_label_set_text_selection_end(label, 10);
    TEST_ASSERT_EQUAL(1U, lv_textarea_text_is_selected(textarea));

    lv_textarea_clear_selection(textarea);
    TEST_ASSERT_EQUAL(0U, lv_textarea_text_is_selected(textarea));
}

void test_textarea_set_password_show_time(void)
{
    lv_textarea_set_password_show_time(textarea, 1000);

    TEST_ASSERT_EQUAL_UINT32(1000, lv_textarea_get_password_show_time(textarea));
}

void test_textarea_set_align(void)
{
    lv_textarea_set_align(textarea, LV_TEXT_ALIGN_CENTER);
    lv_textarea_set_text(textarea, "1234567890");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_align_center.png");

    lv_textarea_set_align(textarea, LV_TEXT_ALIGN_LEFT);
    lv_textarea_set_text(textarea, "1234567890");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_align_left.png");

    lv_textarea_set_align(textarea, LV_TEXT_ALIGN_RIGHT);
    lv_textarea_set_text(textarea, "1234567890");
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_align_right.png");
}

void test_textarea_corsor_show(void)
{
    lv_obj_set_style_anim_duration(textarea, 1000, LV_PART_CURSOR);
    lv_obj_send_event(textarea, LV_EVENT_FOCUSED, NULL);

    lv_test_wait(1000);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_cursor_show.png");
}

void test_textarea_set_cursor_pos(void)
{
    lv_obj_set_size(textarea, 100, 60);

    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "123456789012345678901234567890123456789012345678901");

    lv_textarea_set_cursor_pos(textarea, 0);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_cursor_pos_top.png");

    lv_textarea_set_cursor_pos(textarea, 50);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_cursor_pos_bottom.png");

    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_text(textarea, "123456789012345678901234567890123456789012345678901");

    lv_textarea_set_cursor_pos(textarea, 0);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_cursor_pos_left.png");

    lv_textarea_set_cursor_pos(textarea, 50);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_cursor_pos_right.png");

    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "123456789012345678901234567890123456789012345678901");
    lv_obj_set_size(lv_textarea_get_label(textarea), 100, 200);

    lv_obj_scroll_to_y(textarea, 300, LV_ANIM_OFF);
    lv_textarea_set_cursor_pos(textarea, 4);
    lv_test_wait(500);
    TEST_ASSERT_EQUAL_UINT32(0, lv_obj_get_scroll_y(textarea));

    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_cursor_pos(textarea, 50);
    lv_test_wait(100);
    lv_textarea_set_cursor_pos(textarea, 10);
    lv_test_wait(100);
    TEST_ASSERT_EQUAL_UINT32(10, lv_textarea_get_cursor_pos(textarea));
}

void test_textarea_get_current_char(void)
{
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "1234567890");

    lv_textarea_set_cursor_pos(textarea, 2);
    TEST_ASSERT_EQUAL_INT('2', lv_textarea_get_current_char(textarea));

    lv_textarea_set_cursor_pos(textarea, 0);
    TEST_ASSERT_EQUAL_INT(0, lv_textarea_get_current_char(textarea));
}

void test_textarea_cursor_position_on_click(void)
{
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_text(textarea, "Hello World");
    lv_obj_set_size(textarea, 200, 100);

    lv_textarea_set_cursor_click_pos(textarea, true);

    lv_test_mouse_click_at(5, 15);
    TEST_ASSERT_EQUAL_UINT32(0, lv_textarea_get_cursor_pos(textarea));

    lv_test_mouse_click_at(180, 15);
    TEST_ASSERT_EQUAL_UINT32(11, lv_textarea_get_cursor_pos(textarea));

    int32_t label_width = lv_obj_get_width(lv_textarea_get_label(textarea));
    lv_test_mouse_click_at(label_width + 20, 15);
    TEST_ASSERT_EQUAL_UINT32(11, lv_textarea_get_cursor_pos(textarea));

    lv_textarea_set_text_selection(textarea, true);
    lv_test_mouse_click_at(40, 15);
    lv_test_mouse_press();
    lv_test_wait(100);
    lv_test_mouse_move_to(100, 15);
    lv_test_wait(100);
    lv_test_mouse_release();
    TEST_ASSERT_EQUAL_UINT32(11, lv_textarea_get_cursor_pos(textarea));

    lv_test_mouse_click_at(40, 15);
    lv_test_mouse_press();
    lv_test_wait(100);
    lv_test_mouse_move_to(5, 15);
    lv_test_wait(100);
    lv_test_mouse_release();
    TEST_ASSERT_EQUAL_UINT32(0, lv_textarea_get_cursor_pos(textarea));

    lv_textarea_set_text_selection(textarea, false);
    lv_test_mouse_click_at(5, 15);
    TEST_ASSERT_EQUAL_UINT32(0, lv_textarea_get_cursor_pos(textarea));

    lv_textarea_set_cursor_click_pos(textarea, false);
    uint32_t pos_before = lv_textarea_get_cursor_pos(textarea);
    lv_test_mouse_click_at(50, 15);
    TEST_ASSERT_EQUAL_UINT32(pos_before, lv_textarea_get_cursor_pos(textarea));
}

void test_textarea_key_event(void)
{
    lv_textarea_set_text(textarea, "Hello World");
    lv_textarea_set_cursor_pos(textarea, 11);

    uint32_t key = LV_KEY_BACKSPACE;
    lv_obj_send_event(textarea, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_STRING("Hello Worl", lv_textarea_get_text(textarea));

    lv_textarea_set_text(textarea, "Hello World");
    lv_textarea_set_cursor_pos(textarea, 0);

    key = LV_KEY_DEL;
    lv_obj_send_event(textarea, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_STRING("ello World", lv_textarea_get_text(textarea));

    key = LV_KEY_HOME;
    lv_textarea_set_cursor_pos(textarea, 10);
    lv_obj_send_event(textarea, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_UINT32(0, lv_textarea_get_cursor_pos(textarea));

    key = LV_KEY_END;
    lv_textarea_set_cursor_pos(textarea, 0);
    lv_obj_send_event(textarea, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_UINT32(10, lv_textarea_get_cursor_pos(textarea));

    lv_obj_set_size(textarea, 100, 40);
    lv_textarea_set_one_line(textarea, false);
    lv_textarea_set_text(textarea, "Hello World, this is a test for the key event");
    lv_textarea_set_cursor_pos(textarea, 0);

    key = LV_KEY_DOWN;
    lv_obj_send_event(textarea, LV_EVENT_KEY, (void *) &key);
    lv_test_wait(100);
    TEST_ASSERT_EQUAL_UINT32(6, lv_textarea_get_cursor_pos(textarea));

    lv_textarea_set_text(textarea, "Hello World");
    lv_textarea_set_cursor_pos(textarea, 11);

    key = 49;
    lv_obj_send_event(textarea, LV_EVENT_KEY, (void *) &key);
    TEST_ASSERT_EQUAL_STRING("Hello World1", lv_textarea_get_text(textarea));
}

void test_textarea_check_placeholder_text_postion(void)
{
    lv_textarea_set_placeholder_text(textarea, "Placeholder");
    lv_textarea_set_one_line(textarea, true);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_placeholder_center.png");

    lv_textarea_set_one_line(textarea, false);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_placeholder_top.png");

    lv_obj_set_style_align(lv_textarea_get_label(textarea), LV_ALIGN_LEFT_MID, LV_PART_MAIN);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_placeholder_left_mid.png");

    lv_obj_set_style_align(lv_textarea_get_label(textarea), LV_ALIGN_TOP_LEFT, LV_PART_MAIN);
    lv_obj_set_style_pad_top(lv_textarea_get_label(textarea), 50, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_pad_left(lv_textarea_get_label(textarea), 50, LV_PART_TEXTAREA_PLACEHOLDER);
    TEST_ASSERT_EQUAL_SCREENSHOT("textarea_placeholder_pad_left_top_50.png");
}

#endif
