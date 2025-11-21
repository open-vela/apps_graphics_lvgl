#if LV_BUILD_TEST
#include "../lvgl.h"

#include "unity/unity.h"
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static lv_remote_ctrl_ctx_t * g_ctx = NULL;

#define _draw_info LV_GLOBAL_DEFAULT()->draw_info

static void dummy_print_func_cb(const char * format, ...)
{
    TEST_ASSERT_NOT_NULL(format);
}

void setUp(void)
{
    g_ctx = lv_remote_ctrl_create();
    TEST_ASSERT_NOT_NULL(g_ctx);
    lv_remote_ctrl_show_help("ctrl", dummy_print_func_cb);
}

void tearDown(void)
{
    if(g_ctx) {
        lv_remote_ctrl_destroy(g_ctx);
        g_ctx = NULL;
    }
}

void test_remote_ctrl_args_init(void)
{
    static const char * argv_normal[] = {
        "--argv0",
        "--argv1",
        "--argv2",
        "--argv3",
    };

    lv_remote_ctrl_args_t args;
    lv_result_t res = lv_remote_ctrl_args_init(&args, ARRAY_SIZE(argv_normal), argv_normal);
    TEST_ASSERT_EQUAL(LV_RESULT_OK, res);
    TEST_ASSERT_EQUAL_INT(ARRAY_SIZE(argv_normal), args.argc);

    for(int i = 0; i < args.argc; i++) {
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "--argv%d", i);
        TEST_ASSERT_EQUAL_STRING(buf, &args.argv_buf[args.argv_offsets[i]]);
    }

    static const char * argv_null[] = {
        "--argv0",
        NULL,
        "--argv2",
    };

    /* Test zero argc, should fail */
    res = lv_remote_ctrl_args_init(&args, 0, argv_null);
    TEST_ASSERT_NOT_EQUAL(LV_RESULT_OK, res);

    /* Test NULL argv, should fail */
    res = lv_remote_ctrl_args_init(&args, ARRAY_SIZE(argv_null), argv_null);
    TEST_ASSERT_NOT_EQUAL(LV_RESULT_OK, res);

    /* Test over buffer length, should fail */
    static const char * argv_over_buf_len[] = {
        "--argv0-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
        "--argv1-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
        "--argv2-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
        "--argv3-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
        "--argv4-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
        "--argv5-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
        "--argv6-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789-0123456789",
    };

    res = lv_remote_ctrl_args_init(&args, ARRAY_SIZE(argv_over_buf_len), argv_over_buf_len);
    TEST_ASSERT_NOT_EQUAL(LV_RESULT_OK, res);

    /* Test over LV_REMOTE_CTRL_ARGC_MAX, should fail */
    static const char * argv_over_argc_max[] = {
        "--argv0",
        "--argv1",
        "--argv2",
        "--argv3",
        "--argv4",
        "--argv5",
        "--argv6",
        "--argv7",
        "--argv8",
        "--argv9",
        "--argv10",
        "--argv11",
        "--argv12",
        "--argv13",
        "--argv14",
        "--argv15",
        "--argv16",
    };

    res = lv_remote_ctrl_args_init(&args, ARRAY_SIZE(argv_over_argc_max), argv_over_argc_max);
    TEST_ASSERT_NOT_EQUAL(LV_RESULT_OK, res);
}

static void execute_args(int argc, const char * argv[], lv_result_t expected_res)
{
    lv_remote_ctrl_args_t args;
    lv_result_t res = lv_remote_ctrl_args_init(&args, argc, argv);
    TEST_ASSERT_EQUAL(LV_RESULT_OK, res);

    res = lv_remote_ctrl_execute(g_ctx, &args);
    TEST_ASSERT_EQUAL(expected_res, res);
}

static void test_invalid_arg(const char * cmd)
{
    const char * argv_invalid[] = {
        cmd,
        "--invalid-arg",
    };
    execute_args(ARRAY_SIZE(argv_invalid), argv_invalid, LV_RESULT_INVALID);
}

