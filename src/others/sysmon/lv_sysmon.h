/**
 * @file lv_sysmon.h
 *
 */

#ifndef LV_SYSMON_H
#define LV_SYSMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../misc/lv_types.h"
#include "../../others/observer/lv_observer.h"

#if LV_USE_SYSMON

#if LV_USE_LABEL == 0
#error "lv_sysmon: lv_label is required. Enable it in lv_conf.h (LV_USE_LABEL  1) "
#endif

#if LV_USE_OBSERVER == 0
#error "lv_observer: lv_observer is required. Enable it in lv_conf.h (LV_USE_OBSERVER  1) "
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_subject_t subject;
    lv_timer_t * timer;
    bool inited;
} lv_sysmon_backend_data_t;

#if LV_USE_PERF_MONITOR
struct _lv_sysmon_perf_t;
typedef struct _lv_sysmon_perf_t lv_sysmon_perf_t;

typedef struct {
    struct {
        lv_sysmon_backend_data_t common;
        lv_sysmon_perf_t * backend;
        lv_value_precise_t cpu_avg_total;
        lv_value_precise_t fps_avg_total;
        uint32_t run_cnt;
    } object;

    struct {
        lv_ll_t instances_ll;
        uint32_t refr_start;
        uint32_t render_start;
        bool scrolling;
        bool rendering;
        bool inited;
    } backend;
} lv_sysmon_perf_backend_data_t;

typedef struct {
    struct {
        bool inited;
        uint32_t prev_refr_start;  /* Used for calculating refr_interval_sum */
        uint32_t refr_interval_sum;
        uint32_t refr_elaps_sum;
        uint32_t refr_cnt;
        uint32_t render_elaps_sum; /*Contains the flush time too*/
        uint32_t render_cnt;
        lv_value_precise_t gpu_run_time_sum;
        uint32_t flush_in_render_start;
        uint32_t flush_in_render_elaps_sum;
        uint32_t flush_not_in_render_start;
        uint32_t flush_not_in_render_elaps_sum;
        uint32_t perf_start;
    } measured;

    struct {
        uint32_t duration;
        lv_value_precise_t fps;                   /**< FPS of render */
        lv_value_precise_t fps_refr;              /**< FPS of refresh */
        uint32_t cpu;
        lv_value_precise_t refr_avg_time;
        lv_value_precise_t render_avg_time;       /**< Pure rendering time without flush time*/
        lv_value_precise_t flush_avg_time;        /**< Pure flushing time without rendering time*/
        lv_value_precise_t gpu_run_avg_time;
    } calculated;

} lv_sysmon_perf_info_t;

typedef enum {
    LV_SYSMON_EVENT_TYPE_INVALID = 0,
    LV_SYSMON_EVENT_TYPE_REFR_BEGIN = 1,
    LV_SYSMON_EVENT_TYPE_REFR_END = 2,
    LV_SYSMON_EVENT_TYPE_RENDER_BEGIN = 3,
    LV_SYSMON_EVENT_TYPE_RENDER_END = 4,
    LV_SYSMON_EVENT_TYPE_SCROLL_BEGIN = 5,
    LV_SYSMON_EVENT_TYPE_SCROLL_END = 6,
} lv_sysmon_event_type_t;

typedef struct {
    lv_sysmon_event_type_t type : 8;
    uint64_t timestamp : 56;
} lv_sysmon_event_data_t;

typedef enum {
    LV_SYSMON_PERF_TYPE_NONE = 0,
    LV_SYSMON_PERF_TYPE_OVERALL = 1 << 0,
    LV_SYSMON_PERF_TYPE_SCROLLS = 1 << 1,
    LV_SYSMON_PERF_TYPE_EVENTS = 1 << 2,
    LV_SYSMON_PERF_TYPE_ALL = LV_SYSMON_PERF_TYPE_OVERALL | LV_SYSMON_PERF_TYPE_SCROLLS | LV_SYSMON_PERF_TYPE_EVENTS,
} lv_sysmon_perf_type_t;

typedef struct {
    lv_sysmon_perf_info_t overall;
    lv_circle_buf_t * scrolls;     /**< Circle buf of lv_sysmon_perf_info_t */
    lv_circle_buf_t * events;      /**< Circle buf of lv_sysmon_event_data_t */
} lv_sysmon_perf_data_t;
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a system monitor object.
 * @param parent pointer to an object, it will be the parent of the new system monitor
 * @return       pointer to the new system monitor object
 */
lv_obj_t * lv_sysmon_create(lv_obj_t * parent);

/**
 * Set the refresh period of the system monitor object
 * @param obj    pointer to a system monitor object
 * @param period the refresh period in milliseconds
 */
void lv_sysmon_set_refr_period(lv_obj_t * obj, uint32_t period);

/**
 * Initialize built-in system monitor, such as performance and memory monitor.
 */
void _lv_sysmon_builtin_init(void);

/**
 * DeInitialize built-in system monitor, such as performance and memory monitor.
 */
void _lv_sysmon_builtin_deinit(void);

#if LV_USE_PERF_MONITOR
/**
 * Create a performance monitor instance.
 * @param tag         the tag of the performance monitor
 * @param max_events  the maximum number of events to store, 0 means no event data will be stored
 * @param max_scrolls the maximum number of scrolls to store, 0 means no scroll data will be stored
 * @return            pointer to the new performance monitor instance
 */
lv_sysmon_perf_t * lv_sysmon_perf_create(const char * tag, size_t max_events, size_t max_scrolls);

/**
 * Destroy a performance monitor instance.
 * @param perf pointer to the performance monitor instance
 */
void lv_sysmon_perf_destroy(lv_sysmon_perf_t * perf);

/**
 * Start a performance monitor instance, data will be cleared.
 * @param perf      pointer to the performance monitor instance
 * @param immediate if true, the performance monitor will start immediately, otherwise it will delay start until the first render finished
 * @return          LV_RESULT_OK if the performance monitor instance is started successfully, LV_RESULT_INVALID if the performance monitor instance is failed to start
 */
lv_result_t lv_sysmon_perf_start(lv_sysmon_perf_t * perf, bool immediate);

/**
 * Reset the data of a performance monitor instance.
 * @param perf  pointer to the performance monitor instance
 * @param types the types of performance monitor instance to reset
 */
void lv_sysmon_perf_reset_data(lv_sysmon_perf_t * perf, lv_sysmon_perf_type_t types);

/**
 * Get the data of a performance monitor instance.
 * @param perf pointer to the performance monitor instance
 * @return     pointer to the data of performance
 */
const lv_sysmon_perf_data_t * lv_sysmon_perf_get_data(lv_sysmon_perf_t * perf);

/**
 * Stop a performance monitor instance, data will be reset on next start
 * @param perf pointer to the performance monitor instance
 * @return     pointer to the data of performance
 */
const lv_sysmon_perf_data_t * lv_sysmon_perf_stop(lv_sysmon_perf_t * perf);

/**
 * Generate a trace from a performance monitor instance, call lv_profiler to convert to trace, output to console or file.
 * @param perf pointer to the performance monitor instance
 */
void lv_sysmon_perf_generate_trace(lv_sysmon_perf_t * perf);

/**
 * Event handler for performance monitor.
 * @param e pointer to the event
 */
void lv_sysmon_perf_event(lv_event_t * e);

/**
 * Initialize built-in performance monitor.
 */
void _lv_sysmon_perf_builtin_init(void);
#endif

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_SYSMON*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_SYSMON_H*/
