#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"

#define TEST_EVENT_NAME(event_core_name) TEST_ASSERT_EQUAL_STRING("EVENT_" #event_core_name, lv_event_get_code_name(LV_EVENT_##event_core_name))


void setUp(void)
{
    /* Function run before every test */
}

void tearDown(void)
{
    /* Function run after every test */
}

static void event_object_deletion_cb(const lv_obj_class_t * cls, lv_event_t * e)
{
    LV_UNUSED(cls);
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_delete(lv_event_get_target(e));
    }
}

static const lv_obj_class_t event_object_deletion_class = {
    .event_cb = event_object_deletion_cb,
    .base_class = &lv_obj_class
};

/* Checks for memory leaks/invalid memory accesses on deleted objects */
void test_event_object_deletion(void)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&event_object_deletion_class, lv_screen_active());
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
}

/* Add and then remove event should not memory leak */
void test_event_should_not_memory_lean(void)
{
    lv_mem_monitor_t monitor;
    lv_mem_monitor(&monitor);
    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    size_t initial_free_size = monitor.free_size;

    for(int i = 0; i < 10; i++) {
        lv_obj_add_event_cb(obj, NULL, LV_EVENT_ALL, NULL);
    }

    lv_obj_delete(obj);

    lv_mem_monitor_t m2;
    lv_mem_monitor(&m2);
    TEST_ASSERT_LESS_OR_EQUAL_CHAR(initial_free_size, m2.free_size);
}

/* Test delayed removal of events */
void test_event_delay_removal(void)
{
    /* Minimal test setup with static storage */
    lv_event_list_t list = {0};

    /* Add test events */
    lv_event_dsc_t * dsc1 = lv_event_add(&list, NULL, LV_EVENT_VALUE_CHANGED, NULL);
    lv_event_dsc_t * dsc2 = lv_event_add(&list, NULL, LV_EVENT_CLICKED, NULL);
    TEST_ASSERT_NOT_NULL(dsc1);
    TEST_ASSERT_NOT_NULL(dsc2);
    lv_event_add_flag(&list, LV_EVENT_FLAG_TRAVERSING);

    uint32_t event_cnt = lv_event_get_count(&list);
    for(uint32_t i = 0; i < event_cnt; i++) {
        lv_event_dsc_t * curr_dsc = lv_event_get_dsc(&list, i);
        if(curr_dsc == dsc1) {
            lv_event_remove_dsc(&list, dsc1);
        }
    }

    lv_event_remove_flag(&list, LV_EVENT_FLAG_TRAVERSING);

    /* Verify list has REMOVED flag and NULL entry */
    TEST_ASSERT_TRUE(lv_event_has_flag(&list, LV_EVENT_FLAG_REMOVED));
    TEST_ASSERT_EQUAL_UINT32(2, lv_event_get_count(&list));

    /* Trigger delayed removal */
    lv_event_t e = {0};
    e.code = LV_EVENT_DELETE;
    lv_event_send(&list, &e, false);

    /* Verify list was compacted */
    TEST_ASSERT_EQUAL_UINT32(1, lv_event_get_count(&list));
    TEST_ASSERT_FALSE(lv_event_has_flag(&list, LV_EVENT_FLAG_REMOVED));
    lv_event_dsc_t * remaining_dsc = lv_event_get_dsc(&list, 0);
    TEST_ASSERT_EQUAL_PTR(dsc2, remaining_dsc);

    /* Cleanup */
    for(uint32_t i = 0; i < lv_event_get_count(&list); i++) {
        lv_event_dsc_t * dsc3 = lv_event_get_dsc(&list, i);
        if(dsc3) lv_free(dsc3);
    }
    lv_array_deinit(&list.array);
}

/* Test direct removal of events (non-TRAVERSING state) */
void test_event_direct_removal(void)
{
    lv_event_list_t list = {0};

    /* Add event */
    lv_event_dsc_t * dsc = lv_event_add(&list, NULL, LV_EVENT_VALUE_CHANGED, NULL);
    TEST_ASSERT_NOT_NULL(dsc);

    /* Remove event directly (non-TRAVERSING) */
    bool removed = lv_event_remove_dsc(&list, dsc);

    /* Verify removal */
    TEST_ASSERT_TRUE(removed);
    TEST_ASSERT_EQUAL_INT32(0, lv_event_get_count(&list));
    TEST_ASSERT_FALSE(lv_event_has_flag(&list, LV_EVENT_FLAG_REMOVED));
}

