/**
 * @file lv_sysmon.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_sysmon.h"

#if LV_USE_SYSMON

#include "../../core/lv_global.h"
#include "../../misc/lv_async.h"
#include "../../stdlib/lv_string.h"
#include "../../widgets/label/lv_label.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_sysmon_class)

#define SYSMON_REFR_PERIOD_DEF 300 /* ms */

#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR && !LV_PERF_MONITOR_SERVICE_ONLY
    #define sysmon_perf LV_GLOBAL_DEFAULT()->sysmon_perf.object
    #define _USE_PERF_MONITOR   1
#else
    #define _USE_PERF_MONITOR   0
#endif

#if defined(LV_USE_MEM_MONITOR) && LV_USE_MEM_MONITOR
    #define sysmon_mem LV_GLOBAL_DEFAULT()->sysmon_mem
    #define _USE_MEM_MONITOR   1
#else
    #define _USE_MEM_MONITOR   0
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

#if _USE_PERF_MONITOR
    static void perf_update_timer_cb(lv_timer_t * t);
    static void perf_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

#if _USE_MEM_MONITOR
    static void mem_update_timer_cb(lv_timer_t * t);
    static void mem_observer_cb(lv_observer_t * observer, lv_subject_t * subject);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void _lv_sysmon_builtin_init(void)
{
#if defined(LV_USE_PERF_MONITOR) && LV_USE_PERF_MONITOR
    _lv_sysmon_perf_builtin_init();
#endif
#if _USE_PERF_MONITOR
    sysmon_perf.backend = lv_sysmon_perf_create("lv_sysmon_builtin", 0, 0);
    LV_ASSERT_NULL(sysmon_perf.backend);

    const lv_sysmon_perf_data_t * data = lv_sysmon_perf_get_data(sysmon_perf.backend);
    const lv_sysmon_perf_info_t * info = &data->overall;
    lv_subject_init_pointer(&sysmon_perf.common.subject, (void *)info);

    sysmon_perf.common.timer = lv_timer_create(perf_update_timer_cb, SYSMON_REFR_PERIOD_DEF, NULL);
#endif

#if _USE_MEM_MONITOR
    static lv_mem_monitor_t mem_info;
    lv_subject_init_pointer(&sysmon_mem.subject, &mem_info);
    sysmon_mem.timer = lv_timer_create(mem_update_timer_cb, SYSMON_REFR_PERIOD_DEF, &mem_info);
#endif
}

void _lv_sysmon_builtin_deinit(void)
{
#if _USE_PERF_MONITOR
    lv_timer_delete(sysmon_perf.common.timer);
    lv_sysmon_perf_destroy(sysmon_perf.backend);
#endif

#if _USE_MEM_MONITOR
    lv_timer_delete(sysmon_mem.timer);
#endif
}

lv_obj_t * lv_sysmon_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * label = lv_label_create(parent);
    lv_obj_set_style_bg_opa(label, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_pad_all(label, 3, 0);
    lv_label_set_text(label, "?");
    return label;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if _USE_PERF_MONITOR

static void perf_update_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    /*Wait for a display*/
    if(!sysmon_perf.common.inited && lv_display_get_default()) {
        lv_obj_t * obj1 = lv_sysmon_create(lv_layer_sys());
        lv_obj_align(obj1, LV_USE_PERF_MONITOR_POS, 0, 0);
        lv_subject_add_observer_obj(&sysmon_perf.common.subject, perf_observer_cb, obj1, NULL);
#if LV_USE_PERF_MONITOR_LOG_MODE
        lv_obj_add_flag(obj1, LV_OBJ_FLAG_HIDDEN);
#endif
        lv_sysmon_perf_start(sysmon_perf.backend, true);
        sysmon_perf.common.inited = true;
    }

    if(!sysmon_perf.common.inited) return;

    const lv_sysmon_perf_data_t * data = lv_sysmon_perf_get_data(sysmon_perf.backend);
    const lv_sysmon_perf_info_t * info = &data->overall;
    sysmon_perf.run_cnt++;
    sysmon_perf.cpu_avg_total = ((sysmon_perf.cpu_avg_total * (sysmon_perf.run_cnt - 1)) +
                                 info->calculated.cpu) / sysmon_perf.run_cnt;
    sysmon_perf.fps_avg_total = ((sysmon_perf.fps_avg_total * (sysmon_perf.run_cnt - 1)) +
                                 info->calculated.fps) / sysmon_perf.run_cnt;
    lv_subject_set_pointer(&sysmon_perf.common.subject, (void *)info);
    lv_sysmon_perf_reset_data(sysmon_perf.backend, LV_SYSMON_PERF_TYPE_OVERALL);
}

