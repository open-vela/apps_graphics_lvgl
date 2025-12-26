/**
 * @file lv_vector_stroke.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <math.h>
#include <stdlib.h>

#include "lv_vector_stroke.h"
#include "lv_vector_polygon.h"

#if LV_USE_VECTOR_GRAPHIC

/*********************
 *      DEFINES
 *********************/
#define DEFAULT_DISTANCE_NUM    (16)
#define DISTANCE_MAX            (1E-14f)
#define INTERSECT_LIMIT         (1E-30)
#define DEFAULT_DASH_NUM        (8)

#define ARRAY_PUSH_BACK(arr, p, type) \
    do { \
        if ((lv_array_size((arr)) + 1) > lv_array_capacity((arr))) { \
            lv_array_resize((arr), (arr)->capacity << 1); \
        } \
        type * data = ((type *)(arr)->data) + (arr)->size; \
        *data = *p; \
        (arr)->size++; \
    } while(0)

enum {
    LV_BASE_GEN_INITIAL,
    LV_BASE_GEN_START_ACCUMULATE,
    LV_BASE_GEN_ACCUMULATE,
    LV_BASE_GEN_GENERATE,
};
typedef uint16_t lv_base_gen_state_t;

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    float x;
    float y;
    float d;
} lv_distance_point_t;

typedef struct _base_generator {
    lv_base_gen_state_t state;
    lv_vector_path_op_t last_op;
    lv_fpoint_t start_point;
    const lv_vector_stroke_dsc_t * dsc;
    void (*reset)(struct _base_generator * gen);
    void (*clear)(struct _base_generator * gen);
    void (*accumulate)(struct _base_generator * gen, const lv_fpoint_t * point, lv_vector_path_op_t cmd);
    bool (*generated)(struct _base_generator * gen, lv_fpoint_t * point, lv_vector_path_op_t * op);
    // user callback
    path_generate_cb cb;
    void * user_data;
} lv_base_generator;

#define BASE_GEN(x) ((lv_base_generator*)(x))

enum {
    LV_LINE_GEN_INITIAL,
    LV_LINE_GEN_READY,
    LV_LINE_GEN_START_CAP,
    LV_LINE_GEN_END_CAP,
    LV_LINE_GEN_FIRST_OUTLINE,
    LV_LINE_GEN_SECOND_OUTLINE,
    LV_LINE_GEN_FIRST_CLOSE,
    LV_LINE_GEN_OUTPUT,
    LV_LINE_GEN_END_POLYGON,
    LV_LINE_GEN_STOP,
};
typedef uint16_t lv_line_gen_state_t;

/* stroke to path line generator */
typedef struct {
    lv_base_generator gen;
    lv_line_gen_state_t state;
    lv_line_gen_state_t prev_state;
    lv_array_t dist_arr;
    lv_array_t out_arr;
    uint32_t dist_idx;
    uint32_t out_idx;
    bool closed;
    /* line calc */
    float half_width;
    float w_abs;
    float w_eps;
    int32_t w_sign;
} lv_line_generator;

#define LINE_GEN(x) ((lv_line_generator*)(x))

/* dash line generator */
typedef struct {
    lv_line_generator gen;
    lv_array_t * dash_arr;
    float dash_offset;
    /* dash calc */
    lv_distance_point_t * p1;
    lv_distance_point_t * p2;
    float distance;
    float cur_dash_offset;
    uint32_t cur_dash_idx;
} lv_dash_generator;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_base_generator_init(lv_base_generator * gen)
{
    lv_memset(gen, 0, sizeof(lv_base_generator));
    gen->state = LV_BASE_GEN_INITIAL;
    gen->last_op = LV_VECTOR_PATH_OP_MOVE_TO;
    gen->cb = NULL;
    gen->user_data = NULL;
}

static void remove_last_distance_point(lv_array_t * array)
{
    if(!lv_array_is_empty(array)) {
        array->size--;
    }
}