/* Test getter functions for event descriptor */
void test_event_dsc_getters(void)
{
    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    lv_obj_allocate_spec_attr(obj);
    TEST_ASSERT_NOT_NULL(obj->spec_attr);

    /* Test callback getter */
    lv_event_cb_t test_cb = (lv_event_cb_t)0x1234;
    lv_event_dsc_t * dsc = lv_obj_add_event_cb(obj, test_cb, LV_EVENT_VALUE_CHANGED, (void *)0x5678);
    TEST_ASSERT_EQUAL_PTR(test_cb, lv_event_dsc_get_cb(dsc));

    /* Test user data getter */
    TEST_ASSERT_EQUAL_PTR((void *)0x5678, lv_event_dsc_get_user_data(dsc));

    lv_obj_delete(obj);
}

/* Test removing event by index */
void test_event_remove_by_index(void)
{
    /* Minimal test setup with static storage */
    lv_event_list_t list = {0};

    /* Add test events */
    TEST_ASSERT_NOT_NULL(lv_event_add(&list, NULL, LV_EVENT_VALUE_CHANGED, NULL));
    TEST_ASSERT_NOT_NULL(lv_event_add(&list, NULL, LV_EVENT_CLICKED, NULL));

    /* Test normal removal */
    bool removed = lv_event_remove(&list, 0);
    TEST_ASSERT_TRUE(removed);
    TEST_ASSERT_EQUAL_UINT32(1, lv_event_get_count(&list));

    /* Test removal during traversal */
    lv_event_add_flag(&list, LV_EVENT_FLAG_TRAVERSING);
    removed = lv_event_remove(&list, 0);
    TEST_ASSERT_TRUE_MESSAGE(removed, "Failed to remove event during traversal");
    TEST_ASSERT_TRUE_MESSAGE(lv_event_has_flag(&list, LV_EVENT_FLAG_REMOVED),
                             "REMOVED flag not set after traversal removal");

    /* Verify the entry was set to NULL */
    lv_event_dsc_t * dsc = lv_event_get_dsc(&list, 0);
    TEST_ASSERT_NULL_MESSAGE(dsc, "Event descriptor not set to NULL after removal");

    lv_event_remove_flag(&list, LV_EVENT_FLAG_TRAVERSING);

    /* Test invalid index */
    removed = lv_event_remove(&list, 100);
    TEST_ASSERT_FALSE(removed);

    /* Cleanup */
    for(uint32_t i = 0; i < lv_event_get_count(&list); i++) {
        lv_event_dsc_t * dsc2 = lv_event_get_dsc(&list, i);
        if(dsc2) lv_free(dsc2);
    }
    lv_array_deinit(&list.array);
}

/* Test event control functions */
void test_event_control(void)
{
    lv_event_t e = {0};

    /* Test stop bubbling */
    TEST_ASSERT_EQUAL_UINT8(0, e.stop_bubbling);
    lv_event_stop_bubbling(&e);
    TEST_ASSERT_EQUAL_UINT8(1, e.stop_bubbling);

    /* Test stop processing */
    TEST_ASSERT_EQUAL_UINT8(0, e.stop_processing);
    lv_event_stop_processing(&e);
    TEST_ASSERT_EQUAL_UINT8(1, e.stop_processing);

    /* Test event ID registration */
    uint32_t id1 = lv_event_register_id();
    uint32_t id2 = lv_event_register_id();
    TEST_ASSERT_NOT_EQUAL_UINT32(id1, id2);
    TEST_ASSERT_TRUE(id2 > id1);  /* Just verify IDs are increasing */
}

/* Test removing all events while TRAVERSING flag is set */
void test_event_remove_all_with_traversing(void)
{
    /* Minimal test setup with static storage */
    lv_event_list_t list = {0};

    /* Add test events */
    TEST_ASSERT_NOT_NULL(lv_event_add(&list, NULL, LV_EVENT_VALUE_CHANGED, NULL));
    TEST_ASSERT_NOT_NULL(lv_event_add(&list, NULL, LV_EVENT_CLICKED, NULL));

    /* Test core functionality */
    lv_event_add_flag(&list, LV_EVENT_FLAG_TRAVERSING);
    lv_event_remove_all(&list);

    /* Key verifications */
    TEST_ASSERT_TRUE(lv_event_has_flag(&list, LV_EVENT_FLAG_REMOVED));
    TEST_ASSERT_NULL(lv_event_get_dsc(&list, 0));
    TEST_ASSERT_NULL(lv_event_get_dsc(&list, 1));

    /* Test removing dsc from wrong list */
    lv_event_list_t list_b = {0};

    /* Add event to list_b */
    lv_event_dsc_t * dsc_b = lv_event_add(&list_b, NULL, LV_EVENT_PRESSED, NULL);
    TEST_ASSERT_NOT_NULL(dsc_b);

    /* Try to remove list_b's dsc from list_a - should fail */
    bool removed = lv_event_remove_dsc(&list, dsc_b);
    TEST_ASSERT_FALSE(removed);

    /* Cleanup */
    lv_free(dsc_b);
    for(uint32_t i = 0; i < lv_event_get_count(&list); i++) {
        lv_event_dsc_t * dsc = lv_event_get_dsc(&list, i);
        if(dsc) lv_free(dsc);
    }
    lv_event_remove_flag(&list, LV_EVENT_FLAG_TRAVERSING);
    lv_array_deinit(&list.array);
    lv_array_deinit(&list_b.array);
}

