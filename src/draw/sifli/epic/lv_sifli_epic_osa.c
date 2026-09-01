/**
 * @file lv_sifli_epic_osa.c
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

#include "lv_sifli_epic_osa.h"
#include "lv_sifli_epic_cfg.h"

#if LV_USE_SIFLI_EPIC
#include "../../../osal/lv_os.h"

#if defined(__ZEPHYR__)
    #include <zephyr/kernel.h>
    #include <zephyr/irq.h>
#endif

#if defined(__NuttX__)
    #include <nuttx/arch.h>
    #include <nuttx/clock.h>
    #include <nuttx/irq.h>
    #include <nuttx/semaphore.h>
    #include <arch/irq.h>
    #include <semaphore.h>
#endif

#if LV_USE_OS == LV_OS_RTTHREAD
    #include "rtthread.h"
#endif

/*********************
 *      DEFINES
 *********************/

#if defined(__NuttX__)
    #ifndef NVIC_IRQ_FIRST
        #define NVIC_IRQ_FIRST 16
    #endif
    #define LV_SIFLI_EPIC_NUTTX_IRQ(irqn) ((int)(irqn) + NVIC_IRQ_FIRST)
    #define LV_SIFLI_EPIC_WAIT_TIMEOUT_US 1000000
    #define LV_SIFLI_EPIC_WAIT_TIMEOUT_MS 1000U
#endif

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _epic_interrupt_init(void);
static void _epic_interrupt_deinit(void);
static void _epic_run(void);
static void _epic_wait(void);
static void _epic_irq_enter(void);
static void _epic_irq_leave(void);

#if defined(__ZEPHYR__)
    static void _epic_zephyr_irq_handler(const void * arg);
    #ifdef HAL_EZIP_MODULE_ENABLED
        static void _ezip_zephyr_irq_handler(const void * arg);
    #endif
#endif

#if defined(__NuttX__)
    static int _epic_nuttx_irq_handler(int irq, void * context, void * arg);
    #ifdef HAL_EZIP_MODULE_ENABLED
        static int _ezip_nuttx_irq_handler(int irq, void * context, void * arg);
    #endif
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

#if LV_USE_OS && !defined(__NuttX__)
    static lv_thread_sync_t epic_sync;
#endif
#if defined(__NuttX__)
    static sem_t epic_sem;
    static bool epic_sem_inited;
#endif
static volatile bool epic_idle = true;
#if defined(__NuttX__)
static volatile bool epic_wait_failure;
#endif