void test_remote_ctrl_anim(void)
{
    /* Test invalid args */
    test_invalid_arg("anim");

    int var = 0;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &var);
    lv_anim_t * a_started = lv_anim_start(&a);
    TEST_ASSERT_NOT_NULL(a_started);
    TEST_ASSERT_EQUAL_PTR(a_started, lv_anim_get(&var, NULL));

    /* Test dump specific anim */
    char a_started_buf[32];
    lv_snprintf(a_started_buf, sizeof(a_started_buf), "%p", a_started);
    const char * argv_dump_anim[] = {
        "anim",
        "--anim", a_started_buf,
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_anim), argv_dump_anim, LV_RESULT_OK);

    /* Test set anim timer period */
    const char * argv_period[] = {
        "anim",
        "--period", "1000",
    };
    execute_args(ARRAY_SIZE(argv_period), argv_period, LV_RESULT_OK);

    lv_timer_t * timer = lv_anim_get_timer();
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_EQUAL_UINT32(1000, timer->period);

    /* Test anim refresh */
    const char * argv_refresh[] = {
        "anim",
        "--refresh",
    };
    execute_args(ARRAY_SIZE(argv_refresh), argv_refresh, LV_RESULT_OK);

    /* Test set anim duration */
    const char * argv_duration[] = {
        "anim",
        "--anim", a_started_buf,
        "--duration", "123",
    };
    execute_args(ARRAY_SIZE(argv_duration), argv_duration, LV_RESULT_OK);
    TEST_ASSERT_EQUAL_UINT32(123, a_started->duration);

    /* Test delele anim */
    const char * argv_del_empty[] = {
        "anim",
        "--del",
    };
    execute_args(ARRAY_SIZE(argv_del_empty), argv_del_empty, LV_RESULT_INVALID);

    const char * argv_del[] = {
        "anim",
        "--anim", a_started_buf,
        "--del",
    };
    execute_args(ARRAY_SIZE(argv_del), argv_del, LV_RESULT_OK);
    TEST_ASSERT_NULL(lv_anim_get(&a, NULL));

    const char * argv_del_all[] = {
        "anim",
        "--del-all",
    };
    execute_args(ARRAY_SIZE(argv_del_all), argv_del_all, LV_RESULT_OK);
}

void test_remote_ctrl_disp(void)
{
    /* Test invalid args */
    test_invalid_arg("disp");

    lv_display_t * disp = lv_display_get_default();
    TEST_ASSERT_NOT_NULL(disp);

    /* Test dump all displays */
    static const char * argv_dump_all[] = {
        "disp",
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_all), argv_dump_all, LV_RESULT_OK);

    /* Test dump specific display */
    char disp_buf[32];
    lv_snprintf(disp_buf, sizeof(disp_buf), "%p", disp);
    const char * argv_dump_disp[] = {
        "disp",
        "--disp", disp_buf,
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_disp), argv_dump_disp, LV_RESULT_OK);

    /* Test set display rotation */
    for(int i = 0; i < 5; i++) {
        static const lv_display_rotation_t rotations[] = {
            LV_DISPLAY_ROTATION_0,
            LV_DISPLAY_ROTATION_90,
            LV_DISPLAY_ROTATION_180,
            LV_DISPLAY_ROTATION_270,
            LV_DISPLAY_ROTATION_0, /* restore rotation */
        };

        static const char * rotation_str[] = { "0", "90", "180", "270", "0" };

        const char * argv_rotation[] = {
            "disp",
            "--disp", disp_buf,
            "--rotation", rotation_str[i],
        };
        execute_args(ARRAY_SIZE(argv_rotation), argv_rotation, LV_RESULT_OK);
        TEST_ASSERT_EQUAL(rotations[i], lv_display_get_rotation(disp));
    }

    /* Test invalid rotation value */
    const char * argv_invalid_rotation[] = {
        "disp",
        "--disp", disp_buf,
        "--rotation", "45",
    };
    execute_args(ARRAY_SIZE(argv_invalid_rotation), argv_invalid_rotation, LV_RESULT_INVALID);
}

