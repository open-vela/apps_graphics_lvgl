/**
* @file lv_test_assert.c
*
* Copyright 2002-2010 Guillaume Cottenceau.
*
* This software may be freely redistributed under the terms
* of the X11 license.
*
*/

/*********************
 *      INCLUDES
 *********************/
#if LV_BUILD_TEST
#include "../lvgl.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "unity.h"
#include "../../src/libs/lodepng/lodepng.h"

#if LV_USE_LODEPNG == 0
    #error "lodepng is required for screenshot compare. Enable it in lv_conf.h (LV_USE_LODEPNG 1)"
#endif


/*********************
 *      DEFINES
 *********************/

#ifndef REF_IMGS_PATH
#define REF_IMGS_PATH "ref_imgs/"
#endif

#ifndef REF_IMG_TOLERANCE
#define REF_IMG_TOLERANCE 0
#endif

#define ERR_FILE_NOT_FOUND  -1
#define ERR_PNG             -2

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool screenshot_compare(const char * fn_ref, const char * mode, uint8_t tolerance);
static unsigned read_png_file(lv_draw_buf_t ** refr_draw_buf, unsigned * width, unsigned * height,
                              const char * file_name);
static unsigned write_png_file(void * raw_img, uint32_t width, uint32_t height, char * file_name);
static void buf_to_xrgb8888(const uint8_t * buf_in, uint8_t * buf_out, lv_color_format_t cf_in);
static void create_folders_if_needed(const char * path) ;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool lv_test_assert_image_eq(const char * fn_ref)
{
    bool pass;

    lv_obj_t * scr = lv_screen_active();
    lv_obj_invalidate(scr);

    pass = screenshot_compare(fn_ref, "full refresh", REF_IMG_TOLERANCE);
    if(!pass) return false;

    //Software has minor rounding errors when not the whole image is updated
    //so ignore stripe invalidation for now
    //    uint32_t i;
    //    for(i = 0; i < 800; i += 50 ) {
    //        lv_area_t a;
    //        a.y1 = 0;
    //        a.y2 = 479;
    //        a.x1 = i;
    //        a.x2 = i + 12;
    //        lv_obj_invalidate_area(scr, &a);
    //
    //        a.x1 = i + 25;
    //        a.x2 = i + 32;
    //        lv_obj_invalidate_area(scr, &a);
    //    }
    //
    //    pass = screenshot_compare(fn_ref, "vertical stripes", 32);
    //    if(!pass) return false;
    //
    //
    //    for(i = 0; i < 480; i += 40) {
    //        lv_area_t a;
    //        a.x1 = 0;
    //        a.x2 = 799;
    //        a.y1 = i;
    //        a.y2 = i + 9;
    //        lv_obj_invalidate_area(scr, &a);
    //
    //        a.y1 = i + 25;
    //        a.y2 = i + 32;
    //        lv_obj_invalidate_area(scr, &a);
    //    }
    //
    //    pass = screenshot_compare(fn_ref, "horizontal stripes", 32);
    //    if(!pass) return false;

    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static uint8_t screen_buf_xrgb8888[800 * 480 * 4];
/**
 * Compare the content of the frame buffer with a reference image
 * @param fn_ref        reference image name
 * @param mode          arbitrary string to tell more about the compare
 * @return  true: test passed; false: test failed
 */
static bool screenshot_compare(const char * fn_ref, const char * mode, uint8_t tolerance)
{

    char fn_ref_full[256];
    lv_snprintf(fn_ref_full, sizeof(fn_ref_full), "%s%s", REF_IMGS_PATH, fn_ref);

    create_folders_if_needed(fn_ref_full);

    lv_refr_now(NULL);

    extern uint8_t * last_flushed_buf;

    lv_color_format_t cf = lv_display_get_color_format(NULL);
    uint8_t * screen_buf = lv_draw_buf_align(last_flushed_buf, cf);
    buf_to_xrgb8888(screen_buf, screen_buf_xrgb8888, cf);

    lv_draw_buf_t * ref_draw_buf;
    unsigned ref_img_width = 0;
    unsigned ref_img_height = 0;
    unsigned res = read_png_file(&ref_draw_buf, &ref_img_width, &ref_img_height, fn_ref_full);
    if(res) {
        TEST_PRINTF("%s%s", fn_ref_full, " was not found, creating is now from the rendered screen");
        fflush(stderr);
        write_png_file(screen_buf_xrgb8888, 800, 480, fn_ref_full);
        return true;
    }

    bool err = false;
    unsigned x, y;
    for(y = 0; y < ref_img_height; y++) {
        uint8_t * screen_buf_tmp = screen_buf_xrgb8888 + 800 * 4 * y;
        uint8_t * ref_row = (uint8_t *)ref_draw_buf->data + y * ref_draw_buf->header.stride;
        for(x = 0; x < ref_img_width; x++) {
            uint8_t * ptr_ref = &(ref_row[x * 4]);
            uint8_t * ptr_act = &screen_buf_tmp[x * 4];

            if(LV_ABS((int32_t) ptr_act[0] - (int32_t) ptr_ref[0]) > tolerance ||
               LV_ABS((int32_t) ptr_act[1] - (int32_t) ptr_ref[1]) > tolerance ||
               LV_ABS((int32_t) ptr_act[2] - (int32_t) ptr_ref[2]) > tolerance) {
                uint32_t act_px = (ptr_act[2] << 16) + (ptr_act[1] << 8) + (ptr_act[0] << 0);
                uint32_t ref_px = 0;
                memcpy(&ref_px, ptr_ref, 3);
                TEST_PRINTF("\nScreenshot compare error\n"
                            "  - File: %s\n"
                            "  - Mode: %s\n"
                            "  - At x:%d, y:%d.\n"
                            "  - Expected: %X\n"
                            "  - Actual:   %X\n"
                            "  - Tolerance: %d",
                            fn_ref_full, mode,  x, y, ref_px, act_px, tolerance);
                fflush(stderr);
                err = true;
                break;
            }
        }
        if(err) break;
    }

    if(err) {
        char fn_ref_no_ext[128];
        lv_strlcpy(fn_ref_no_ext, fn_ref, sizeof(fn_ref_no_ext));
        fn_ref_no_ext[strlen(fn_ref_no_ext) - 4] = '\0';

        char fn_err_full[256];
        lv_snprintf(fn_err_full, sizeof(fn_err_full), "%s%s_err.png", REF_IMGS_PATH, fn_ref_no_ext);

        write_png_file(screen_buf_xrgb8888, 800, 480, fn_err_full);
    }

    fflush(stdout);
    lv_draw_buf_destroy(ref_draw_buf);
    return !err;

}

static unsigned read_png_file(lv_draw_buf_t ** refr_draw_buf, unsigned * width, unsigned * height,
                              const char * file_name)
{
    unsigned error = lodepng_decode32_file((void *)refr_draw_buf, width, height, file_name);
    if(error) TEST_PRINTF("error %u: %s\n", error, lodepng_error_text(error));
    return error;
}

static unsigned write_png_file(void * raw_img, uint32_t width, uint32_t height, char * file_name)
{
    TEST_PRINTF("start to write png file: %s, width: %u, height: %u, img: %p\n", file_name, width, height, raw_img);
    unsigned  error = lodepng_encode32_file(file_name, raw_img, width, height);
    if(error) TEST_PRINTF("error %u: %s\n", error, lodepng_error_text(error));
    return error;
}

static void buf_to_xrgb8888(const uint8_t * buf_in, uint8_t * buf_out, lv_color_format_t cf_in)
{
    uint32_t stride = lv_draw_buf_width_to_stride(800, cf_in);
    if(cf_in == LV_COLOR_FORMAT_RGB565) {
        uint32_t y;
        for(y = 0; y < 480; y++) {

            uint32_t x;
            for(x = 0; x < 800; x++) {
                const lv_color16_t * c16 = (const lv_color16_t *)&buf_in[x * 2];

                buf_out[x * 4 + 3] = 0xff;
                buf_out[x * 4 + 2] = (c16->blue * 2106) >> 8;  /*To make it rounded*/
                buf_out[x * 4 + 1] = (c16->green * 1037) >> 8;
                buf_out[x * 4 + 0] = (c16->red * 2106) >> 8;
            }

            buf_in += stride;
            buf_out += 800 * 4;
        }
    }
    else if(cf_in == LV_COLOR_FORMAT_ARGB8888 || cf_in == LV_COLOR_FORMAT_XRGB8888) {
        uint32_t y;
        for(y = 0; y < 480; y++) {
            uint32_t x;
            for(x = 0; x < 800; x++) {
                buf_out[x * 4 + 3] = buf_in[x * 4 + 3];
                buf_out[x * 4 + 2] = buf_in[x * 4 + 0];
                buf_out[x * 4 + 1] = buf_in[x * 4 + 1];
                buf_out[x * 4 + 0] = buf_in[x * 4 + 2];
            }

            buf_in += stride;
            buf_out += 800 * 4;
        }
    }
    else if(cf_in == LV_COLOR_FORMAT_RGB888) {
        uint32_t y;
        for(y = 0; y < 480; y++) {
            uint32_t x;
            for(x = 0; x < 800; x++) {
                buf_out[x * 4 + 3] = 0xff;
                buf_out[x * 4 + 2] = buf_in[x * 3 + 0];
                buf_out[x * 4 + 1] = buf_in[x * 3 + 1];
                buf_out[x * 4 + 0] = buf_in[x * 3 + 2];
            }

            buf_in += stride;
            buf_out += 800 * 4;
        }
    }
}

static void create_folders_if_needed(const char * path)
{
    char * ptr;
    char * path_copy = lv_strdup(path);
    if(path_copy == NULL) {
        perror("Error duplicating path");
        exit(EXIT_FAILURE);
    }

    char * token = strtok_r(path_copy, "/", &ptr);
    char current_path[1024] = {'\0'}; /* Adjust the size as needed */
    struct stat st;

    while(token && ptr && *ptr != '\0') {
        lv_strcat(current_path, token);
        lv_strcat(current_path, "/");

        if(stat(current_path, &st) != 0) {
            // Folder doesn't exist, create it
            if(mkdir(current_path, 0777) != 0) {
                perror("Error creating folder");
                lv_free(path_copy);
                exit(EXIT_FAILURE);
            }
            printf("Created folder: %s\n", current_path);
        }

        token = strtok_r(NULL, "/", &ptr);
    }

    lv_free(path_copy);
}

#endif
