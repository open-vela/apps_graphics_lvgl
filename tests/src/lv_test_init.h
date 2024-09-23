
#ifndef LV_TEST_INIT_H
#define LV_TEST_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <../lvgl.h>

void lv_test_init(void);
void lv_test_deinit(void);

/*
 * a workaround to set colorformat of display.
 * reinit the buffer of display by colorformat.
 */
void hal_init_colorformat(lv_color_format_t colorformat);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TEST_INIT_H*/
