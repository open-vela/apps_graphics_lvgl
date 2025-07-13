/**
 * @file lv_nuttx_touchscreen.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_nuttx_touchscreen.h"

#if LV_USE_NUTTX

#if LV_USE_NUTTX_TOUCHSCREEN

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/input/touchscreen.h>
#include "../../lvgl_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    /* fd should be defined at the beginning */
    int fd;
    uint8_t maxpoint; /* Maximal point supported by the touchscreen */
    struct touch_sample_s * sample;
    struct touch_sample_s * last_sample;
    bool has_last_sample;
    lv_indev_state_t last_state;
    lv_indev_t * indev_drv;
    struct touch_point_s primary_point; /* Track current active contact point (primary touch in multi-touch scenarios) */
#if LV_USE_GESTURE_RECOGNITION
    lv_indev_gesture_recognizer_t recognizer;
    lv_indev_touch_data_t * touch_data;
    uint8_t active_points; /* Number of currently active touch points */
#endif
} lv_nuttx_touchscreen_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void indev_set_cursor(lv_indev_t * indev, int32_t size);
static void touchscreen_read(lv_indev_t * drv, lv_indev_data_t * data);
static void touchscreen_delete_cb(lv_event_t * e);
static lv_indev_t * touchscreen_init(int fd, uint8_t maxpoint);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_indev_t * lv_nuttx_touchscreen_create(const char * dev_path)
{
    lv_indev_t * indev;
    uint8_t maxpoint;
    int fd;

    LV_ASSERT_NULL(dev_path);
    LV_LOG_USER("touchscreen %s opening", dev_path);
    fd = open(dev_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(fd < 0) {
        LV_LOG_ERROR("cannot open touchscreen device %s, (errno=%d)", dev_path, errno);
        return NULL;
    }

    if(ioctl(fd, TSIOC_GETMAXPOINTS, &maxpoint) < 0) {
        LV_LOG_ERROR("get touch maxpoints failed (errno=%d)", errno);
        close(fd);
        return NULL;
    }

    if(maxpoint == 0) {
        LV_LOG_ERROR("touchscreen %s unsupported maxpoint %d", dev_path, maxpoint);
        close(fd);
        return NULL;
    }

    LV_LOG_USER("touchscreen %s open success, maxpoint %d", dev_path, maxpoint);

    indev = touchscreen_init(fd, maxpoint);

    if(indev == NULL) {
        close(fd);
        return NULL;
    }

    indev_set_cursor(indev, LV_NUTTX_TOUCHSCREEN_CURSOR_SIZE);

    return indev;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void indev_set_cursor(lv_indev_t * indev, int32_t size)
{
    lv_obj_t * cursor_obj = lv_indev_get_cursor(indev);
    if(size <= 0) {
        if(cursor_obj) {
            lv_obj_delete(cursor_obj);
            lv_indev_set_cursor(indev, NULL);
        }
    }
    else {
        if(cursor_obj == NULL) {
            cursor_obj = lv_obj_create(lv_layer_sys());
            lv_obj_remove_style_all(cursor_obj);
            lv_obj_set_style_radius(cursor_obj, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(cursor_obj, LV_OPA_50, 0);
            lv_obj_set_style_bg_color(cursor_obj, lv_color_black(), 0);
            lv_obj_set_style_border_width(cursor_obj, 2, 0);
            lv_obj_set_style_border_color(cursor_obj, lv_palette_main(LV_PALETTE_GREY), 0);
        }
        lv_obj_set_size(cursor_obj, size, size);
        lv_obj_set_style_translate_x(cursor_obj, -size / 2, 0);
        lv_obj_set_style_translate_y(cursor_obj, -size / 2, 0);
        lv_indev_set_cursor(indev, cursor_obj);
    }
}

static void process_single_touch(lv_indev_t * drv,
                                 lv_indev_data_t * data,
                                 struct touch_sample_s * sample)
{
    lv_nuttx_touchscreen_t * touchscreen = drv->driver_data;
    uint8_t touch_flags = sample->point[0].flags;

    if(touch_flags & (TOUCH_DOWN | TOUCH_MOVE)) {
        lv_display_t * disp = lv_indev_get_display(drv);
        int32_t hor_max = lv_display_get_horizontal_resolution(disp) - 1;
        int32_t ver_max = lv_display_get_vertical_resolution(disp) - 1;

        data->point.x = LV_CLAMP(0, sample->point[0].x, hor_max);
        data->point.y = LV_CLAMP(0, sample->point[0].y, ver_max);
        touchscreen->last_state = LV_INDEV_STATE_PRESSED;

        if(touch_flags & TOUCH_DOWN) {
            touchscreen->primary_point.id = sample->point[0].id;
            touchscreen->primary_point.x = data->point.x;
            touchscreen->primary_point.y = data->point.y;
        }
    }
    else if(touch_flags & TOUCH_UP) {
        touchscreen->primary_point.id = UINT8_MAX;
        touchscreen->last_state = LV_INDEV_STATE_RELEASED;
    }
}

#if LV_USE_GESTURE_RECOGNITION
static void touchscreen_update_primary_point(struct touch_sample_s * sample,
                                             lv_nuttx_touchscreen_t * touchscreen,
                                             int32_t hor_max, int32_t ver_max)
{
    int primary_index = -1;

    for(int i = 0; i < sample->npoints; i++) {
        uint8_t touch_flags = sample->point[i].flags;

        if(touchscreen->primary_point.id == sample->point[i].id) {
            if(touch_flags & (TOUCH_DOWN | TOUCH_MOVE)) {
                primary_index = i;
                break;
            }
            else if((touch_flags & TOUCH_UP) && primary_index >= 0) {
                break;
            }
        }
        else if((touch_flags & (TOUCH_DOWN | TOUCH_MOVE)) && primary_index < 0) {
            primary_index = i;
        }
    }

    if(primary_index != -1) {
        touchscreen->primary_point.id = sample->point[primary_index].id;
        touchscreen->primary_point.x =
            LV_CLAMP(0, sample->point[primary_index].x, hor_max);
        touchscreen->primary_point.y =
            LV_CLAMP(0, sample->point[primary_index].y, ver_max);
    }
    else {
        touchscreen->primary_point.id = UINT8_MAX;
    }
}

static void process_multi_touch(lv_indev_t * indev,
                                lv_indev_data_t * data,
                                struct touch_sample_s * sample)
{
    lv_nuttx_touchscreen_t * touchscreen = lv_indev_get_driver_data(indev);
    lv_indev_touch_data_t * touch_data_tmp = touchscreen->touch_data;
    lv_display_t * disp = lv_indev_get_display(indev);
    int32_t hor_max = lv_display_get_horizontal_resolution(disp) - 1;
    int32_t ver_max = lv_display_get_vertical_resolution(disp) - 1;
    uint16_t finger_release_cnt = 0;

    for(int i = 0; i < sample->npoints; i++) {
        touch_data_tmp->id = sample->point[i].id;
        uint8_t touch_flags = sample->point[i].flags;
        if(touch_flags & (TOUCH_DOWN | TOUCH_MOVE)) {
            touch_data_tmp->point.x = LV_CLAMP(0, sample->point[i].x, hor_max);
            touch_data_tmp->point.y = LV_CLAMP(0, sample->point[i].y, ver_max);
            touch_data_tmp->state = LV_INDEV_STATE_PRESSED;
            touch_data_tmp++;
        }
        else if(touch_flags & TOUCH_UP) {
            touch_data_tmp->state = LV_INDEV_STATE_RELEASED;
            finger_release_cnt++;
            touch_data_tmp++;
        }
    }

    lv_indev_gesture_detect_pinch(&touchscreen->recognizer,
                                  touchscreen->touch_data,
                                  touchscreen->active_points);
    lv_indev_set_gesture_data(data, &touchscreen->recognizer);

    data->update_primary_point = false;
    if(finger_release_cnt == touchscreen->active_points) {
        data->state = LV_INDEV_STATE_RELEASED;
        touchscreen->primary_point.id = UINT8_MAX;
    }
    else {
        data->state = LV_INDEV_STATE_PRESSED;
        if(finger_release_cnt > 0) {
            data->update_primary_point = true;
        }

        touchscreen_update_primary_point(sample, touchscreen, hor_max, ver_max);
        data->point.x = touchscreen->primary_point.x;
        data->point.y = touchscreen->primary_point.y;
    }
}
#endif

static void conv_touch(lv_indev_t * drv,
                       lv_indev_data_t * data,
                       struct touch_sample_s * sample)
{
#if LV_USE_GESTURE_RECOGNITION
    if(sample->npoints > 1) {
        lv_nuttx_touchscreen_t * touchscreen = drv->driver_data;
        touchscreen->active_points = 0;

        for(int i = 0; i < sample->npoints; i++) {
            if((sample->point[i].flags)) {
                ++touchscreen->active_points;
            }
        }

        if(touchscreen->active_points > 1) {
            process_multi_touch(drv, data, sample);
            return;
        }
    }

#endif
    process_single_touch(drv, data, sample);
}
static bool touchscreen_read_sample(lv_nuttx_touchscreen_t * touchscreen)
{
    int nbytes = read(touchscreen->fd,
                      touchscreen->sample,
                      SIZEOF_TOUCH_SAMPLE_S(touchscreen->maxpoint));
    return nbytes == SIZEOF_TOUCH_SAMPLE_S(touchscreen->maxpoint);
}

static void touchscreen_read(lv_indev_t * drv, lv_indev_data_t * data)
{
    lv_nuttx_touchscreen_t * touchscreen = drv->driver_data;

    /* Note: Since it is necessary to avoid multi-processing click events
    * caused by redundant continue_reading, a two-unit sample sliding window
    * algorithm is used here. continue_reading is only activated when there
    * are two points in the window.
    */

    /* If has last sample, use it first */
    if(touchscreen->has_last_sample) {
        conv_touch(drv, data, touchscreen->last_sample);
    }
    else {
        /* Read first sample */
        if(!touchscreen_read_sample(touchscreen)) {
            /* No sample available, return last state */
#if LV_USE_GESTURE_RECOGNITION
            if(touchscreen->active_points > 1) {
                return;
            }
#endif
            data->state = touchscreen->last_state;
            return;
        }

        conv_touch(drv, data, touchscreen->sample);
    }

    /* Try to read next sample */
    if(touchscreen_read_sample(touchscreen)) {
        /* Save last sample and let lvgl continue reading */
        lv_memcpy(touchscreen->last_sample, touchscreen->sample,
                  SIZEOF_TOUCH_SAMPLE_S(touchscreen->maxpoint));
        touchscreen->has_last_sample = true;
        data->continue_reading = true;
    }
    else {
        /* No more sample available, clear last sample flag */
        touchscreen->has_last_sample = false;
    }

#if LV_USE_GESTURE_RECOGNITION
    if(touchscreen->active_points > 1) {
        return;
    }
#endif
    data->state = touchscreen->last_state;
}

static void touchscreen_delete_cb(lv_event_t * e)
{
    lv_indev_t * indev = (lv_indev_t *) lv_event_get_user_data(e);
    lv_nuttx_touchscreen_t * touchscreen = lv_indev_get_driver_data(indev);
    if(touchscreen) {
        if(touchscreen->sample) {
            lv_free(touchscreen->sample);
            touchscreen->sample = NULL;
        }

        if(touchscreen->last_sample) {
            lv_free(touchscreen->last_sample);
            touchscreen->last_sample = NULL;
        }

#if LV_USE_GESTURE_RECOGNITION
        if(touchscreen->touch_data) {
            lv_free(touchscreen->touch_data);
            touchscreen->touch_data = NULL;
        }
#endif

        lv_indev_set_driver_data(indev, NULL);
        lv_indev_set_read_cb(indev, NULL);
        indev_set_cursor(indev, -1);
        if(touchscreen->fd >= 0) {
            close(touchscreen->fd);
            touchscreen->fd = -1;
        }
        lv_free(touchscreen);
        LV_LOG_USER("done");
    }
}

static lv_indev_t * touchscreen_init(int fd, uint8_t maxpoint)
{
    lv_nuttx_touchscreen_t * touchscreen;
    lv_indev_t * indev = NULL;

    touchscreen = lv_malloc_zeroed(sizeof(lv_nuttx_touchscreen_t));
    if(touchscreen == NULL) {
        LV_LOG_ERROR("touchscreen_s malloc failed");
        return NULL;
    }

    touchscreen->fd = fd;
    touchscreen->maxpoint = maxpoint;
    touchscreen->last_state = LV_INDEV_STATE_RELEASED;
    touchscreen->indev_drv = indev = lv_indev_create();
    touchscreen->sample =
        lv_malloc_zeroed(SIZEOF_TOUCH_SAMPLE_S(touchscreen->maxpoint));
    LV_ASSERT_MALLOC(touchscreen->sample);

    touchscreen->last_sample =
        lv_malloc_zeroed(SIZEOF_TOUCH_SAMPLE_S(touchscreen->maxpoint));
    LV_ASSERT_MALLOC(touchscreen->last_sample);

#if LV_USE_GESTURE_RECOGNITION
    touchscreen->touch_data =
        lv_malloc_zeroed(sizeof(lv_indev_touch_data_t) * touchscreen->maxpoint);
    LV_ASSERT_MALLOC(touchscreen->touch_data);
#endif

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchscreen_read);
    lv_indev_set_driver_data(indev, touchscreen);
    lv_indev_add_event_cb(indev, touchscreen_delete_cb, LV_EVENT_DELETE, indev);
    return indev;
}

#endif /*LV_USE_NUTTX_TOUCHSCREEN*/

#endif /* LV_USE_NUTTX*/
