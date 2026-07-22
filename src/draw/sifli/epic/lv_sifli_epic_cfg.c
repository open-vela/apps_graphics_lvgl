/**
 * @file lv_sifli_epic_cfg.c
 *
 */

/**
 * Copyright 2024 SiFli Technologies
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_sifli_epic_cfg.h"
#include "lv_sifli_epic_osa.h"

#if LV_USE_SIFLI_EPIC
#include "../../../misc/lv_log.h"
#include <inttypes.h>
#include "system_bf0_ap.h"
#include <string.h>
#include <syslog.h>

#if defined(__ZEPHYR__)
    #include <zephyr/arch/cache.h>
#endif

#if defined(__NuttX__)
    #include <arch/irq.h>
    #include <nuttx/arch.h>
    #include <nuttx/cache.h>
    #include <nuttx/clock.h>

    #ifndef NVIC_IRQ_FIRST
        #define NVIC_IRQ_FIRST 16
    #endif
#endif

/**********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static EPIC_HandleTypeDef epic_handle;
#ifdef HAL_EZIP_MODULE_ENABLED
    static EZIP_HandleTypeDef ezip_handle;
#endif
static bool epic_initialized = false;
static bool epic_cont_active = false;
static lv_epic_cplt_cbk epic_async_cb;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void epic_xfer_cplt_callback(EPIC_HandleTypeDef * epic);
static HAL_StatusTypeDef epic_wait_async_result(HAL_StatusTypeDef start_status);
static HAL_StatusTypeDef epic_prepare_start(EPIC_HandleTypeDef * epic,
                                            lv_epic_cplt_cbk cb);
static void epic_start_failed_cleanup(void);
static HAL_StatusTypeDef epic_cont_wait_done(const char * operation);
static HAL_StatusTypeDef epic_recover(void);
static uint32_t epic_tick_get(void);

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_epic_init(void)
{
    if(epic_initialized) {
        return;
    }

    memset(&epic_handle, 0, sizeof(epic_handle));
    epic_handle.Instance = LV_SIFLI_EPIC_INSTANCE;

#ifdef HAL_EZIP_MODULE_ENABLED
    memset(&ezip_handle, 0, sizeof(ezip_handle));
    ezip_handle.Instance = EZIP;
    epic_handle.hezip = &ezip_handle;

    if(HAL_EZIP_Init(epic_handle.hezip) != HAL_OK) {
        return;
    }
#endif

    if(HAL_EPIC_Init(&epic_handle) != HAL_OK) {
        return;
    }

    if(lv_epic_osa_init() != LV_RESULT_OK) {
        memset(&epic_handle, 0, sizeof(epic_handle));
#ifdef HAL_EZIP_MODULE_ENABLED
        memset(&ezip_handle, 0, sizeof(ezip_handle));
#endif
        return;
    }

    epic_initialized = true;
    epic_cont_active = false;
}

void lv_epic_deinit(void)
{
    if(!epic_initialized) {
        return;
    }

    lv_epic_wait();
    lv_epic_osa_deinit();

    memset(&epic_handle, 0, sizeof(epic_handle));
#ifdef HAL_EZIP_MODULE_ENABLED
        memset(&ezip_handle, 0, sizeof(ezip_handle));
#endif
    epic_async_cb = NULL;
    epic_cont_active = false;
    epic_initialized = false;
}

bool lv_epic_is_initialized(void)
{
    return epic_initialized;
}

bool lv_epic_is_busy(void)
{
    if(!epic_initialized) {
        return false;
    }

    if(epic_handle.State == HAL_EPIC_STATE_BUSY) {
        return true;
    }

#ifdef HAL_EZIP_MODULE_ENABLED
    if(ezip_handle.State == HAL_EZIP_STATE_BUSY) {
        return true;
    }
#endif

    return false;
}

void lv_epic_run(void)
{
    epic_osa_cfg_t * cfg = epic_get_default_cfg();
    if(cfg && cfg->epic_run) {
        cfg->epic_run();
    }
}

HAL_StatusTypeDef lv_epic_wait(void)
{
    if(epic_cont_active) {
        return lv_epic_cont_blend_reset();
    }

    epic_osa_cfg_t * cfg = epic_get_default_cfg();
    if(cfg && cfg->epic_wait) {
        cfg->epic_wait();
    }

    if(lv_epic_osa_take_wait_failure()) {
        if(epic_handle.Instance != NULL) {
            syslog(LOG_ERR,
                   "[lv_epic][wait_timeout] status=%08" PRIx32
                   " command=%08" PRIx32 " setting=%08" PRIx32
                   " state=%d error=%08" PRIx32 "\n",
                   epic_handle.Instance->STATUS,
                   epic_handle.Instance->COMMAND,
                   epic_handle.Instance->SETTING,
                   (int)epic_handle.State, epic_handle.ErrorCode);
        }
        return epic_recover() == HAL_OK ? HAL_TIMEOUT : HAL_ERROR;
    }

    return HAL_OK;
}

EPIC_HandleTypeDef * lv_epic_get_handle(void)
{
    if(!epic_initialized) {
        return NULL;
    }

    return &epic_handle;
}

#ifdef HAL_EZIP_MODULE_ENABLED
EZIP_HandleTypeDef * lv_ezip_get_handle(void)
{
    if(!epic_initialized) {
        return NULL;
    }

    return &ezip_handle;
}
#endif

void lv_epic_flush_cache_range(const void * data, uint32_t size)
{
    if(data == NULL || size == 0U) {
        return;
    }

#if defined(__ZEPHYR__)
    (void)arch_dcache_flush_range((void *)data, size);
#elif defined(__NuttX__)
    /* SF32LB52 configures PSRAM as write-through.  The SDK helper makes
     * clean a no-op in that mode and keeps the write-back case correct. */
    (void)mpu_dcache_clean((void *)data, size);