static inline float distance_to(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

static inline bool calc_distance(lv_distance_point_t * p1, lv_distance_point_t * p2)
{
    p1->d = distance_to(p1->x, p1->y, p2->x, p2->y);
    bool ret = p1->d > DISTANCE_MAX;

    if(!ret) {
        p1->d = 1.0f / DISTANCE_MAX;
    }
    return ret;
}

static void add_distance_point(lv_array_t * array, const lv_distance_point_t * dp)
{
    uint32_t size = lv_array_size(array);
    if(size > 1) {
        lv_distance_point_t * p1 = (lv_distance_point_t *)lv_array_at(array, size - 2);
        lv_distance_point_t * p2 = (lv_distance_point_t *)lv_array_at(array, size - 1);

        if(!calc_distance(p1, p2)) {
            remove_last_distance_point(array);
        }
    }
    ARRAY_PUSH_BACK(array, dp, lv_distance_point_t);
}

static void close_distance(lv_array_t * array, bool closed)
{
    uint32_t size = 0;
    lv_distance_point_t * p1 = NULL;
    lv_distance_point_t * p2 = NULL;

    while((size = lv_array_size(array)) > 1) {
        p1 = (lv_distance_point_t *)lv_array_at(array, size - 2);
        p2 = (lv_distance_point_t *)lv_array_at(array, size - 1);

        if(calc_distance(p1, p2)) {
            break;
        }

        remove_last_distance_point(array);
        // modify last
        remove_last_distance_point(array);
        add_distance_point(array, p2);
    }

    if(closed) {
        while((size = lv_array_size(array)) > 1) {
            p1 = (lv_distance_point_t *)lv_array_at(array, size - 1);
            p2 = (lv_distance_point_t *)lv_array_at(array, 0);

            if(calc_distance(p1, p2)) {
                break;
            }

            remove_last_distance_point(array);
        }
    }
}

static inline void add_out_point(lv_array_t * array, float x, float y)
{
    lv_fpoint_t p = {.x = x, .y = y };
    ARRAY_PUSH_BACK(array, &p, lv_fpoint_t);
}

static void calc_cap(lv_line_generator * gen, const lv_distance_point_t * p1, const lv_distance_point_t * p2, float len)
{
    lv_array_t * cs = &gen->out_arr;
    lv_array_clear(cs);

    const lv_vector_stroke_dsc_t * dsc = BASE_GEN(gen)->dsc;

    float w = gen->half_width;
    float w_abs = gen->w_abs;
    int32_t w_sign = gen->w_sign;

    float dx1 = (p2->y - p1->y) / len;
    float dy1 = (p2->x - p1->x) / len;
    float dx2 = 0;
    float dy2 = 0;

    dx1 *= w;
    dy1 *= w;

    if(dsc->cap != LV_VECTOR_STROKE_CAP_ROUND) {
        if(dsc->cap == LV_VECTOR_STROKE_CAP_SQUARE) {
            dx2 = dy1 * w_sign;
            dy2 = dx1 * w_sign;
        }
        add_out_point(cs, p1->x - dx1 - dx2, p1->y + dy1 - dy2);
        add_out_point(cs, p1->x + dx1 - dx2, p1->y - dy1 - dy2);
    }
    else {
        float a1;
        float da = acosf(w_abs / (w_abs + 0.125f)) * 2;
        int n = (int)roundf((float)M_PI / da);

        da = (float)M_PI / (n + 1);
        add_out_point(cs, p1->x - dx1, p1->y + dy1);

        if(w_sign > 0) {
            a1 = atan2f(dy1, -dx1);
            a1 += da;
            for(int i = 0; i < n; i++) {
                add_out_point(cs, p1->x + cosf(a1) * w, p1->y + sinf(a1) * w);
                a1 += da;
            }
        }
        else {
            a1 = atan2f(-dy1, dx1);
            a1 -= da;
            for(int i = 0; i < n; i++) {
                add_out_point(cs, p1->x + cosf(a1) * w, p1->y + sinf(a1) * w);
                a1 -= da;
            }
        }
        add_out_point(cs, p1->x + dx1, p1->y - dy1);
    }
}

static inline bool calc_intersection(float ax, float ay, float bx, float by,
                                     float cx, float cy, float dx, float dy, float * x, float * y)
{
    float num = (ay - cy) * (dx - cx) - (ax - cx) * (dy - cy);
    float den = (bx - ax) * (dy - cy) - (by - ay) * (dx - cx);

    if(fabs(den) < INTERSECT_LIMIT) {
        return false;
    }

    float r = num / den;
    *x = ax + r * (bx - ax);
    *y = ay + r * (by - ay);
    return true;
}

static inline float cross_product(float x1, float y1, float x2, float y2, float x, float y)
{
    return (x - x2) * (y2 - y1) - (y - y2) * (x2 - x1);
}

static void calc_arc(lv_line_generator * gen, float x, float y, float dx1, float dy1, float dx2, float dy2)
{
    lv_array_t * cs = &gen->out_arr;

    float w = gen->half_width;
    float w_abs = gen->w_abs;
    int32_t w_sign = gen->w_sign;

    float a1 = atan2f(dy1 * w_sign, dx1 * w_sign);
    float a2 = atan2f(dy2 * w_sign, dx2 * w_sign);
    float da = a1 - a2;
    int i, n;

    da = acosf(w_abs / (w_abs + 0.125f)) * 2;

    add_out_point(cs, x + dx1, y + dy1);
    if(w_sign > 0) {
        if(a1 > a2) {
            a2 += (float)(M_PI * 2);
        }
        n = (int)roundf((a2 - a1) / da);
        da = (a2 - a1) / (n + 1);
        a1 += da;
        for(i = 0; i < n; i++) {
            add_out_point(cs, x + cosf(a1) * w, y + sinf(a1) * w);
            a1 += da;
        }
    }
    else {
        if(a1 < a2) {
            a2 -= (float)(M_PI * 2);
        }
        n = (int)roundf((a1 - a2) / da);
        da = (a1 - a2) / (n + 1);
        a1 -= da;
        for(i = 0; i < n; i++) {
            add_out_point(cs, x + cosf(a1) * w, y + sinf(a1) * w);
            a1 -= da;
        }
    }
    add_out_point(cs, x + dx2, y + dy2);
}

static void calc_miter(lv_line_generator * gen, const lv_distance_point_t * p1, const lv_distance_point_t * p2,
                       const lv_distance_point_t * p3, float dx1, float dy1, float dx2, float dy2, float dbevel)
{
    lv_array_t * cs = &gen->out_arr;

    const lv_vector_stroke_dsc_t * dsc = BASE_GEN(gen)->dsc;

    float w_abs = gen->w_abs;
    int32_t w_sign = gen->w_sign;

    float mlimit = dsc->miter_limit;

    float xi = p2->x;
    float yi = p2->y;
    float di = 1.0f;
    float lim = w_abs * mlimit;
    bool miter_limit_exceeded = true;
    bool intersection_failed = true;

    if(calc_intersection(p1->x + dx1, p1->y - dy1, p2->x + dx1, p2->y - dy1,
                         p2->x + dx2, p2->y - dy2, p3->x + dx2, p3->y - dy2, &xi, &yi)) {
        di = distance_to(p2->x, p2->y, xi, yi);
        if(di <= lim) {
            add_out_point(cs, xi, yi);
            miter_limit_exceeded = false;
        }
        intersection_failed = false;
    }
    else {
        float x2 = p2->x + dx1;
        float y2 = p2->y - dy1;
        if((cross_product(p1->x, p1->y, p2->x, p2->y, x2, y2) < 0.0f) ==
           (cross_product(p2->x, p2->y, p3->x, p3->y, x2, y2) < 0.0f)) {
            // this case means that the next segment continues
            // the previous one (straight line)
            add_out_point(cs, p2->x + dx1, p2->y - dy1);
            miter_limit_exceeded = false;
        }
    }

    if(miter_limit_exceeded) {
        if(dsc->join == LV_VECTOR_STROKE_JOIN_ROUND) {
            calc_arc(gen, p2->x, p2->y, dx1, -dy1, dx2, -dy2);
        }
        else {
            if(intersection_failed) {
                mlimit *= w_sign;
                add_out_point(cs, p2->x + dx1 + dy1 * mlimit,
                              p2->y - dy1 + dx1 * mlimit);
                add_out_point(cs, p2->x + dx2 - dy2 * mlimit,
                              p2->y - dy2 - dx2 * mlimit);
            }
            else {
                float x1 = p2->x + dx1;
                float y1 = p2->y - dy1;
                float x2 = p2->x + dx2;
                float y2 = p2->y - dy2;
                di = (lim - dbevel) / (di - dbevel);
                add_out_point(cs, x1 + (xi - x1) * di, y1 + (yi - y1) * di);
                add_out_point(cs, x2 + (xi - x2) * di, y2 + (yi - y2) * di);
            }
        }
    }
}

static void calc_join(lv_line_generator * gen, const lv_distance_point_t * p1, const lv_distance_point_t * p2,
                      const lv_distance_point_t * p3, float len1, float len2)
{
    lv_array_t * cs = &gen->out_arr;
    lv_array_clear(cs);

    const lv_vector_stroke_dsc_t * dsc = BASE_GEN(gen)->dsc;

    float w = gen->half_width;
    float w_abs = gen->w_abs;
    float w_eps = gen->w_eps;

    float dx1 = w * (p2->y - p1->y) / len1;
    float dy1 = w * (p2->x - p1->x) / len1;
    float dx2 = w * (p3->y - p2->y) / len2;
    float dy2 = w * (p3->x - p2->x) / len2;

    float cp = cross_product(p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
    if((cp != 0) && ((cp > 0) == (w > 0))) {
        // inner join bevel
        add_out_point(cs, p2->x + dx1, p2->y - dy1);
        add_out_point(cs, p2->x + dx2, p2->y - dy2);
    }
    else {
        // outer join
        float dx = (dx1 + dx2) / 2;
        float dy = (dy1 + dy2) / 2;
        float dbevel = sqrtf(dx * dx + dy * dy);

        if(dsc->join == LV_VECTOR_STROKE_JOIN_ROUND || dsc->join == LV_VECTOR_STROKE_JOIN_BEVEL) {
            if((w_abs - dbevel) < w_eps) {
                if(calc_intersection(p1->x + dx1, p1->y - dy1, p2->x + dx1, p2->y - dy1,
                                     p2->x + dx2, p2->y - dy2, p3->x + dx2, p3->y - dy2, &dx, &dy)) {
                    add_out_point(cs, dx, dy);
                }
                else {
                    add_out_point(cs, p2->x + dx1, p2->y - dy1);
                }
                return;
            }
        }

        switch(dsc->join) {
            case LV_VECTOR_STROKE_JOIN_MITER:
                calc_miter(gen, p1, p2, p3, dx1, dy1, dx2, dy2, dbevel);
                break;
            case LV_VECTOR_STROKE_JOIN_ROUND:
                calc_arc(gen, p2->x, p2->y, dx1, -dy1, dx2, -dy2);
                break;
            default: // Bevel join
                add_out_point(cs, p2->x + dx1, p2->y - dy1);
                add_out_point(cs, p2->x + dx2, p2->y - dy2);
                break;
        }
    }
}

/* line generator callbacks */
static void stroke_line_reset(struct _base_generator * gen)
{
    lv_line_generator * line_gen = (lv_line_generator *)gen;
    if(line_gen->state == LV_LINE_GEN_INITIAL) {
        lv_array_t * arr = &line_gen->dist_arr;
        close_distance(arr, line_gen->closed);

        if(lv_array_size(arr) < 3) {
            line_gen->closed = false;
        }
    }

    line_gen->state = LV_LINE_GEN_READY;
    line_gen->dist_idx = 0;
    line_gen->out_idx = 0;
}

static void stroke_line_clear(struct _base_generator * gen)
{
    lv_line_generator * line_gen = (lv_line_generator *)gen;
    lv_array_clear(&line_gen->dist_arr);
    line_gen->state = LV_LINE_GEN_INITIAL;
    line_gen->closed = false;
}

static void stroke_line_accumulate(struct _base_generator * gen, const lv_fpoint_t * point, lv_vector_path_op_t op)
{
    lv_line_generator * line_gen = (lv_line_generator *)gen;

    line_gen->state = LV_LINE_GEN_INITIAL;

    if(op == LV_VECTOR_PATH_OP_MOVE_TO) {
        lv_distance_point_t dp = {.x = point->x, .y = point->y, .d = 0.0f};
        // modify previous last point
        lv_array_t * arr = &line_gen->dist_arr;
        remove_last_distance_point(arr);
        add_distance_point(arr, &dp);
    }
    else {
        if(op < LV_VECTOR_PATH_OP_CLOSE) {
            lv_distance_point_t dp = {.x = point->x, .y = point->y, .d = 0.0f};
            add_distance_point(&line_gen->dist_arr, &dp);
        }
        else {
            if(op == LV_VECTOR_PATH_OP_CLOSE) {
                line_gen->closed = true;
            }
        }
    }
}

static bool stroke_line_generated(struct _base_generator * gen, lv_fpoint_t * point, lv_vector_path_op_t * op)
{
    lv_line_generator * line_gen = (lv_line_generator *)gen;

    uint8_t cmd = LV_VECTOR_PATH_OP_LINE_TO;
    lv_array_t * array = &line_gen->dist_arr;

    while(cmd != LV_VECTOR_POLYGON_STOP) {
        lv_line_gen_state_t state = line_gen->state;
        switch(state) {
            case LV_LINE_GEN_INITIAL: {
                    gen->reset(gen);
                }
            /* fall through */
            case LV_LINE_GEN_READY: {
                    size_t size = lv_array_size(array);
                    if(size < 2 + (line_gen->closed ? 1 : 0)) {
                        *op = LV_VECTOR_POLYGON_STOP;
                        return false;
                    }
                    line_gen->state = line_gen->closed ? LV_LINE_GEN_FIRST_OUTLINE : LV_LINE_GEN_START_CAP;
                    line_gen->dist_idx = 0;
                    line_gen->out_idx = 0;
                    cmd = LV_VECTOR_PATH_OP_MOVE_TO;
                }
                break;
            case LV_LINE_GEN_START_CAP: {
                    lv_distance_point_t * p1 = (lv_distance_point_t *)lv_array_at(array, 0);
                    lv_distance_point_t * p2 = (lv_distance_point_t *)lv_array_at(array, 1);
                    calc_cap(line_gen, p1, p2, p1->d);

                    line_gen->prev_state = LV_LINE_GEN_FIRST_OUTLINE;
                    line_gen->state = LV_LINE_GEN_OUTPUT;
                    line_gen->dist_idx = 1;
                    line_gen->out_idx = 0;
                }
                break;
            case LV_LINE_GEN_END_CAP: {
                    size_t size = lv_array_size(array);
                    lv_distance_point_t * p1 = (lv_distance_point_t *)lv_array_at(array, size - 1);
                    lv_distance_point_t * p2 = (lv_distance_point_t *)lv_array_at(array, size - 2);
                    calc_cap(line_gen, p1, p2, p2->d);

                    line_gen->prev_state = LV_LINE_GEN_SECOND_OUTLINE;
                    line_gen->state = LV_LINE_GEN_OUTPUT;
                    line_gen->out_idx = 0;
                }
                break;
            case LV_LINE_GEN_FIRST_OUTLINE: {
                    size_t size = lv_array_size(array);
                    if(line_gen->closed) {
                        if(line_gen->dist_idx >= size) {
                            line_gen->prev_state = LV_LINE_GEN_FIRST_CLOSE;
                            line_gen->state = LV_LINE_GEN_END_POLYGON;
                            break;
                        }
                    }
                    else {
                        if(line_gen->dist_idx >= (size - 1)) {
                            line_gen->state = LV_LINE_GEN_END_CAP;
                            break;
                        }
                    }

                    uint32_t idx = line_gen->dist_idx;
                    uint32_t idx_next = (line_gen->dist_idx + 1) % size;
                    uint32_t idx_prev = (line_gen->dist_idx + size - 1) % size;

                    lv_distance_point_t * pc = (lv_distance_point_t *)lv_array_at(array, idx);
                    lv_distance_point_t * pp = (lv_distance_point_t *)lv_array_at(array, idx_prev);
                    lv_distance_point_t * pn = (lv_distance_point_t *)lv_array_at(array, idx_next);
                    calc_join(line_gen, pp, pc, pn, pp->d, pc->d);

                    line_gen->dist_idx++;
                    line_gen->prev_state = line_gen->state;
                    line_gen->state = LV_LINE_GEN_OUTPUT;
                    line_gen->out_idx = 0;
                }
                break;
            case LV_LINE_GEN_FIRST_CLOSE: {
                    line_gen->state = LV_LINE_GEN_SECOND_OUTLINE;
                    cmd = LV_VECTOR_PATH_OP_MOVE_TO;
                }
            /* fall through */
            case LV_LINE_GEN_SECOND_OUTLINE: {
                    if(line_gen->dist_idx <= (line_gen->closed ? 0 : 1)) {
                        line_gen->state = LV_LINE_GEN_END_POLYGON;
                        line_gen->prev_state = LV_LINE_GEN_STOP;
                        break;
                    }

                    line_gen->dist_idx--;

                    size_t size = lv_array_size(array);
                    uint32_t idx = line_gen->dist_idx;
                    uint32_t idx_next = (line_gen->dist_idx + 1) % size;
                    uint32_t idx_prev = (line_gen->dist_idx + size - 1) % size;

                    lv_distance_point_t * pc = (lv_distance_point_t *)lv_array_at(array, idx);
                    lv_distance_point_t * pp = (lv_distance_point_t *)lv_array_at(array, idx_prev);
                    lv_distance_point_t * pn = (lv_distance_point_t *)lv_array_at(array, idx_next);
                    calc_join(line_gen, pn, pc, pp, pc->d, pp->d);

                    line_gen->prev_state = line_gen->state;
                    line_gen->state = LV_LINE_GEN_OUTPUT;
                    line_gen->out_idx = 0;
                }
                break;
            case LV_LINE_GEN_END_POLYGON: {
                    line_gen->state = line_gen->prev_state;
                    *op = LV_VECTOR_PATH_OP_CLOSE;
                    return true;
                }
                break;
            case LV_LINE_GEN_OUTPUT: {
                    if(line_gen->out_idx >= lv_array_size(&line_gen->out_arr)) {
                        line_gen->state = line_gen->prev_state;
                    }
                    else {
                        *point = *((lv_fpoint_t *)lv_array_at(&line_gen->out_arr, line_gen->out_idx++));
                        *op = cmd;
                        return true;
                    }
                }
                break;
            case LV_LINE_GEN_STOP:
                cmd = LV_VECTOR_POLYGON_STOP;
                break;
        }
    }
    *op = cmd;
    return false;
}

static void dash_line_reset(struct _base_generator * gen)
{
    lv_line_generator * line_gen = (lv_line_generator *)gen;
    if(line_gen->state == LV_LINE_GEN_INITIAL) {
        lv_array_t * arr = &line_gen->dist_arr;
        close_distance(arr, line_gen->closed);
    }

    line_gen->state = LV_LINE_GEN_READY;
    line_gen->dist_idx = 0;
}

static void calc_dash_offset(lv_dash_generator * gen, float offset)
{
    gen->cur_dash_idx = 0;
    gen->cur_dash_offset = 0.0f;

    while(offset > 0.0f) {
        float dash = *((float *)lv_array_at(gen->dash_arr, gen->cur_dash_idx));

        if(offset > dash) {
            offset -= dash;
            gen->cur_dash_idx++;
            gen->cur_dash_offset = 0.0f;

            if(gen->cur_dash_idx >= lv_array_size(gen->dash_arr)) {
                gen->cur_dash_idx = 0;
            }
        }
        else {
            gen->cur_dash_offset = offset;
            offset = 0.0f;
        }
    }
}

static bool dash_line_generated(struct _base_generator * gen, lv_fpoint_t * point, lv_vector_path_op_t * op)
{
    lv_dash_generator * dash_gen = (lv_dash_generator *)gen;

    uint8_t cmd = LV_VECTOR_PATH_OP_MOVE_TO;
    lv_array_t * array = &LINE_GEN(gen)->dist_arr;
    lv_array_t * dash_array = dash_gen->dash_arr;

    lv_line_generator * line_gen = LINE_GEN(gen);

    while(cmd != LV_VECTOR_POLYGON_STOP) {
        lv_line_gen_state_t state = line_gen->state;
        switch(state) {
            case LV_LINE_GEN_INITIAL: {
                    gen->reset(gen);
                }
            /* fall through */
            case LV_LINE_GEN_READY: {
                    size_t size = lv_array_size(array);
                    if(size < 2 || lv_array_size(dash_array) < 2) {
                        *op = LV_VECTOR_POLYGON_STOP;
                        return false;
                    }
                    line_gen->state = LV_LINE_GEN_FIRST_OUTLINE;
                    line_gen->dist_idx = 1;
                    // dash attributes
                    dash_gen->p1 = (lv_distance_point_t *)lv_array_at(array, 0);
                    dash_gen->p2 = (lv_distance_point_t *)lv_array_at(array, 1);
                    dash_gen->distance = dash_gen->p1->d;

                    point->x = dash_gen->p1->x;
                    point->y = dash_gen->p1->y;
                    *op = LV_VECTOR_PATH_OP_MOVE_TO;

                    if(dash_gen->dash_offset >= 0.0f) {
                        calc_dash_offset(dash_gen, dash_gen->dash_offset);
                    }
                    return true;
                }
            case LV_LINE_GEN_FIRST_OUTLINE: {
                    float dash_distance = *((float *)lv_array_at(dash_array, dash_gen->cur_dash_idx));
                    dash_distance -= dash_gen->cur_dash_offset;

                    lv_vector_path_op_t opt = (dash_gen->cur_dash_idx & 0x1) ? LV_VECTOR_PATH_OP_MOVE_TO : LV_VECTOR_PATH_OP_LINE_TO;

                    if(dash_gen->distance > dash_distance) {
                        dash_gen->distance -= dash_distance;
                        dash_gen->cur_dash_idx++;

                        if(dash_gen->cur_dash_idx >= lv_array_size(dash_array)) {
                            dash_gen->cur_dash_idx = 0;
                        }
                        dash_gen->cur_dash_offset = 0.0f;
                        point->x = dash_gen->p2->x - (dash_gen->p2->x - dash_gen->p1->x) * dash_gen->distance / dash_gen->p1->d;
                        point->y = dash_gen->p2->y - (dash_gen->p2->y - dash_gen->p1->y) * dash_gen->distance / dash_gen->p1->d;
                    }
                    else {
                        dash_gen->cur_dash_offset += dash_gen->distance;
                        point->x = dash_gen->p2->x;
                        point->y = dash_gen->p2->y;
                        dash_gen->p1 = dash_gen->p2;
                        dash_gen->distance = dash_gen->p1->d;
                        line_gen->dist_idx++;

                        if(line_gen->closed) {
                            if(line_gen->dist_idx > lv_array_size(array)) {
                                line_gen->state = LV_LINE_GEN_STOP;
                            }
                            else {
                                uint32_t idx = (line_gen->dist_idx == lv_array_size(array)) ? 0 : line_gen->dist_idx;
                                dash_gen->p2 = (lv_distance_point_t *)lv_array_at(array, idx);
                            }
                        }
                        else {
                            if(line_gen->dist_idx >= lv_array_size(array)) {
                                line_gen->state = LV_LINE_GEN_STOP;
                            }
                            else {
                                dash_gen->p2 = (lv_distance_point_t *)lv_array_at(array, line_gen->dist_idx);
                            }
                        }
                    }
                    *op = opt;
                    return true;
                }
                break;
            case LV_LINE_GEN_STOP:
                cmd = LV_VECTOR_POLYGON_STOP;
                break;
        }
    }
    *op = cmd;
    return false;
}

static void path_to_stroke_cb(lv_vector_path_op_t op, const lv_fpoint_t * pt, void * data)
{
    lv_base_generator * gen = (lv_base_generator *)data;
    lv_base_gen_state_t state;
start:
    state = gen->state;

    switch(state) {
        case LV_BASE_GEN_INITIAL: {
                gen->last_op = op;
                gen->start_point = *pt;
                gen->state = LV_BASE_GEN_START_ACCUMULATE;
            }
        /* fall through */
        case LV_BASE_GEN_START_ACCUMULATE: {
                if(gen->last_op == LV_VECTOR_POLYGON_STOP) {
                    gen->cb(gen->last_op, NULL, gen->user_data);
                    return;
                }

                gen->clear(gen);
                gen->accumulate(gen, &gen->start_point, LV_VECTOR_PATH_OP_MOVE_TO); // store start point
                gen->state = LV_BASE_GEN_ACCUMULATE;
            }
            break;
        case LV_BASE_GEN_ACCUMULATE: {
                gen->last_op = op;
                if(op < LV_VECTOR_PATH_OP_CLOSE) {
                    if(op == LV_VECTOR_PATH_OP_MOVE_TO) {
                        gen->start_point = *pt;
                        gen->state = LV_BASE_GEN_GENERATE;
                    }
                    else {
                        gen->accumulate(gen, pt, op);
                        return;
                    }
                }
                else {
                    if(gen->last_op != LV_VECTOR_POLYGON_STOP) {
                        gen->accumulate(gen, pt, op);
                        gen->state = LV_BASE_GEN_GENERATE;
                    }
                }
            }
        /* fall through */
        case LV_BASE_GEN_GENERATE: {
                gen->reset(gen);
                lv_fpoint_t p = {0};
                lv_vector_path_op_t cmd;
                while(gen->generated(gen, &p, &cmd)) {
                    gen->cb(cmd, &p, gen->user_data);
                }
                gen->state = LV_BASE_GEN_START_ACCUMULATE;
                goto start;
            }
            break;
    }
}

static void lv_line_generator_init(lv_line_generator * gen, const lv_vector_stroke_dsc_t * stroke_dsc)
{
    lv_base_generator_init(BASE_GEN(gen));
    BASE_GEN(gen)->dsc = stroke_dsc;
    BASE_GEN(gen)->reset = stroke_line_reset;
    BASE_GEN(gen)->clear = stroke_line_clear;
    BASE_GEN(gen)->accumulate = stroke_line_accumulate;
    BASE_GEN(gen)->generated = stroke_line_generated;

    lv_array_init(&gen->dist_arr, DEFAULT_DISTANCE_NUM, sizeof(lv_distance_point_t));
    lv_array_init(&gen->out_arr, DEFAULT_DISTANCE_NUM, sizeof(lv_fpoint_t));
    gen->state = LV_LINE_GEN_INITIAL;
    gen->prev_state = LV_LINE_GEN_INITIAL;
    gen->dist_idx = 0;
    gen->out_idx = 0;
    gen->closed = false;

    gen->half_width = stroke_dsc->width * 0.5f;
    if(gen->half_width < 0) {
        gen->w_abs = -gen->half_width;
        gen->w_sign = -1;
    }
    else {
        gen->w_abs = gen->half_width;
        gen->w_sign = 1;
    }
    gen->w_eps = gen->half_width / 1024.0f;
}

static void lv_line_generator_deinit(lv_line_generator * gen)
{
    lv_array_deinit(&gen->dist_arr);
    lv_array_deinit(&gen->out_arr);
}

static void lv_dash_generator_init(lv_dash_generator * gen, const lv_vector_stroke_dsc_t * stroke_dsc,
                                   const lv_line_generator * line_gen)
{
    lv_line_generator_init(LINE_GEN(gen), stroke_dsc);
    BASE_GEN(gen)->reset = dash_line_reset;
    BASE_GEN(gen)->generated = dash_line_generated;
    BASE_GEN(gen)->cb = path_to_stroke_cb;
    BASE_GEN(gen)->user_data = (void *)line_gen;

    gen->dash_arr = (lv_array_t *)(&stroke_dsc->dash_pattern);
    gen->dash_offset = 0.0f;
    gen->p1 = NULL;
    gen->p2 = NULL;
    gen->distance = 0.0f;
    gen->cur_dash_offset = 0.0f;
    gen->cur_dash_idx = 0;
}

static void lv_dash_generator_deinit(lv_dash_generator * gen)
{
    lv_line_generator_deinit(LINE_GEN(gen));
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
bool lv_vector_stroke_generate(const lv_vector_path_t * path, const lv_vector_stroke_dsc_t * stroke_dsc,
                               path_generate_cb cb, void * user_data)
{
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(stroke_dsc);
    LV_ASSERT_NULL(cb);

    if(lv_array_size(&path->points) == 0) {
        LV_LOG_ERROR("path is empty!");
        return false;
    }

    if(stroke_dsc->width <= 0.0f) {
        LV_LOG_ERROR("stroke width is less than or equal to zero!");
        return false;
    }

    bool ret = false;

    lv_line_generator line_gen;
    lv_line_generator_init(&line_gen, stroke_dsc);

    BASE_GEN(&line_gen)->cb = cb;
    BASE_GEN(&line_gen)->user_data = user_data;

    lv_array_t * dash_array = (lv_array_t *)(&(stroke_dsc->dash_pattern));

    if(lv_array_size(dash_array) > 0) { /* dash line */
        lv_dash_generator dash_gen;
        lv_dash_generator_init(&dash_gen, stroke_dsc, &line_gen);
        ret = lv_vector_path_flatten(path, path_to_stroke_cb, &dash_gen);
        lv_dash_generator_deinit(&dash_gen);
    }
    else {   /* solid line */
        ret = lv_vector_path_flatten(path, path_to_stroke_cb, &line_gen);
    }
    lv_line_generator_deinit(&line_gen);
    return ret;
}

static void polygon_to_path(lv_vector_path_op_t op, const lv_fpoint_t * pt, void * data)
{
    lv_vector_path_t * fpath = (lv_vector_path_t *)data;

    switch(op) {
        case LV_VECTOR_PATH_OP_MOVE_TO: {
                lv_vector_path_move_to(fpath, pt);
            }
            break;
        case LV_VECTOR_PATH_OP_LINE_TO: {
                lv_vector_path_line_to(fpath, pt);
            }
            break;
        case LV_VECTOR_PATH_OP_CLOSE: {
                lv_vector_path_close(fpath);
            }
            break;
    }
}

bool lv_vector_stroke_to_path(lv_vector_path_t * result, const lv_vector_path_t * path,
                              const lv_vector_stroke_dsc_t * stroke_dsc)
{
    LV_ASSERT_NULL(result);
    LV_ASSERT_NULL(path);
    LV_ASSERT_NULL(stroke_dsc);

    if(lv_array_size(&path->points) == 0) {
        LV_LOG_ERROR("path is empty!");
        return false;
    }

    bool ret = false;
    lv_vector_path_clear(result);

    ret = lv_vector_stroke_generate(path, stroke_dsc, polygon_to_path, result);

    if(ret) {
        result->flags |= PATH_FLAG_POLYGON; // polygon flag
    }
    return ret;
}

#endif