void test_remote_ctrl_draw(void)
{
    /* Test invalid args */
    test_invalid_arg("draw");

    lv_draw_unit_t * draw_unit = _draw_info.unit_head;
    TEST_ASSERT_NOT_NULL(draw_unit);

    const char * argv_dump_null[] = {
        "draw",
        "--unit", "0",
    };
    execute_args(ARRAY_SIZE(argv_dump_null), argv_dump_null, LV_RESULT_INVALID);

    /* Test dump all draw units */
    static const char * argv_dump_all[] = {
        "draw",
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_all), argv_dump_all, LV_RESULT_OK);

    /* Test dump specific draw unit */
    char draw_unit_buf[32];
    lv_snprintf(draw_unit_buf, sizeof(draw_unit_buf), "%p", draw_unit);
    const char * argv_dump_draw[] = {
        "draw",
        "--unit", draw_unit_buf,
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_draw), argv_dump_draw, LV_RESULT_OK);

    /* Test hook draw unit */
    const char * argv_evaluate_task_mask[] = {
        "draw",
        "--unit", draw_unit_buf,
        "--evaluate-task-mask", "1",
    };
    execute_args(ARRAY_SIZE(argv_evaluate_task_mask), argv_evaluate_task_mask, LV_RESULT_OK);

    const char * argv_hook[] = {
        "draw",
        "--unit", draw_unit_buf,
        "--hook", "1",
    };
    execute_args(ARRAY_SIZE(argv_hook), argv_hook, LV_RESULT_OK);
    execute_args(ARRAY_SIZE(argv_hook), argv_hook, LV_RESULT_INVALID);

    /* redraw */
    lv_refr_now(NULL);

    /* Test unhook draw unit */
    const char * argv_unhook[] = {
        "draw",
        "--unit", draw_unit_buf,
        "--hook", "0",
    };
    execute_args(ARRAY_SIZE(argv_unhook), argv_unhook, LV_RESULT_OK);
    execute_args(ARRAY_SIZE(argv_unhook), argv_unhook, LV_RESULT_INVALID);

    /* redraw */
    lv_refr_now(NULL);

    /* Test send event to draw unit, send LV_EVENT_CANCEL to VG-Lite */
    const char * argv_send_event[] = {
        "draw",
        "--unit-name", "VG_LITE",
        "--send-event", "34",
    };
    execute_args(ARRAY_SIZE(argv_send_event), argv_send_event, LV_RESULT_OK);

    const char * argv_send_event_invalid[] = {
        "draw",
        "--send-event", "100",
    };
    execute_args(ARRAY_SIZE(argv_send_event_invalid), argv_send_event_invalid, LV_RESULT_INVALID);
}

void test_remote_ctrl_indev(void)
{
    /* Test invalid args */
    test_invalid_arg("indev");

    lv_indev_t * indev = lv_indev_get_next(NULL);
    while(indev) {
        if(lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            break;
        }
        indev = lv_indev_get_next(indev);
    }
    TEST_ASSERT_NOT_NULL(indev);

    /* Test dump all indevs */
    static const char * argv_dump_all[] = {
        "indev",
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_all), argv_dump_all, LV_RESULT_OK);

    /* Test dump specific indev */
    char indev_buf[32];
    lv_snprintf(indev_buf, sizeof(indev_buf), "%p", indev);
    const char * argv_dump_indev[] = {
        "indev",
        "--indev", indev_buf,
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_indev), argv_dump_indev, LV_RESULT_OK);

    /* Test enable/disable indev */
    const char * argv_disable[] = {
        "indev",
        "--indev", indev_buf,
        "--enable", "0",
    };
    execute_args(ARRAY_SIZE(argv_disable), argv_disable, LV_RESULT_OK);

    const char * argv_enable[] = {
        "indev",
        "--indev", indev_buf,
        "--enable", "1",
    };
    execute_args(ARRAY_SIZE(argv_enable), argv_enable, LV_RESULT_OK);

    /* Test cursor size */
    const char * argv_cursor[] = {
        "indev",
        "--indev", indev_buf,
        "--cursor-size", "20",
    };
    execute_args(ARRAY_SIZE(argv_cursor), argv_cursor, LV_RESULT_OK);
    lv_obj_t * cursor_obj = lv_indev_get_cursor(indev);
    TEST_ASSERT_EQUAL_INT32(20, lv_obj_get_style_width(cursor_obj, LV_PART_MAIN));
    TEST_ASSERT_EQUAL_INT32(20, lv_obj_get_style_height(cursor_obj, LV_PART_MAIN));
}

