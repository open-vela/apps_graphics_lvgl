/**
 * @file lv_libwebp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"
#if LV_USE_LIBWEBP

#include "lv_libwebp.h"
#include <webp/format_constants.h>
#include <webp/decode.h>

/*********************
 *      DEFINES
 *********************/

#define DECODER_NAME    "WEBP"

#define image_cache_draw_buf_handlers &(LV_GLOBAL_DEFAULT()->image_cache_draw_buf_handlers)

#define WEBP_HEADER_SIZE    12
#define WEBP_HEADER_OFFSET  8

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header);
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static bool get_webp_size(const char * filename, int * width, int * height);
static lv_draw_buf_t * decode_webp_file(lv_image_decoder_dsc_t * dsc, const char * filename);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Register the WEBP decoder functions in LVGL
 */
void lv_libwebp_init(void)
{
    lv_image_decoder_t * dec = lv_image_decoder_create();
    lv_image_decoder_set_info_cb(dec, decoder_info);
    lv_image_decoder_set_open_cb(dec, decoder_open);
    lv_image_decoder_set_close_cb(dec, decoder_close);

    dec->name = DECODER_NAME;
}

void lv_libwebp_deinit(void)
{
    lv_image_decoder_t * dec = NULL;
    while((dec = lv_image_decoder_get_next(dec)) != NULL) {
        if(dec->info_cb == decoder_info) {
            lv_image_decoder_delete(dec);
            break;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Get info about a WEBP image
 * @param dsc can be file name or pointer to a C array
 * @param header store the info here
 * @return LV_RESULT_OK: no error; LV_RESULT_INVALID: can't get the info
 */
static lv_result_t decoder_info(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc, lv_image_header_t * header)
{
    LV_UNUSED(decoder); /*Unused*/
    const void * src = dsc->src;
    lv_image_src_t src_type = dsc->src_type;          /*Get the source type*/

    /*If it's a webp file...*/
    if(src_type == LV_IMAGE_SRC_FILE) {
        uint8_t buf[WEBP_HEADER_SIZE];
        uint32_t rn;
        lv_fs_res_t res = lv_fs_read(&dsc->file, buf, sizeof(buf), &rn);

        if(res != LV_FS_RES_OK || rn != sizeof(buf)) return LV_RESULT_INVALID;

        static const uint8_t header_riff[] = { 0x52, 0x49, 0x46, 0x46 }; /*RIFF*/
        static const uint8_t header_webp[] = { 0x57, 0x45, 0x42, 0x50 }; /*WEBP*/

        if(lv_memcmp(buf, header_riff, sizeof(header_riff)) != 0) return LV_RESULT_INVALID;
        if(lv_memcmp(buf + WEBP_HEADER_OFFSET, header_webp, sizeof(header_webp)) != 0) return LV_RESULT_INVALID;

        int width;
        int height;
        if(!get_webp_size(src, &width, &height)) {
            return LV_RESULT_INVALID;
        }

        /*Default decoder color format is ARGB8888*/
        header->cf = LV_COLOR_FORMAT_ARGB8888;
        header->w = width;
        header->h = height;
        header->stride = width * sizeof(lv_color32_t);
        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID;         /*If didn't succeeded earlier then it's an error*/
}

/**
 * Open a WEBP image and return the decided image
 * @param decoder pointer to the decoder
 * @param dsc     pointer to the decoder descriptor
 * @return LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/
    LV_PROFILER_DECODER_BEGIN_TAG("lv_libwebp_decoder_open");

    /*If it's a PNG file...*/
    if(dsc->src_type == LV_IMAGE_SRC_FILE) {
        const char * fn = dsc->src;
        lv_draw_buf_t * decoded = decode_webp_file(dsc, fn);
        if(decoded == NULL) {
            LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
            return LV_RESULT_INVALID;
        }

        lv_draw_buf_t * adjusted = lv_image_decoder_post_process(dsc, decoded);
        if(adjusted == NULL) {
            lv_draw_buf_destroy(decoded);
            LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
            return LV_RESULT_INVALID;
        }

        /*The adjusted draw buffer is newly allocated.*/
        if(adjusted != decoded) {
            lv_draw_buf_destroy(decoded);
            decoded = adjusted;
        }

        dsc->decoded = decoded;

        if(dsc->args.no_cache) {
            LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
            return LV_RES_OK;
        }

        /*If the image cache is disabled, just return the decoded image*/
        if(!lv_image_cache_is_enabled()) {
            LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
            return LV_RESULT_OK;
        }

        lv_image_cache_data_t search_key;
        search_key.src_type = dsc->src_type;
        search_key.src = dsc->src;
        search_key.slot.size = decoded->data_size;

        lv_cache_entry_t * entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, NULL);

        if(entry == NULL) {
            lv_draw_buf_destroy(decoded);
            LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
            return LV_RESULT_INVALID;
        }
        dsc->cache_entry = entry;

        LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
        return LV_RESULT_OK;     /*The image is fully decoded. Return with its pointer*/
    }

    LV_PROFILER_DECODER_END_TAG("lv_libwebp_decoder_open");
    return LV_RESULT_INVALID;    /*If not returned earlier then it failed*/
}

/**
 * Free the allocated resources
 */
static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/

    if(dsc->args.no_cache ||
       !lv_image_cache_is_enabled()) lv_draw_buf_destroy_user(image_cache_draw_buf_handlers, (lv_draw_buf_t *)dsc->decoded);
}

static uint8_t * alloc_file(const char * filename, uint32_t * size)
{
    uint8_t * data = NULL;
    lv_fs_file_t f;
    uint32_t data_size;
    uint32_t rn;
    lv_fs_res_t res;

    *size = 0;

    res = lv_fs_open(&f, filename, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        LV_LOG_WARN("can't open %s", filename);
        return NULL;
    }

    res = lv_fs_seek(&f, 0, LV_FS_SEEK_END);
    if(res != LV_FS_RES_OK) {
        goto failed;
    }

    res = lv_fs_tell(&f, &data_size);
    if(res != LV_FS_RES_OK) {
        goto failed;
    }

    res = lv_fs_seek(&f, 0, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        goto failed;
    }

    /*Read file to buffer*/
    data = lv_malloc(data_size);
    if(data == NULL) {
        LV_LOG_WARN("malloc failed for data");
        goto failed;
    }

    res = lv_fs_read(&f, data, data_size, &rn);

    if(res == LV_FS_RES_OK && rn == data_size) {
        *size = rn;
    }
    else {
        LV_LOG_WARN("read file failed");
        lv_free(data);
        data = NULL;
    }

failed:
    lv_fs_close(&f);

    return data;
}

static bool get_webp_size(const char * filename, int * width, int * height)
{
    uint8_t * data = NULL;
    uint32_t data_size;
    data = alloc_file(filename, &data_size);
    if(data == NULL) {
        return false;
    }

    if(WebPGetInfo(data, data_size, width, height) == 0) {
        LV_LOG_ERROR("webp file %s get info failed.", filename);
        lv_free(data);
        return false;
    }

    lv_free(data);
    return true;
}

static lv_draw_buf_t * decode_webp_file(lv_image_decoder_dsc_t * dsc, const char * filename)
{
    uint32_t data_size;
    uint8_t * data = alloc_file(filename, &data_size);
    if(data == NULL) {
        LV_LOG_WARN("can't load file: %s", filename);
        return NULL;
    }

    /*Alloc image buffer*/
    lv_draw_buf_t * decoded;
    decoded = lv_draw_buf_create_user(image_cache_draw_buf_handlers, dsc->header.w, dsc->header.h, dsc->header.cf,
                                      LV_STRIDE_AUTO);
    if(decoded == NULL) {
        LV_LOG_ERROR("alloc draw buffer failed: %s", filename);
        lv_free(data);
        return NULL;
    }

    WebPDecoderConfig config;
    WebPInitDecoderConfig(&config);

    config.output.colorspace = MODE_BGRA;
    config.output.u.RGBA.rgba = (uint8_t *) decoded->data;
    config.output.u.RGBA.stride = decoded->header.stride;
    config.output.u.RGBA.size = decoded->data_size;
    config.output.is_external_memory = 1;

    WebPDecode(data, data_size, &config);

    lv_free(data);
    return decoded;
}

#endif /*LV_USE_LIBWEBP*/
