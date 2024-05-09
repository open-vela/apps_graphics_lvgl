#include "../../lv_examples.h"
#if LV_BUILD_EXAMPLES && LV_USE_VECTOR_GRAPHIC && LV_USE_SVG

void lv_example_svg_1(void)
{
    const char * svg_src = \
                           "<svg><g fill=\"#FF0000\">"
                           "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\"/>"
                           "<circle cx=\"100\" cy=\"100\" r=\"50\"/>"
                           "<ellipse fill=\"#00F\" cx=\"200\" cy=\"200\" rx=\"100\" ry=50/>"
                           "</g></svg>";

    lv_svg_node_t * svg = lv_svg_load_data(svg_src, lv_strlen(svg_src));
    if(!svg) {
        LV_LOG_ERROR("Could not load SVG data");
        return;
    }

    lv_draw_buf_t * draw_buf = lv_draw_buf_create(480, 480, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    if(!draw_buf) {
        LV_LOG_ERROR("Could not create canvas buffer");
        lv_svg_node_delete(svg);
        return;
    }
    lv_draw_buf_clear(draw_buf, NULL);

    lv_obj_t * canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_draw_buf(canvas, draw_buf);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_draw_svg(&layer, svg);

    lv_canvas_finish_layer(canvas, &layer);
    lv_image_cache_drop(draw_buf);
    lv_svg_node_delete(svg);
}

#endif