#else
    mpu_dcache_clean((void *)data, size);
#endif
}

void lv_epic_invalidate_cache_range(const void * data, uint32_t size)
{
    if(data == NULL || size == 0U) {
        return;
    }

#if defined(__ZEPHYR__)
    (void)arch_dcache_invd_range((void *)data, size);
#elif defined(__NuttX__)
    /* Use the SiFli cache-size-aware path.  It invalidates the whole 16 KiB
     * D-cache for larger regions instead of walking every address line. */
    (void)mpu_dcache_invalidate((void *)data, size);
#else
    mpu_dcache_invalidate((void *)data, size);
#endif
}

bool lv_epic_is_cached_ram(uint32_t start, uint32_t len)
{
    LV_UNUSED(len);

#if defined(__NuttX__)
    return IS_DCACHED_RAM(start);
#elif defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    LV_UNUSED(start);
    return true;
#else
    LV_UNUSED(start);
    return false;
#endif
}

HAL_StatusTypeDef lv_epic_fill(EPIC_LayerConfigTypeDef * input_layers, uint8_t input_layer_cnt,
                               EPIC_LayerConfigTypeDef * output_layer)
{
    EPIC_HandleTypeDef * epic = lv_epic_get_handle();

    if(epic_prepare_start(epic, NULL) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_StatusTypeDef start_status = HAL_EPIC_BlendStartEx_IT(epic, input_layers, input_layer_cnt, output_layer);
    return epic_wait_async_result(start_status);
}

HAL_StatusTypeDef lv_epic_blend(EPIC_LayerConfigTypeDef * input_layers, uint8_t input_layer_cnt,
                                EPIC_LayerConfigTypeDef * output_layer)
{
    EPIC_HandleTypeDef * epic = lv_epic_get_handle();

    if(epic_prepare_start(epic, NULL) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_StatusTypeDef start_status = HAL_EPIC_BlendStartEx_IT(epic, input_layers, input_layer_cnt, output_layer);
    return epic_wait_async_result(start_status);
}

HAL_StatusTypeDef lv_epic_cont_blend(EPIC_LayerConfigTypeDef * input_layers, uint8_t input_layer_cnt,
                                     EPIC_LayerConfigTypeDef * output_layer)
{
    EPIC_HandleTypeDef * epic = lv_epic_get_handle();
    HAL_StatusTypeDef status;

    if(epic == NULL || input_layers == NULL || output_layer == NULL ||
       (input_layer_cnt != 2U && input_layer_cnt != 3U)) {
        return HAL_ERROR;
    }

    EPIC_LayerConfigTypeDef * fg_layer = &input_layers[1];
    EPIC_LayerConfigTypeDef * mask_layer = input_layer_cnt == 3U ? &input_layers[2] : NULL;

    if(!epic_cont_active) {
        if(epic->State == HAL_EPIC_STATE_BUSY) {
            if(lv_epic_wait() != HAL_OK) {
                return HAL_ERROR;
            }
        }

        epic_async_cb = NULL;
        epic->XferCpltCallback = NULL;
        epic->IntXferCpltCallback = NULL;
        status = HAL_EPIC_ContBlendStart(epic, fg_layer, mask_layer, output_layer);
        if(status == HAL_OK) {
            epic_cont_active = true;
        }
    }
    else {
        status = epic_cont_wait_done("repeat");
        if(status == HAL_OK) {
            status = HAL_EPIC_ContBlendRepeat(epic, fg_layer, mask_layer, output_layer);
        }
    }

    if(status != HAL_OK) {
        if(epic_cont_active) {
            (void)HAL_EPIC_ContBlendStop(epic);
        }
        epic_cont_active = false;
        lv_epic_osa_set_idle();
    }

    return status;
}

HAL_StatusTypeDef lv_epic_cont_blend_reset(void)
{
    if(!epic_cont_active) {
        return HAL_OK;
    }

    HAL_StatusTypeDef status = epic_cont_wait_done("stop");
    if(status == HAL_OK) {
        status = HAL_EPIC_ContBlendStop(&epic_handle);
    }
    epic_cont_active = false;
    lv_epic_osa_set_idle();
    return status;
}

HAL_StatusTypeDef lv_epic_fill_grad(EPIC_GradCfgTypeDef * param)
{
    EPIC_HandleTypeDef * epic = lv_epic_get_handle();

    if(epic_prepare_start(epic, NULL) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_StatusTypeDef start_status = HAL_EPIC_FillGrad_IT(epic, param);
    return epic_wait_async_result(start_status);
}

HAL_StatusTypeDef lv_epic_copy(EPIC_BlendingDataType * src, EPIC_BlendingDataType * dst)
{
    EPIC_HandleTypeDef * epic = lv_epic_get_handle();

    if(epic_prepare_start(epic, NULL) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_StatusTypeDef start_status = HAL_EPIC_Copy_IT(epic, src, dst);
    return epic_wait_async_result(start_status);
}

HAL_StatusTypeDef lv_epic_copy_async(EPIC_BlendingDataType * src, EPIC_BlendingDataType * dst,
                                     lv_epic_cplt_cbk cb)
{
    EPIC_HandleTypeDef * epic = lv_epic_get_handle();

    if(epic_prepare_start(epic, cb) != HAL_OK) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef start_status = HAL_EPIC_Copy_IT(epic, src, dst);
    if(start_status != HAL_OK) {
        LV_LOG_WARN("EPIC async copy start failed: status=%d", (int)start_status);
        epic_start_failed_cleanup();
    }

    return start_status;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void epic_xfer_cplt_callback(EPIC_HandleTypeDef * epic)
{
    lv_epic_osa_thread_sync_signal_isr();

    if(epic_async_cb != NULL) {
        lv_epic_cplt_cbk cb = epic_async_cb;
        epic_async_cb = NULL;
        cb(epic);
    }
}

static HAL_StatusTypeDef epic_wait_async_result(HAL_StatusTypeDef start_status)
{
    if(start_status != HAL_OK) {
        LV_LOG_WARN("EPIC start failed: status=%d", (int)start_status);
        epic_start_failed_cleanup();
        return start_status;
    }

    return lv_epic_wait();
}

static HAL_StatusTypeDef epic_prepare_start(EPIC_HandleTypeDef * epic,
                                            lv_epic_cplt_cbk cb)
{
    if(epic == NULL) {
        return HAL_ERROR;
    }

    if(lv_epic_is_busy()) {
        if(lv_epic_wait() != HAL_OK) {
            return HAL_ERROR;
        }
    }

    epic_async_cb = cb;
    epic->XferCpltCallback = epic_xfer_cplt_callback;
    lv_epic_run();
    return HAL_OK;
}

static void epic_start_failed_cleanup(void)
{
    epic_handle.XferCpltCallback = NULL;
    epic_async_cb = NULL;
    lv_epic_osa_set_idle();
}

static HAL_StatusTypeDef epic_cont_wait_done(const char * operation)
{
    uint32_t spin_count = 0;
    uint32_t start_tick;

    if(!HAL_EPIC_IsHWBusy(&epic_handle)) {
        return HAL_OK;
    }

    start_tick = epic_tick_get();

    while(HAL_EPIC_IsHWBusy(&epic_handle)) {
        if((++spin_count & 0xffU) == 0U &&
           epic_tick_get() - start_tick >= LV_SIFLI_EPIC_CONT_TIMEOUT_MS) {
            syslog(LOG_ERR,
                   "[lv_epic][cont_timeout] op=%s elapsed=%" PRIu32
                   "ms status=%08" PRIx32 " command=%08" PRIx32
                   " setting=%08" PRIx32 " state=%d error=%08" PRIx32 "\n",
                   operation, epic_tick_get() - start_tick,
                   epic_handle.Instance->STATUS,
                   epic_handle.Instance->COMMAND,
                   epic_handle.Instance->SETTING,
                   (int)epic_handle.State, epic_handle.ErrorCode);
            (void)epic_recover();
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

static uint32_t epic_tick_get(void)
{
#if defined(__NuttX__)
    return (uint32_t)TICK2MSEC(clock_systime_ticks());
#else
    return HAL_GetTick();
#endif
}

static HAL_StatusTypeDef epic_recover(void)
{
    HAL_StatusTypeDef status;

#if defined(__NuttX__)
    int nuttx_irq = (int)LV_SIFLI_EPIC_IRQn + NVIC_IRQ_FIRST;
    up_disable_irq(nuttx_irq);
#else
    HAL_NVIC_DisableIRQ(LV_SIFLI_EPIC_IRQn);
#endif
    HAL_NVIC_ClearPendingIRQ(LV_SIFLI_EPIC_IRQn);
    HAL_RCC_ResetModule(RCC_MOD_EPIC);

    memset(&epic_handle, 0, sizeof(epic_handle));
    epic_handle.Instance = LV_SIFLI_EPIC_INSTANCE;
#ifdef HAL_EZIP_MODULE_ENABLED
    epic_handle.hezip = &ezip_handle;
#endif
    status = HAL_EPIC_Init(&epic_handle);

    epic_async_cb = NULL;
    epic_cont_active = false;
    lv_epic_osa_set_idle();

    if(status == HAL_OK) {
        HAL_NVIC_ClearPendingIRQ(LV_SIFLI_EPIC_IRQn);
#if defined(__NuttX__)
        up_enable_irq(nuttx_irq);
#else
        HAL_NVIC_EnableIRQ(LV_SIFLI_EPIC_IRQn);
#endif
        syslog(LOG_WARNING, "[lv_epic][recover] EPIC reinitialized\n");
    }
    else {
        epic_initialized = false;
        syslog(LOG_ERR, "[lv_epic][recover] EPIC reinit failed: %d\n", (int)status);
    }

    return status;
}

#endif /*LV_USE_SIFLI_EPIC*/
