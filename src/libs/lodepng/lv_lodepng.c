/**
 * @file lv_lodepng.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"
#if LV_USE_LODEPNG

#include "lv_lodepng.h"
#include "lodepng.h"
#include <stdlib.h>

#if LV_USE_LODEPNG_ZLIB_EXTERNAL
#include <zlib.h>
#endif

/*********************
 *      DEFINES
 *********************/

#define DECODER_NAME    "LODEPNG"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_result_t decoder_info(lv_image_decoder_t * decoder, const void * src, lv_image_header_t * header);
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc);
static void decoder_close(lv_image_decoder_t * dec, lv_image_decoder_dsc_t * dsc);
static void convert_color_depth(uint8_t * img_p, uint32_t px_cnt);
static lv_draw_buf_t * decode_png_data(lv_image_decoder_dsc_t * dsc, const void * png_data, size_t png_data_size);
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
 * Register the PNG decoder functions in LVGL
 */
void lv_lodepng_init(void)
{
    lv_image_decoder_t * dec = lv_image_decoder_create();
    lv_image_decoder_set_info_cb(dec, decoder_info);
    lv_image_decoder_set_open_cb(dec, decoder_open);
    lv_image_decoder_set_close_cb(dec, decoder_close);

    dec->name = DECODER_NAME;
}

void lv_lodepng_deinit(void)
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
 * Get info about a PNG image
 * @param decoder   pointer to the decoder where this function belongs
 * @param src       can be file name or pointer to a C array
 * @param header    image information is set in header parameter
 * @return          LV_RESULT_OK: no error; LV_RESULT_INVALID: can't get the info
 */