void test_remote_ctrl_obj(void)
{
    /* Test invalid args */
    test_invalid_arg("obj");

    /* Create a test object */
    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    TEST_ASSERT_NOT_NULL(obj);
    char obj_buf[32];
    lv_snprintf(obj_buf, sizeof(obj_buf), "%p", obj);

    /* Test dump object info */
    const char * argv_dump[] = {
        "obj",
        "--obj", obj_buf,
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump), argv_dump, LV_RESULT_OK);

    /* Test dump object tree */
    const char * argv_dump_tree[] = {
        "obj",
        "--dump-tree",
    };
    execute_args(ARRAY_SIZE(argv_dump_tree), argv_dump_tree, LV_RESULT_OK);

    /* Test add state */
    const char * argv_add_state[] = {
        "obj",
        "--obj", obj_buf,
        "--add-state", "--state", "0x1",  /* LV_STATE_CHECKED */
    };
    execute_args(ARRAY_SIZE(argv_add_state), argv_add_state, LV_RESULT_OK);
    TEST_ASSERT_TRUE(lv_obj_has_state(obj, LV_STATE_CHECKED));

    /* Test remove state */
    const char * argv_remove_state[] = {
        "obj",
        "--obj", obj_buf,
        "--remove-state", "--state", "0x1",  /* LV_STATE_CHECKED */
    };
    execute_args(ARRAY_SIZE(argv_remove_state), argv_remove_state, LV_RESULT_OK);
    TEST_ASSERT_FALSE(lv_obj_has_state(obj, LV_STATE_CHECKED));

    /* Test add flag */
    const char * argv_add_flag[] = {
        "obj",
        "--obj", obj_buf,
        "--add-flag", "--flag", "0x1",  /* LV_OBJ_FLAG_HIDDEN */
    };
    execute_args(ARRAY_SIZE(argv_add_flag), argv_add_flag, LV_RESULT_OK);
    TEST_ASSERT_TRUE(lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN));

    /* Test remove flag */
    const char * argv_remove_flag[] = {
        "obj",
        "--obj", obj_buf,
        "--remove-flag", "--flag", "0x1",  /* LV_OBJ_FLAG_HIDDEN */
    };
    execute_args(ARRAY_SIZE(argv_remove_flag), argv_remove_flag, LV_RESULT_OK);
    TEST_ASSERT_FALSE(lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN));

    /* Test delete object */
    const char * argv_del[] = {
        "obj",
        "--obj", obj_buf,
        "--del",
    };
    execute_args(ARRAY_SIZE(argv_del), argv_del, LV_RESULT_OK);
}

void test_remote_ctrl_profiler(void)
{
    /* Test invalid args */
    test_invalid_arg("profiler");

    /* Test enable profiler */
    const char * argv_enable[] = {
        "profiler",
        "--enable", "1",
    };
    execute_args(ARRAY_SIZE(argv_enable), argv_enable, LV_RESULT_OK);

    /* Test disable profiler */
    const char * argv_disable[] = {
        "profiler",
        "--enable", "0",
    };
    execute_args(ARRAY_SIZE(argv_disable), argv_disable, LV_RESULT_OK);

    /* Test flush profiler */
    const char * argv_flush[] = {
        "profiler",
        "--flush",
    };
    execute_args(ARRAY_SIZE(argv_flush), argv_flush, LV_RESULT_OK);

    /* Test reset profiler */
    const char * argv_reset[] = {
        "profiler",
        "--reset",
    };
    execute_args(ARRAY_SIZE(argv_reset), argv_reset, LV_RESULT_OK);
}

void test_remote_ctrl_refr(void)
{
    /* Test invalid args */
    test_invalid_arg("refr");

    /* Test enable debug mode */
    const char * argv_debug_enable[] = {
        "refr",
        "--debug", "1",
    };
    execute_args(ARRAY_SIZE(argv_debug_enable), argv_debug_enable, LV_RESULT_OK);

    /* Test disable debug mode */
    const char * argv_debug_disable[] = {
        "refr",
        "--debug", "0",
    };
    execute_args(ARRAY_SIZE(argv_debug_disable), argv_debug_disable, LV_RESULT_OK);

    /* Test refresh now */
    const char * argv_refr_now[] = {
        "refr",
        "--now",
    };
    execute_args(ARRAY_SIZE(argv_refr_now), argv_refr_now, LV_RESULT_OK);
}

static void dummy_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
}

