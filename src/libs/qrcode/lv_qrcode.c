/**
 * @file lv_qrcode.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../lvgl.h"

#if LV_USE_QRCODE

#include "qrcodegen.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_qrcode_class)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_qrcode_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_qrcode_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static int32_t get_satisfied_size(int32_t min_version, int32_t size, int32_t * scale);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_qrcode_class = {
    .constructor_cb = lv_qrcode_constructor,
    .destructor_cb = lv_qrcode_destructor,
    .instance_size = sizeof(lv_qrcode_t),
    .base_class = &lv_canvas_class,
    .name = "qrcode",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_qrcode_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_qrcode_set_size(lv_obj_t * obj, int32_t size)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_draw_buf_t * old_buf = lv_canvas_get_draw_buf(obj);
    lv_draw_buf_t * new_buf = lv_draw_buf_create(size, size, LV_COLOR_FORMAT_I1, LV_STRIDE_AUTO);
    if(new_buf == NULL) {
        LV_LOG_ERROR("malloc failed for canvas buffer");
        return;
    }

    lv_canvas_set_draw_buf(obj, new_buf);
    LV_LOG_INFO("set canvas buffer: %p, size = %d", (void *)new_buf, (int)size);

    /*Clear canvas buffer*/
    lv_draw_buf_clear(new_buf, NULL);

    if(old_buf != NULL) lv_draw_buf_destroy(old_buf);
}

void lv_qrcode_set_type(lv_obj_t * obj, lv_qrcode_type_t type)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_qrcode_t * qrcode = (lv_qrcode_t *)obj;
    qrcode->style_type = type;
}

void lv_qrcode_set_dark_color(lv_obj_t * obj, lv_color_t color)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_qrcode_t * qrcode = (lv_qrcode_t *)obj;
    qrcode->dark_color = color;
}

void lv_qrcode_set_light_color(lv_obj_t * obj, lv_color_t color)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_qrcode_t * qrcode = (lv_qrcode_t *)obj;
    qrcode->light_color = color;
}

/* Helper: set a pixel in the 1-bit canvas buffer to dark (1).
 * buf_u8 points to image data (skip palette).
 * row_byte_cnt is draw_buf->header.stride.
 * Assumes buffer was cleared (0 = light) before drawing. */
static inline void set_canvas_px(uint8_t * buf_u8, uint32_t stride, int32_t x, int32_t y)
{
    uint32_t byte_index = stride * (uint32_t)y + (uint32_t)(x >> 3);
    uint8_t mask = (uint8_t)(1 << (7 - (x & 7)));
    buf_u8[byte_index] |= mask; /* set pixel to dark */
}

