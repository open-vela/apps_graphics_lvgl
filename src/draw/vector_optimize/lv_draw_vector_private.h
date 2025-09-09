/**
 * @file lv_draw_vector_private.h
 *
 */

#ifndef LV_DRAW_VECTOR_PRIV_H
#define LV_DRAW_VECTOR_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_vector.h"

#if LV_USE_VECTOR_GRAPHIC_OPTIMIZE

/**********************
 *      TYPEDEFS
 **********************/
enum {
    PATH_FLAG_POLYGON = 0x1,
    PATH_FLAG_CHANGED = 0x2
};
typedef uint8_t lv_vector_path_flags_t;

struct lv_platform_path_base_t;

typedef struct _lv_platform_path_handlers lv_platform_path_handlers;
typedef struct lv_platform_path_base_t lv_platform_path_base_t;

typedef struct {
    lv_array_t ops;
    lv_array_t points;
} lv_vector_path_data_t;

typedef void (*path_generate_callback)(lv_vector_path_op_t op, const lv_fpoint_t * point, void * data);

typedef struct {
    void * user_data;
    path_generate_callback cb;
    lv_fpoint_t last_point;
} lv_vector_path_transform_data_t;

struct _lv_vector_path_t {
    struct lv_platform_path_base_t * impl;
};

struct _lv_platform_path_handlers {
    struct lv_platform_path_base_t * (*create)(lv_vector_path_quality_t quality);
    void (*destroy)(struct lv_platform_path_base_t * self);
    struct lv_platform_path_base_t * (*clone)(struct lv_platform_path_base_t * self);
    void (*concat)(struct lv_platform_path_base_t * self, struct lv_platform_path_base_t * other);
    void (*move_to)(struct lv_platform_path_base_t * self, const lv_fpoint_t * p);
    void (*line_to)(struct lv_platform_path_base_t * self, const lv_fpoint_t * p);
    void (*quad_to)(struct lv_platform_path_base_t * self, const lv_fpoint_t * p1, const lv_fpoint_t * p2);
    void (*cubic_to)(struct lv_platform_path_base_t * self, const lv_fpoint_t * p1, const lv_fpoint_t * p2,
                     const lv_fpoint_t * p3);
    void (*close_path)(struct lv_platform_path_base_t * self);
    void (*clear)(struct lv_platform_path_base_t * self);
    void (*get_bounding)(struct lv_platform_path_base_t * self, lv_area_t * area);
    lv_vector_path_quality_t (*get_quality)(struct lv_platform_path_base_t * self);
    void (*transform)(struct lv_platform_path_base_t * self, const lv_matrix_t * matrix);
    void (*get_data)(struct lv_platform_path_base_t * self, lv_vector_path_data_t * data);
    void (*transform_path)(struct lv_platform_path_base_t * self, lv_vector_path_transform_data_t * transform_data);
    bool (*is_empty)(struct lv_platform_path_base_t * self);
    size_t (*get_mem_size)(struct lv_platform_path_base_t * self);
#if LV_USE_VECTOR_DUMP_INFO
    void (*dump_path_info)(struct lv_platform_path_base_t * self);
#endif
};

struct lv_platform_path_base_t {
    int16_t ref_count;
    lv_vector_path_flags_t flags;
    const lv_platform_path_handlers * handlers;
};

typedef struct {
    lv_platform_path_base_t * path_impl;
    lv_vector_draw_dsc_t * dsc;
} _lv_vector_draw_task;

/**
 * @brief Increment the reference count of the vector path object
 *
 * This function increases the reference counter associated with the path implementation object.
 * Each call to ref must have a corresponding unref call to ensure proper resource management.
 * @param impl  Pointer to the platform-specific path implementation object
 */
void lv_vector_path_ref(lv_platform_path_base_t * impl);

/**
 * @brief Decrement the reference count of the vector path object
 *
 * This function decreases the reference counter of the path implementation object.
 * When the reference count reaches zero, the associated resources will be automatically released.
 * @param impl  Pointer to the platform-specific path implementation object
 */
void lv_vector_path_unref(lv_platform_path_base_t * impl);

const lv_platform_path_handlers * lv_vector_get_platform_handlers(void);

/**
 * Retrieve underlying data of vector path (points, ops, etc.)
 * @param impl   Platform-specific path implementation object
 * @param data   Structure to receive path data
 */
void lv_vector_path_get_data(lv_platform_path_base_t * impl, lv_vector_path_data_t * data);

/**
 * Apply transform to vector path
 * @param impl       Platform-specific path implementation object
 * @param path_data  Transformation data to apply
 */
void lv_vector_path_transform_path(const lv_platform_path_base_t * impl, lv_vector_path_transform_data_t * path_data);

/**
 * Check if vector path contains no drawing elements
 * @param impl Platform-specific path implementation object
 * @return true if path contains no points or commands, false otherwise
 */
bool lv_vector_path_impl_is_empty(const lv_platform_path_base_t * impl);

#if LV_USE_VECTOR_DUMP_INFO
void lv_vector_dump_path_info(const lv_platform_path_base_t * impl);
void lv_vector_dump_dsc_info(const lv_vector_draw_dsc_t * dsc);
void lv_vector_for_each_task_dump_info(const lv_vector_draw_task_list_t * draw_task_list);
#endif

/* Traverser for task list */
typedef void (*vector_draw_task_cb)(void * ctx, const lv_platform_path_base_t * path, const lv_vector_draw_dsc_t * dsc);

void _lv_vector_for_each_destroy_tasks(lv_vector_draw_task_list_t * draw_task_list, vector_draw_task_cb cb,
                                       void * data);

#endif /* LV_USE_VECTOR_GRAPHIC_OPTIMIZE */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_DRAW_VECTOR_PRIV_H */