static lv_result_t decoder_info(lv_image_decoder_t * decoder, const void * src, lv_image_header_t * header)
{
    LV_UNUSED(decoder); /*Unused*/
    lv_image_src_t src_type = lv_image_src_get_type(src);          /*Get the source type*/

    /*If it's a PNG file...*/
    if(src_type == LV_IMAGE_SRC_FILE) {
        const char * fn = src;
        if(lv_strcmp(lv_fs_get_ext(fn), "png") == 0) {              /*Check the extension*/

            /* Read the width and height from the file. They have a constant location:
             * [16..23]: width
             * [24..27]: height
             */
            uint32_t size[2];
            lv_fs_file_t f;
            lv_fs_res_t res = lv_fs_open(&f, fn, LV_FS_MODE_RD);
            if(res != LV_FS_RES_OK) return LV_RESULT_INVALID;

            lv_fs_seek(&f, 16, LV_FS_SEEK_SET);

            uint32_t rn;
            lv_fs_read(&f, &size, 8, &rn);
            lv_fs_close(&f);

            if(rn != 8) return LV_RESULT_INVALID;

            /*Save the data in the header*/
            header->cf = LV_COLOR_FORMAT_ARGB8888;
            /*The width and height are stored in Big endian format so convert them to little endian*/
            header->w = (int32_t)((size[0] & 0xff000000) >> 24) + ((size[0] & 0x00ff0000) >> 8);
            header->h = (int32_t)((size[1] & 0xff000000) >> 24) + ((size[1] & 0x00ff0000) >> 8);

            return LV_RESULT_OK;
        }
    }
    /*If it's a PNG file in a  C array...*/
    else if(src_type == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t * img_dsc = src;
        const uint32_t data_size = img_dsc->data_size;
        const uint32_t * size = ((uint32_t *)img_dsc->data) + 4;
        const uint8_t magic[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
        if(data_size < sizeof(magic)) return LV_RESULT_INVALID;
        if(memcmp(magic, img_dsc->data, sizeof(magic))) return LV_RESULT_INVALID;

        header->cf = LV_COLOR_FORMAT_ARGB8888;

        if(img_dsc->header.w) {
            header->w = img_dsc->header.w;         /*Save the image width*/
        }
        else {
            header->w = (int32_t)((size[0] & 0xff000000) >> 24) + ((size[0] & 0x00ff0000) >> 8);
        }

        if(img_dsc->header.h) {
            header->h = img_dsc->header.h;         /*Save the color height*/
        }
        else {
            header->h = (int32_t)((size[1] & 0xff000000) >> 24) + ((size[1] & 0x00ff0000) >> 8);
        }

        return LV_RESULT_OK;
    }

    return LV_RESULT_INVALID;         /*If didn't succeeded earlier then it's an error*/
}

/**
 * Open a PNG image and decode it into dsc.decoded
 * @param decoder   pointer to the decoder where this function belongs
 * @param dsc       decoded image descriptor
 * @return          LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
static lv_result_t decoder_open(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder);
    LV_PROFILER_BEGIN_TAG("lv_lodepng_decoder_open");

    const uint8_t * png_data = NULL;
    size_t png_data_size = 0;
    if(dsc->src_type == LV_IMAGE_SRC_FILE) {
        const char * fn = dsc->src;
        if(lv_strcmp(lv_fs_get_ext(fn), "png") == 0) {              /*Check the extension*/
            unsigned error;
            error = lodepng_load_file((void *)&png_data, &png_data_size, fn);  /*Load the file*/
            if(error) {
                if(png_data != NULL) {
                    lv_free((void *)png_data);
                }
                LV_LOG_WARN("error %u: %s\n", error, lodepng_error_text(error));
                LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
                return LV_RESULT_INVALID;
            }
        }
    }
    else if(dsc->src_type == LV_IMAGE_SRC_VARIABLE) {
        const lv_image_dsc_t * img_dsc = dsc->src;
        png_data = img_dsc->data;
        png_data_size = img_dsc->data_size;
    }
    else {
        LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
        return LV_RESULT_INVALID;
    }

    lv_draw_buf_t * decoded = decode_png_data(dsc, png_data, png_data_size);

    if(dsc->src_type == LV_IMAGE_SRC_FILE) lv_free((void *)png_data);

    if(!decoded) {
        LV_LOG_WARN("Error decoding PNG\n");
        LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
        return LV_RESULT_INVALID;
    }

    lv_draw_buf_t * adjusted = lv_image_decoder_post_process(dsc, decoded);
    if(adjusted == NULL) {
        lv_draw_buf_destroy(decoded);
        LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
        return LV_RESULT_INVALID;
    }

    /*The adjusted draw buffer is newly allocated.*/
    if(adjusted != decoded) {
        lv_draw_buf_destroy(decoded);
        decoded = adjusted;
    }

    dsc->decoded = decoded;

    if(dsc->args.no_cache) {
        LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
        return LV_RES_OK;
    }

    /*If the image cache is disabled, just return the decoded image*/
    if(!lv_image_cache_is_enabled()) return LV_RESULT_OK;

    /*Add the decoded image to the cache*/
    lv_image_cache_data_t search_key;
    search_key.src_type = dsc->src_type;
    search_key.src = dsc->src;
    search_key.slot.size = decoded->data_size;

    lv_cache_entry_t * entry = lv_image_decoder_add_to_cache(decoder, &search_key, decoded, NULL);

    if(entry == NULL) {
        LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
        return LV_RESULT_INVALID;
    }
    dsc->cache_entry = entry;

    LV_PROFILER_END_TAG("lv_lodepng_decoder_open");
    return LV_RESULT_OK;    /*If not returned earlier then it failed*/
}

/**
 * Close PNG image and free data
 * @param decoder   pointer to the decoder where this function belongs
 * @param dsc       decoded image descriptor
 * @return          LV_RESULT_OK: no error; LV_RESULT_INVALID: can't open the image
 */
static void decoder_close(lv_image_decoder_t * decoder, lv_image_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder);

    if(dsc->args.no_cache || !lv_image_cache_is_enabled())
        lv_draw_buf_destroy((lv_draw_buf_t *)dsc->decoded);
    else
        lv_cache_release(dsc->cache, dsc->cache_entry, NULL);
}