void lv_circle_qrcode_update(int32_t qr_size, int32_t scale, uint8_t * qr0, int margin, lv_draw_buf_t * draw_buf,
                             uint8_t * buf_u8)
{
    uint32_t row_byte_cnt = draw_buf->header.stride;
    /* Determine finder (eye) areas in module coordinates (7x7 blocks at three corners) */
    const int modules = qr_size;
    const int fp_size = 7; /* finder pattern is 7x7 modules */
    const int fp_coords[3][2] = {
        {0, 0},                       /* top-left */
        {modules - fp_size, 0},       /* top-right */
        {0, modules - fp_size}        /* bottom-left */
    };

    /* 1) Draw ordinary modules as circular modules, except modules that are inside finder areas.
    * Each module is drawn as a filled circle centered in its cell (radius ~= scale/2).
    */
    const float module_radius = scale / 2.0f;
    const float module_r2 = module_radius * module_radius;
    for(int module_y = 0; module_y < modules; module_y++) {
        for(int module_x = 0; module_x < modules; module_x++) {
            /* Skip modules that belong to finder patterns; they'll be drawn as circular rings below */
            bool in_finder = false;
            for(int i = 0; i < 3; i++) {
                int fx = fp_coords[i][0], fy = fp_coords[i][1];
                if(module_x >= fx && module_x < fx + fp_size && module_y >= fy && module_y < fy + fp_size) {
                    in_finder = true;
                    break;
                }
            }
            if(in_finder || !qrcodegen_getModule(qr0, module_x, module_y)) continue;

            /* pixel region for this module */
            int start_x = margin + module_x * scale;
            int start_y = margin + module_y * scale;
            /* center of the module cell */
            float cx = start_x + (scale - 1) / 2.0f;
            float cy = start_y + (scale - 1) / 2.0f;

            for(int py = 0; py < scale; py++) {
                int yPixel = start_y + py;
                if(yPixel < 0 || yPixel >= draw_buf->header.h) continue;
                for(int px = 0; px < scale; px++) {
                    int xPixel = start_x + px;
                    if(xPixel < 0 || xPixel >= draw_buf->header.w) continue;

                    float dx = (float)xPixel - cx;
                    float dy = (float)yPixel - cy;
                    if(dx * dx + dy * dy <= module_r2) {
                        set_canvas_px(buf_u8, row_byte_cnt, xPixel, yPixel);
                    }
                }
            }
        }
    }

    /* 2) Draw finder patterns as circular ring style:
    *    - Outer ring: corresponds to the outer 7x7 dark border (dark modules except the 5x5 inner area).
    *    - Inner dot: corresponds to the central 3x3 dark area (drawn as a filled circle).
    *
    * emulate the original 7/5/3 nested squares with concentric circles:
    *   r_outer = half of (7*scale)
    *   r_mid   = half of (5*scale)
    *   r_inner = half of (3*scale)
    */
    for(int f = 0; f < 3; f++) {
        int fx = fp_coords[f][0], fy = fp_coords[f][1];

        /* pixel region covering the 7x7 finder block */
        int start_x = margin + fx * scale;
        int start_y = margin + fy * scale;
        int fp_pix_size = fp_size * scale;

        float cx = start_x + (fp_pix_size - 1) / 2.0f;
        float cy = start_y + (fp_pix_size - 1) / 2.0f;

        float r_outer = fp_pix_size / 2.0f;
        float r_mid = (fp_size - 2) * scale / 2.0f;   /* 5x5 => gap between outer and inner */
        float r_inner = (fp_size - 4) * scale / 2.0f; /* 3x3 => central filled circle */

        float r_outer2 = r_outer * r_outer;
        float r_mid2 = r_mid * r_mid;
        float r_inner2 = r_inner * r_inner;

        for(int py = 0; py < fp_pix_size; py++) {
            int yPixel = start_y + py;
            if(yPixel < 0 || yPixel >= draw_buf->header.h) continue;
            for(int px = 0; px < fp_pix_size; px++) {
                int xPixel = start_x + px;
                if(xPixel < 0 || xPixel >= draw_buf->header.w) continue;

                float dx = (float)xPixel - cx;
                float dy = (float)yPixel - cy;
                float d2 = dx * dx + dy * dy;

                /* Outer ring or inner filled dot */
                if((d2 <= r_outer2 && d2 > r_mid2) || d2 <= r_inner2) {
                    set_canvas_px(buf_u8, row_byte_cnt, xPixel, yPixel);
                }
            }
        }
    }
}