static epic_osa_cfg_t _epic_default_cfg = {
    .epic_interrupt_init = _epic_interrupt_init,
    .epic_interrupt_deinit = _epic_interrupt_deinit,
    .epic_run = _epic_run,
    .epic_wait = _epic_wait,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if defined(__ZEPHYR__)
static void _epic_zephyr_irq_handler(const void * arg)
{
    LV_UNUSED(arg);
    EPIC_IRQHandler();
}

#ifdef HAL_EZIP_MODULE_ENABLED
static void _ezip_zephyr_irq_handler(const void * arg)
{
    LV_UNUSED(arg);
    EZIP_IRQHandler();
}
#endif
#endif

#if defined(__NuttX__)
static int _epic_nuttx_irq_handler(int irq, void * context, void * arg)
{
    LV_UNUSED(irq);
    LV_UNUSED(context);
    LV_UNUSED(arg);
    EPIC_IRQHandler();
    return 0;
}

#ifdef HAL_EZIP_MODULE_ENABLED
static int _ezip_nuttx_irq_handler(int irq, void * context, void * arg)
{
    LV_UNUSED(irq);
    LV_UNUSED(context);
    LV_UNUSED(arg);
    EZIP_IRQHandler();
    return 0;
}
#endif
#endif

void EPIC_IRQHandler(void)
{
    _epic_irq_enter();

    if(lv_epic_is_initialized()) {
        EPIC_HandleTypeDef * epic_handle = lv_epic_get_handle();
        if(epic_handle != NULL) {
            HAL_EPIC_IRQHandler(epic_handle);
        }
    }

    _epic_irq_leave();
}

#ifdef HAL_EZIP_MODULE_ENABLED
void EZIP_IRQHandler(void)
{
    _epic_irq_enter();

    if(lv_epic_is_initialized()) {
        EZIP_HandleTypeDef * ezip_handle = lv_ezip_get_handle();
        if(ezip_handle != NULL) {
            HAL_EZIP_IRQHandler(ezip_handle);
        }
    }

    _epic_irq_leave();
}
#endif

epic_osa_cfg_t * epic_get_default_cfg(void)
{
    return &_epic_default_cfg;
}

lv_result_t lv_epic_osa_init(void)
{
#if defined(__NuttX__)
    epic_wait_failure = false;
#endif
#if LV_USE_OS
    if(lv_epic_osa_thread_sync_init() != LV_RESULT_OK) {
        return LV_RESULT_INVALID;
    }
#endif
    _epic_interrupt_init();
    return LV_RESULT_OK;
}

void lv_epic_osa_deinit(void)
{
    _epic_interrupt_deinit();
#if LV_USE_OS
    lv_epic_osa_thread_sync_delete();
#endif
}

lv_result_t lv_epic_osa_thread_sync_init(void)
{
#if defined(__NuttX__)
    if(sem_init(&epic_sem, 0, 0) < 0) {
        return LV_RESULT_INVALID;
    }
    epic_sem_inited = true;
#elif LV_USE_OS
    if(lv_thread_sync_init(&epic_sync) != LV_RESULT_OK) {
        return LV_RESULT_INVALID;
    }
#endif
    return LV_RESULT_OK;
}

lv_result_t lv_epic_osa_thread_sync_wait(void)
{
#if defined(__NuttX__)
    int ret;

    if(!epic_sem_inited) {
        return LV_RESULT_INVALID;
    }

    ret = nxsem_tickwait_uninterruptible(&epic_sem,
                                         MSEC2TICK(LV_SIFLI_EPIC_WAIT_TIMEOUT_MS));

    if(ret < 0) {
        epic_wait_failure = true;
        return LV_RESULT_INVALID;
    }
#elif LV_USE_OS
    if(lv_thread_sync_wait(&epic_sync) != LV_RESULT_OK) {
        return LV_RESULT_INVALID;
    }
#endif
    return LV_RESULT_OK;
}

void lv_epic_osa_thread_sync_signal_isr(void)
{
    epic_idle = true;
#if defined(__NuttX__)
    if(epic_sem_inited) {
        (void)sem_post(&epic_sem);
    }
#elif LV_USE_OS
    (void)lv_thread_sync_signal(&epic_sync);
#endif
}

void lv_epic_osa_set_idle(void)
{
    epic_idle = true;
}

bool lv_epic_osa_take_wait_failure(void)
{
#if defined(__NuttX__)
    bool failed = epic_wait_failure;
    epic_wait_failure = false;
    return failed;
#else
    return false;
#endif
}

void lv_epic_osa_thread_sync_delete(void)
{
#if defined(__NuttX__)
    if(epic_sem_inited) {
        (void)sem_destroy(&epic_sem);
        epic_sem_inited = false;
    }
#elif LV_USE_OS
    (void)lv_thread_sync_delete(&epic_sync);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _epic_irq_enter(void)
{
#if LV_USE_OS == LV_OS_RTTHREAD
    rt_interrupt_enter();
#endif
}

static void _epic_irq_leave(void)
{
#if LV_USE_OS == LV_OS_RTTHREAD
    rt_interrupt_leave();
#endif
}

static void _epic_interrupt_init(void)
{
#if defined(__ZEPHYR__)
    IRQ_CONNECT(LV_SIFLI_EPIC_IRQn, LV_SIFLI_EPIC_IRQ_PRIORITY, _epic_zephyr_irq_handler, NULL,
                LV_SIFLI_EPIC_ZEPHYR_IRQ_FLAGS);
    irq_enable(LV_SIFLI_EPIC_IRQn);
#elif defined(__NuttX__)
    int nuttx_irq = LV_SIFLI_EPIC_NUTTX_IRQ(LV_SIFLI_EPIC_IRQn);
    int ret = irq_attach(nuttx_irq, _epic_nuttx_irq_handler, NULL);
    if(ret == 0) {
        up_enable_irq(nuttx_irq);
    }
#elif LV_USE_OS == LV_OS_FREERTOS && defined(LV_SIFLI_EPIC_FREERTOS_IRQ_PRIORITY)
    HAL_NVIC_SetPriority(LV_SIFLI_EPIC_IRQn, LV_SIFLI_EPIC_FREERTOS_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(LV_SIFLI_EPIC_IRQn);
#else
    HAL_NVIC_SetPriority(LV_SIFLI_EPIC_IRQn, LV_SIFLI_EPIC_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(LV_SIFLI_EPIC_IRQn);
#endif

#ifdef HAL_EZIP_MODULE_ENABLED
#if defined(__ZEPHYR__)
    IRQ_CONNECT(LV_SIFLI_EZIP_IRQn, LV_SIFLI_EPIC_IRQ_PRIORITY, _ezip_zephyr_irq_handler, NULL,
                LV_SIFLI_EPIC_ZEPHYR_IRQ_FLAGS);
    irq_enable(LV_SIFLI_EZIP_IRQn);
#elif defined(__NuttX__)
    int ezip_nuttx_irq = LV_SIFLI_EPIC_NUTTX_IRQ(LV_SIFLI_EZIP_IRQn);
    int ezip_ret = irq_attach(ezip_nuttx_irq, _ezip_nuttx_irq_handler, NULL);
    if(ezip_ret == 0) {
        up_enable_irq(ezip_nuttx_irq);
    }
#elif LV_USE_OS == LV_OS_FREERTOS && defined(LV_SIFLI_EPIC_FREERTOS_IRQ_PRIORITY)
    HAL_NVIC_SetPriority(LV_SIFLI_EZIP_IRQn, LV_SIFLI_EPIC_FREERTOS_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(LV_SIFLI_EZIP_IRQn);
#else
    HAL_NVIC_SetPriority(LV_SIFLI_EZIP_IRQn, LV_SIFLI_EPIC_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(LV_SIFLI_EZIP_IRQn);
#endif
#endif

    epic_idle = true;
}

static void _epic_interrupt_deinit(void)
{
#if defined(__ZEPHYR__)
    irq_disable(LV_SIFLI_EPIC_IRQn);
#elif defined(__NuttX__)
    up_disable_irq(LV_SIFLI_EPIC_NUTTX_IRQ(LV_SIFLI_EPIC_IRQn));
    irq_detach(LV_SIFLI_EPIC_NUTTX_IRQ(LV_SIFLI_EPIC_IRQn));
#else
    HAL_NVIC_DisableIRQ(LV_SIFLI_EPIC_IRQn);
#endif

#ifdef HAL_EZIP_MODULE_ENABLED
#if defined(__ZEPHYR__)
    irq_disable(LV_SIFLI_EZIP_IRQn);
#elif defined(__NuttX__)
    up_disable_irq(LV_SIFLI_EPIC_NUTTX_IRQ(LV_SIFLI_EZIP_IRQn));
    irq_detach(LV_SIFLI_EPIC_NUTTX_IRQ(LV_SIFLI_EZIP_IRQn));
#else
    HAL_NVIC_DisableIRQ(LV_SIFLI_EZIP_IRQn);
#endif
#endif

    epic_idle = true;
}

static void _epic_run(void)
{
#if defined(__NuttX__)
    epic_wait_failure = false;
    if(epic_sem_inited) {
        while(sem_trywait(&epic_sem) == 0) {
        }
    }
#elif LV_USE_OS
    /* Reset completion token from any previous EPIC job so the next wait only
     * observes the current run's completion signal. Using LVGL's thread-sync
     * API ensures cross-platform compatibility without accessing OS internals. */
    (void)lv_thread_sync_delete(&epic_sync);
    (void)lv_thread_sync_init(&epic_sync);
#endif
    epic_idle = false;
}

static void _epic_wait(void)
{
    if(epic_idle) {
        return;
    }

#if LV_USE_OS
    if(lv_epic_osa_thread_sync_wait() == LV_RESULT_OK) {
        epic_idle = true;
    }
    else {
        epic_idle = true;
    }
#else
#if defined(__NuttX__)
    uint32_t waited_us = 0;
    while(!epic_idle && waited_us < LV_SIFLI_EPIC_WAIT_TIMEOUT_US) {
        up_udelay(1);
        waited_us++;
    }

    if(!epic_idle) {
        epic_idle = true;
    }
#else
    while(!epic_idle) {
    }
#endif
#endif

}

#endif /*LV_USE_SIFLI_EPIC*/
