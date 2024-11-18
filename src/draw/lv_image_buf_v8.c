/**
 * @file lv_image_buf_v8.c
 */
#include "lv_image_buf_v8.h"
#include "../misc/lv_color_v8.h"
#include "../misc/lv_assert.h"
#include "../misc/lv_log.h"


void lv_image_header_convert_from_v8(lv_image_header_t * src_header)
{
    LV_ASSERT_NULL(src_header);

    lv_bin_file_header_v8_t header_v8;
    /* v8 image header size must be >= v9 image header size*/
    LV_ASSERT(sizeof(lv_bin_file_header_v8_t) >= sizeof(lv_image_header_t));

    lv_memcpy(&header_v8, src_header, sizeof(lv_image_header_t));
    lv_memzero(src_header, sizeof(lv_image_header_t));
    /* v8 rle file */
    if(header_v8.rleheader.magic == LV_IMAGE_RLE_HEADER_MAGIC) {
        src_header->flags |= LV_IMAGE_FLAGS_COMPRESSED;
    }
    src_header->cf = lv_color_format_convert_from_v8(header_v8.header.cf);
    src_header->magic = LV_IMAGE_HEADER_MAGIC;
    src_header->flags |= LV_IMAGE_FLAGS_HEADER_V8;
    src_header->w = header_v8.header.w;
    src_header->h = header_v8.header.h;
    if(header_v8.header.tiled) {
        /* LV_IMAGE_FLAGS_USER1 defined as LV_VG_LITE_IMAGE_FLAGS_TILED */
        src_header->flags |= LV_IMAGE_FLAGS_USER1;
    }
    LV_LOG_INFO("width %d, height %d, cf %d", (int)header_v8.header.w, (int)header_v8.header.h,
                (int)header_v8.header.cf);
}

uint32_t lv_image_header_get_size(lv_image_header_t * header)
{
    LV_ASSERT_NULL(header);

    if(header->flags & LV_IMAGE_FLAGS_HEADER_V8) {
        if(header->flags & LV_IMAGE_FLAGS_COMPRESSED) {
            return sizeof(lv_bin_file_header_v8_t);
        }
        return sizeof(lv_image_header_v8_t);
    }

    return sizeof(lv_image_header_t);
}