void test_remote_ctrl_timer(void)
{
    /* Test invalid args */
    test_invalid_arg("timer");

    /* Test enable or disable timer */
    const char * argv_zero_disable[] = {
        "timer",
        "--enable", "0",
    };

    /* 0 was not a valid value for enable/disable */
    execute_args(ARRAY_SIZE(argv_zero_disable), argv_zero_disable, LV_RESULT_INVALID);

    const char * argv_disable[] = {
        "timer",
        "--enable", "-1",
    };
    execute_args(ARRAY_SIZE(argv_disable), argv_disable, LV_RESULT_OK);

    const char * argv_enable[] = {
        "timer",
        "--enable", "1",
    };
    execute_args(ARRAY_SIZE(argv_enable), argv_enable, LV_RESULT_OK);

    /* Create a test timer */
    lv_timer_t * timer = lv_timer_create(dummy_timer_cb, 100, NULL);
    TEST_ASSERT_NOT_NULL(timer);

    const char * argv_dump_all[] = {
        "timer",
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump_all), argv_dump_all, LV_RESULT_OK);

    /* Test dump timer info */
    char timer_buf[32];
    lv_snprintf(timer_buf, sizeof(timer_buf), "%p", timer);
    const char * argv_dump[] = {
        "timer",
        "--timer", timer_buf,
        "--dump",
    };
    execute_args(ARRAY_SIZE(argv_dump), argv_dump, LV_RESULT_OK);

    /* Test set timer period */
    const char * argv_period[] = {
        "timer",
        "--timer", timer_buf,
        "--period", "200",
    };
    execute_args(ARRAY_SIZE(argv_period), argv_period, LV_RESULT_OK);
    TEST_ASSERT_EQUAL(200, timer->period);

    /* Test set null timer period */
    const char * argv_null_timer_period[] = {
        "timer",
        "--period", "400",
    };
    execute_args(ARRAY_SIZE(argv_null_timer_period), argv_null_timer_period, LV_RESULT_INVALID);

    /* Should not change period */
    TEST_ASSERT_EQUAL(200, timer->period);

    /* Test pause timer */
    const char * argv_pause[] = {
        "timer",
        "--timer", timer_buf,
        "--pause",
    };
    execute_args(ARRAY_SIZE(argv_pause), argv_pause, LV_RESULT_OK);
    TEST_ASSERT_TRUE(timer->paused);

    /* Test resume timer */
    const char * argv_resume[] = {
        "timer",
        "--timer", timer_buf,
        "--resume",
    };
    execute_args(ARRAY_SIZE(argv_resume), argv_resume, LV_RESULT_OK);
    TEST_ASSERT_FALSE(timer->paused);

    /* Test ready timer */
    const char * argv_ready[] = {
        "timer",
        "--timer", timer_buf,
        "--ready",
    };
    execute_args(ARRAY_SIZE(argv_ready), argv_ready, LV_RESULT_OK);
    TEST_ASSERT_EQUAL(timer->last_run, lv_tick_get() - timer->period - 1);

    /* Test enable hook */
    const char * argv_hook_enable[] = {
        "timer",
        "--timer", timer_buf,
        "--hook", "1",
    };
    execute_args(ARRAY_SIZE(argv_hook_enable), argv_hook_enable, LV_RESULT_OK);
    execute_args(ARRAY_SIZE(argv_hook_enable), argv_hook_enable, LV_RESULT_INVALID);

    lv_timer_handler();

    /* Test disable hook */
    const char * argv_hook_disable[] = {
        "timer",
        "--hook", "0",
    };
    execute_args(ARRAY_SIZE(argv_hook_disable), argv_hook_disable, LV_RESULT_OK);
    execute_args(ARRAY_SIZE(argv_hook_disable), argv_hook_disable, LV_RESULT_INVALID);

    /* Test reset timer */
    const char * argv_reset[] = {
        "timer",
        "--timer", timer_buf,
        "--reset",
    };
    execute_args(ARRAY_SIZE(argv_reset), argv_reset, LV_RESULT_OK);

    /* Test hook timer before delete */
    execute_args(ARRAY_SIZE(argv_hook_enable), argv_hook_enable, LV_RESULT_OK);

    /* Test delete timer */
    const char * argv_del[] = {
        "timer",
        "--timer", timer_buf,
        "--del",
    };
    execute_args(ARRAY_SIZE(argv_del), argv_del, LV_RESULT_OK);
}

void test_remote_ctrl_hello(void)
{
    test_invalid_arg("hello");

    const char * argv_no_args[] = {
        "hello",
    };
    execute_args(ARRAY_SIZE(argv_no_args), argv_no_args, LV_RESULT_OK);

    const char * argv_set_value[] = {
        "hello",
        "--value", "42",
    };
    execute_args(ARRAY_SIZE(argv_set_value), argv_set_value, LV_RESULT_OK);
}

#endif