/* Test event code to name conversion */
void test_event_code_names(void)
{
    /* Input device events */
    TEST_EVENT_NAME(PRESSED);
    TEST_EVENT_NAME(PRESSING);
    TEST_EVENT_NAME(PRESS_LOST);
    TEST_EVENT_NAME(SHORT_CLICKED);
    TEST_EVENT_NAME(LONG_PRESSED);
    TEST_EVENT_NAME(LONG_PRESSED_REPEAT);
    TEST_EVENT_NAME(CLICKED);
    TEST_EVENT_NAME(RELEASED);
    TEST_EVENT_NAME(SCROLL_BEGIN);
    TEST_EVENT_NAME(SCROLL_THROW_BEGIN);
    TEST_EVENT_NAME(SCROLL_END);
    TEST_EVENT_NAME(SCROLL);
    TEST_EVENT_NAME(GESTURE);
    TEST_EVENT_NAME(KEY);
    TEST_EVENT_NAME(ROTARY);
    TEST_EVENT_NAME(FOCUSED);
    TEST_EVENT_NAME(DEFOCUSED);
    TEST_EVENT_NAME(LEAVE);
    TEST_EVENT_NAME(HIT_TEST);
    TEST_EVENT_NAME(INDEV_RESET);

    /* Drawing events */
    TEST_EVENT_NAME(COVER_CHECK);
    TEST_EVENT_NAME(REFR_EXT_DRAW_SIZE);
    TEST_EVENT_NAME(DRAW_MAIN_BEGIN);
    TEST_EVENT_NAME(DRAW_MAIN);
    TEST_EVENT_NAME(DRAW_MAIN_END);
    TEST_EVENT_NAME(DRAW_POST_BEGIN);
    TEST_EVENT_NAME(DRAW_POST);
    TEST_EVENT_NAME(DRAW_POST_END);
    TEST_EVENT_NAME(DRAW_TASK_ADDED);

    /* Special events */
    TEST_EVENT_NAME(VALUE_CHANGED);
    TEST_EVENT_NAME(INSERT);
    TEST_EVENT_NAME(REFRESH);
    TEST_EVENT_NAME(READY);
    TEST_EVENT_NAME(CANCEL);

    /* Other events */
    TEST_EVENT_NAME(CREATE);
    TEST_EVENT_NAME(DELETE);
    TEST_EVENT_NAME(CHILD_CHANGED);
    TEST_EVENT_NAME(CHILD_CREATED);
    TEST_EVENT_NAME(CHILD_DELETED);
    TEST_EVENT_NAME(SCREEN_UNLOAD_START);
    TEST_EVENT_NAME(SCREEN_LOAD_START);
    TEST_EVENT_NAME(SCREEN_LOADED);
    TEST_EVENT_NAME(SCREEN_UNLOADED);
    TEST_EVENT_NAME(SIZE_CHANGED);
    TEST_EVENT_NAME(STYLE_CHANGED);
    TEST_EVENT_NAME(LAYOUT_CHANGED);
    TEST_EVENT_NAME(GET_SELF_SIZE);

    /* Events of optional LVGL components */
    TEST_EVENT_NAME(INVALIDATE_AREA);
    TEST_EVENT_NAME(RESOLUTION_CHANGED);
    TEST_EVENT_NAME(COLOR_FORMAT_CHANGED);
    TEST_EVENT_NAME(REFR_REQUEST);
    TEST_EVENT_NAME(REFR_START);
    TEST_EVENT_NAME(REFR_READY);
    TEST_EVENT_NAME(RENDER_START);
    TEST_EVENT_NAME(RENDER_READY);
    TEST_EVENT_NAME(FLUSH_START);
    TEST_EVENT_NAME(FLUSH_FINISH);
    TEST_EVENT_NAME(FLUSH_WAIT_START);
    TEST_EVENT_NAME(FLUSH_WAIT_FINISH);
    TEST_EVENT_NAME(VSYNC);

    /* Test unknown event */
    TEST_ASSERT_EQUAL_STRING("EVENT_UNKNOWN", lv_event_get_code_name(0xFFFF));
}

#endif