lv_result_t lv_qrcode_update(lv_obj_t * obj, const void * data, uint32_t data_len)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_qrcode_t * qrcode = (lv_qrcode_t *)obj;

    lv_draw_buf_t * draw_buf = lv_canvas_get_draw_buf(obj);
    if(draw_buf == NULL) {
        LV_LOG_ERROR("canvas draw buffer is NULL");
        return LV_RESULT_INVALID;
    }

    lv_draw_buf_clear(draw_buf, NULL);
    lv_canvas_set_palette(obj, 0, lv_color_to_32(qrcode->light_color, LV_OPA_COVER));
    lv_canvas_set_palette(obj, 1, lv_color_to_32(qrcode->dark_color, LV_OPA_COVER));
    lv_image_cache_drop(draw_buf);

    lv_obj_invalidate(obj);

    if(data_len > qrcodegen_BUFFER_LEN_MAX) return LV_RESULT_INVALID;

    int32_t qr_version = qrcodegen_getMinFitVersion(qrcodegen_Ecc_MEDIUM, data_len);
    int32_t quiet_zone_scale = 0;
    if(qrcode->quiet_zone) qr_version = get_satisfied_size(qr_version, draw_buf->header.w, &quiet_zone_scale);
    if(qr_version <= 0 || (qrcode->quiet_zone && quiet_zone_scale <= 0)) return LV_RESULT_INVALID;

    const int32_t qr_size = qrcodegen_version2size(qr_version);
    if(qr_size <= 0) return LV_RESULT_INVALID;
    const int32_t scale = qrcode->quiet_zone ? quiet_zone_scale : draw_buf->header.w / qr_size;

    uint8_t * qr0 = lv_malloc(qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version));
    LV_ASSERT_MALLOC(qr0);
    uint8_t * data_tmp = lv_malloc(qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version));
    LV_ASSERT_MALLOC(data_tmp);
    lv_memcpy(data_tmp, data, data_len);

    bool ok = qrcodegen_encodeBinary(data_tmp, data_len,
                                     qr0, qrcodegen_Ecc_MEDIUM,
                                     qr_version, qr_version,
                                     qrcodegen_Mask_AUTO, true);

    if(!ok) {
        lv_free(qr0);
        lv_free(data_tmp);
        return LV_RESULT_INVALID;
    }

    /* Temporarily disable invalidation to improve the efficiency of lv_canvas_set_px */
    lv_display_enable_invalidation(lv_obj_get_display(obj), false);

    int32_t obj_w = draw_buf->header.w;
    int scaled = qr_size * scale;
    int margin = (obj_w - scaled) / 2;
    uint8_t * buf_u8 = draw_buf->data + 8;    /*+8 skip the palette*/
    lv_color_t c = lv_color_hex(1);

    /* Copy the qr code canvas:
     * A simple `lv_canvas_set_px` would work but it's slow for so many pixels.
     * So buffer 1 byte (8 px) from the qr code and set it in the canvas image */
    uint32_t row_byte_cnt = draw_buf->header.stride;

    // Draw circle type
    if(qrcode->style_type == LV_QRCODE_TYPE_CIRCLE) {
        lv_circle_qrcode_update(qr_size,  scale, qr0, margin, draw_buf, buf_u8);

        /* invalidate the canvas to refresh it */
        lv_display_enable_invalidation(lv_obj_get_display(obj), true);

        lv_free(qr0);
        lv_free(data_tmp);
        return LV_RESULT_OK;
    }

    // Draw rect type
    int y;
    for(y = margin; y < scaled + margin; y += scale) {
        uint8_t b = 0;
        uint8_t p = 0;
        bool aligned = false;
        int x;
        for(x = margin; x < scaled + margin; x++) {
            bool a = qrcodegen_getModule(qr0, (x - margin) / scale, (y - margin) / scale);

            if(aligned == false && (x & 0x7) == 0) aligned = true;

            if(aligned == false) {
                if(a) {
                    lv_canvas_set_px(obj, x, y, c, LV_OPA_COVER);
                }
            }
            else {
                if(!a) b |= (1 << (7 - p));
                p++;
                if(p == 8) {
                    uint32_t px = row_byte_cnt * y + (x >> 3);
                    buf_u8[px] = ~b;
                    b = 0;
                    p = 0;
                }
            }
        }

        /*Process the last byte of the row*/
        if(p) {
            /*Make the rest of the bits white*/
            b |= (1 << (8 - p)) - 1;

            uint32_t px = row_byte_cnt * y + (x >> 3);
            buf_u8[px] = ~b;
        }

        /*The Qr is probably scaled so simply to the repeated rows*/
        int s;
        const uint8_t * row_ori = buf_u8 + row_byte_cnt * y;
        for(s = 1; s < scale; s++) {
            lv_memcpy((uint8_t *)buf_u8 + row_byte_cnt * (y + s), row_ori, row_byte_cnt);
        }
    }

    /* invalidate the canvas to refresh it */
    lv_display_enable_invalidation(lv_obj_get_display(obj), true);

    lv_free(qr0);
    lv_free(data_tmp);
    return LV_RESULT_OK;
}

void lv_qrcode_set_quiet_zone(lv_obj_t * obj, bool enable)
{
    lv_qrcode_t * qrcode = (lv_qrcode_t *)obj;
    qrcode->quiet_zone = enable;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_qrcode_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    /*Set default size*/
    lv_qrcode_set_size(obj, LV_DPI_DEF);

    /*Set default type*/
    lv_qrcode_set_type(obj, LV_QRCODE_TYPE_RECT);

    /*Set default color*/
    lv_qrcode_set_dark_color(obj, lv_color_black());
    lv_qrcode_set_light_color(obj, lv_color_white());
}

static void lv_qrcode_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_draw_buf_t * draw_buf = lv_canvas_get_draw_buf(obj);
    if(draw_buf == NULL) return;
    lv_image_cache_drop(draw_buf);

    /*@fixme destroy buffer in cache free_cb.*/
    lv_draw_buf_destroy(draw_buf);
}

static int32_t get_satisfied_size(int32_t min_version, int32_t size, int32_t * scale)
{
    if(min_version <= 0) return -1;

    int32_t offset = size;
    int32_t satisfied_version = min_version;
    if(scale) *scale = 0;

    for(int32_t version = min_version; version <= min_version + 2 && version <= qrcodegen_VERSION_MAX - 3; version++) {
        int32_t version_size = qrcodegen_version2size(version + 1);
        int32_t tmp_offset = size % version_size;
        int32_t tmp_scale = size / version_size;

        if(tmp_offset < offset) {
            offset = tmp_offset;
            satisfied_version = version;
            if(scale) *scale = tmp_scale;
        }
    }
    return satisfied_version;
}

#endif /*LV_USE_QRCODE*/