static void perf_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * label = lv_observer_get_target(observer);
    const lv_sysmon_perf_info_t * perf = lv_subject_get_pointer(subject);
    LV_ASSERT_NULL(perf);

#if LV_USE_PERF_MONITOR_LOG_MODE
    LV_UNUSED(label);
    LV_LOG("sysmon: "
           "%" LV_PRFv32(".2f") " FPS (refr_cnt: %" LV_PRIu32 " | redraw_cnt: %" LV_PRIu32" | refr_rate: %" LV_PRFv32(".2f") "), "
           "refr %" LV_PRFv32(".2f") "ms (render %" LV_PRFv32(".2f") "ms | flush %" LV_PRFv32(".2f") "ms), GPU time %"
           LV_PRFv32(".2f") "ms, "
           "CPU %" LV_PRIu32 "%%, GPU %" LV_PRFv32(".2f") "%%\n",
           perf->calculated.fps, perf->measured.refr_cnt, perf->measured.render_cnt, perf->calculated.fps_refr,
           perf->calculated.refr_avg_time, perf->calculated.render_avg_time, perf->calculated.flush_avg_time,
           perf->calculated.gpu_run_avg_time, perf->calculated.cpu,
           perf->calculated.render_avg_time > 0 ? perf->calculated.gpu_run_avg_time /
           perf->calculated.render_avg_time * 100 : 0);
#else
    lv_label_set_text_fmt(
        label,
        "%" LV_PRFv32(".2f") "/" "%" LV_PRFv32(".2f") " FPS, %" LV_PRIu32 "%% CPU\n"
        "%" LV_PRFv32(".2f")" ms (%" LV_PRFv32(".2f")" | %" LV_PRFv32(".2f")")"
        "%" LV_PRFv32(".2f")" ms (%" LV_PRFv32(".2f"),
        perf->calculated.fps, perf->calculated.fps_refr, perf->calculated.cpu,
        perf->calculated.render_avg_time + perf->calculated.flush_avg_time,
        perf->calculated.render_avg_time, perf->calculated.flush_avg_time,
        perf->calculated.gpu_run_avg_time,
        perf->calculated.render_avg_time > 0 ? perf->calculated.gpu_run_avg_time /
        perf->calculated.render_avg_time * 100 : 0
    );
#endif /*LV_USE_PERF_MONITOR_LOG_MODE*/
}

#endif

#if _USE_MEM_MONITOR

static void mem_update_timer_cb(lv_timer_t * t)
{
    /*Wait for a display*/
    if(!sysmon_mem.inited && lv_display_get_default()) {
        lv_obj_t * obj2 = lv_sysmon_create(lv_layer_sys());
        lv_obj_align(obj2, LV_USE_MEM_MONITOR_POS, 0, 0);
        lv_subject_add_observer_obj(&sysmon_mem.subject, mem_observer_cb, obj2, NULL);
        sysmon_mem.inited = true;
    }

    if(!sysmon_mem.inited) return;

    lv_mem_monitor_t * mem_mon = lv_timer_get_user_data(t);
    lv_mem_monitor(mem_mon);
    lv_subject_set_pointer(&sysmon_mem.subject, mem_mon);
}

static void mem_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * label = lv_observer_get_target(observer);
    const lv_mem_monitor_t * mon = lv_subject_get_pointer(subject);

    size_t used_size = mon->total_size - mon->free_size;;
    size_t used_kb = used_size / 1024;
    size_t used_kb_tenth = (used_size - (used_kb * 1024)) / 102;
    size_t max_used_kb = mon->max_used / 1024;
    size_t max_used_kb_tenth = (mon->max_used - (max_used_kb * 1024)) / 102;
    lv_label_set_text_fmt(label,
                          "%zu.%zu kB (%d%%)\n"
                          "%zu.%zu kB max, %d%% frag.",
                          used_kb, used_kb_tenth, mon->used_pct,
                          max_used_kb, max_used_kb_tenth,
                          mon->frag_pct);
}

#endif

#endif /*LV_USE_SYSMON*/
