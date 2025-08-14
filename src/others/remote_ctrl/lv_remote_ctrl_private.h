/**
 * @file lv_remote_ctrl_private.h
 *
 */

#ifndef LV_REMOTE_CTRL_PRIVATE_H
#define LV_REMOTE_CTRL_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lv_remote_ctrl.h"

#if LV_USE_REMOTE_CTRL

#include "../../misc/lv_log.h"
#include "../../stdlib/lv_sprintf.h"

/*********************
 *      DEFINES
 *********************/

/**
 * @brief Macro to export a remote control command class
 * @param NAME Command name (will be used as identifier prefix)
 * @param INSTANCE_SIZE Size of the context structure needed by this command (0 if no context needed)
 *
 * @note The macro expects the following callbacks to be defined:
 * - show_help_cb: Function to display command help
 * - constructor_cb: Context constructor (optional)
 * - destructor_cb: Context destructor (optional)
 * - execute_cb: Command execution function
 *
 * @example
 * - See lv_remote_templ.c for an example of how to use this macro
 */
#define LV_REMOTE_CTRL_CLASS_EXPORT(NAME, INSTAMCE_SIZE) \
    const lv_remote_ctrl_class_t lv_remote_ctrl_##NAME##_class = { \
                                                                   .name = #NAME, \
                                                                   .instance_size = INSTAMCE_SIZE, \
                                                                   .show_help_cb = show_help_cb, \
                                                                   .constructor_cb = constructor_cb, \
                                                                   .destructor_cb = destructor_cb, \
                                                                   .execute_cb = execute_cb, \
                                                                 };

/**********************
 *      TYPEDEFS
 **********************/

/**
 * Remote control command class definition
 * Defines the interface for a remote control command implementation
 */
typedef struct {
    /** Command name (must be unique) */
    const char * name;

    /** Size of the context structure needed by this command (0 if no context needed) */
    size_t instance_size;

    /**
     * Callback to show command help information
     * @param print_func Function to use for outputting help text
     */
    void (*show_help_cb)(lv_remote_ctrl_print_func_t print_func);

    /**
     * Constructor callback (optional)
     * Called when the remote control context is created
     * @param ctx Command-specific context
     */
    void (*constructor_cb)(void * ctx);

    /**
     * Destructor callback (optional)
     * Called when the remote control context is destroyed
     * @param ctx Command-specific context
     */
    void (*destructor_cb)(void * ctx);

    /**
     * Command execution callback
     * @param ctx Command-specific context
     * @param argc Number of arguments
     * @param argv Array of argument strings
     * @return Execution result (LV_RESULT_OK on success)
     */
    lv_result_t (*execute_cb)(void * ctx, int argc, const char * argv[]);
} lv_remote_ctrl_class_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_REMOTE_CTRL*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_REMOTE_CTRL_PRIVATE_H*/