#if LV_USE_LODEPNG_ZLIB_EXTERNAL
static  unsigned lv_custom_zlib(unsigned char ** out, size_t * outsize,
                                const unsigned char * in, size_t insize, const LodePNGDecompressSettings * settings)
{
    uLongf destlen = *outsize;

    int ret = uncompress((Bytef *)*out, &destlen, (Bytef *)in, insize);
    *outsize = destlen;

    return ret;
}
#endif

static lv_draw_buf_t * decode_png8_data(const void * png_data, size_t png_data_size)
{
    LodePNGState state;
    LodePNGColorMode * color;
    unsigned error;
    unsigned int png_width;         /*Will be the width of the decoded image*/
    unsigned int png_height;        /*Will be the width of the decoded image*/
    lv_draw_buf_t * decoded;

    /*Check if png is in palette mode*/
    lodepng_state_init(&state);
    error = lodepng_inspect(NULL, NULL, &state, png_data, png_data_size);
    if(error) {
        LV_LOG_WARN("inspect png failed: %s", lodepng_error_text(error));
        lodepng_state_cleanup(&state);
        return NULL;
    }
    color = &state.info_png.color;
    if(color->colortype != LCT_PALETTE || color->bitdepth != 8) {
        lodepng_state_cleanup(&state);
        return NULL;
    }

    state.decoder.color_convert = 0; /* Do not convert color */
    state.decoder.ignore_crc = 1; /* Ignore CRC to improve speed a bit*/
#if LV_USE_LODEPNG_ZLIB_EXTERNAL
    state.decoder.zlibsettings.custom_zlib = lv_custom_zlib;
#endif
    error = lodepng_decode((unsigned char **)&decoded, &png_width, &png_height, &state, png_data, png_data_size);

    if(color->palette == NULL || color->palettesize == 0) {
        LV_LOG_WARN("PNG palette is empty");
        lodepng_state_cleanup(&state);
        if(decoded) lv_draw_buf_destroy(decoded);
        return NULL;
    }

    /* copy palette */
    lv_memcpy(decoded->data, color->palette, color->palettesize * 4);
    /* LODPNG palette is not in ARGB, need to revert B and R */

    lv_color32_t * img_argb = (lv_color32_t *)decoded->data;
    uint32_t i;
    for(i = 0; i < color->palettesize; i++) {
        uint8_t blue = img_argb[i].blue;
        img_argb[i].blue = img_argb[i].red;
        img_argb[i].red = blue;
    }

    lodepng_state_cleanup(&state);
    return (lv_draw_buf_t *)decoded;
}

static lv_draw_buf_t * decode_png_data(lv_image_decoder_dsc_t * dsc, const void * png_data, size_t png_data_size)
{
    unsigned png_width;             /*Not used, just required by the decoder*/
    unsigned png_height;            /*Not used, just required by the decoder*/
    lv_draw_buf_t * decoded = NULL;

    if(dsc->args.use_indexed) {
        decoded = decode_png8_data(png_data, png_data_size);
        if(decoded) return decoded;
    }

    /*Decode the image in ARGB8888 */
    unsigned error = lodepng_decode32((unsigned char **)&decoded, &png_width, &png_height, png_data, png_data_size);
    if(error) {
        if(decoded != NULL)  lv_draw_buf_destroy(decoded);
        return NULL;
    }

    /*Convert the image to the system's color depth*/
    convert_color_depth(decoded->data,  png_width * png_height);

    return decoded;
}

/**
 * If the display is not in 32 bit format (ARGB888) then convert the image to the current color depth
 * @param img the ARGB888 image
 * @param px_cnt number of pixels in `img`
 */
static void convert_color_depth(uint8_t * img_p, uint32_t px_cnt)
{
    lv_color32_t * img_argb = (lv_color32_t *)img_p;
    uint32_t i;
    for(i = 0; i < px_cnt; i++) {
        uint8_t blue = img_argb[i].blue;
        img_argb[i].blue = img_argb[i].red;
        img_argb[i].red = blue;
    }
}

#endif /*LV_USE_LODEPNG*/
