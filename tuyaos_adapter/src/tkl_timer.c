/*****************************************************************************//**
 * @file tkl_timer.c
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_timer.h"
#include "sl_si91x_ulp_timer.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

#define TIMER_DEV_NUM ULP_TIMER_LAST

#define DECLARE_TIMERx_IRQ_HANDLE(x)                                                                                   \
    void sl_timer##x##_irq_handle()                                                                                    \
    {                                                                                                                  \
        if (cfg_save[x].cb != NULL) {                                                                                  \
            cfg_save[x].cb(cfg_save[x].args);                                                                          \
        }                                                                                                              \
    }

typedef void (*timer_isr)(void);

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

static TUYA_TIMER_BASE_CFG_T cfg_save[TIMER_DEV_NUM];

DECLARE_TIMERx_IRQ_HANDLE(0);
DECLARE_TIMERx_IRQ_HANDLE(1);
DECLARE_TIMERx_IRQ_HANDLE(2);
DECLARE_TIMERx_IRQ_HANDLE(3);

static timer_isr timer_isr_handle[TIMER_DEV_NUM] = {
    sl_timer0_irq_handle,
    sl_timer1_irq_handle,
    sl_timer2_irq_handle,
    sl_timer3_irq_handle,
};

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/**
 * @brief timer init
 *
 * @param[in] timer_id timer id
 * @param[in] cfg timer configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_init(TUYA_TIMER_NUM_E timer_id, TUYA_TIMER_BASE_CFG_T *cfg)
{
    sl_status_t rt;

    if ((int)timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    cfg_save[timer_id].args = cfg->args;
    cfg_save[timer_id].cb   = cfg->cb;
    cfg_save[timer_id].mode = cfg->mode;

    ulp_timer_config_t sl_timer = {
        .timer_num         = timer_id,
        .timer_mode        = (cfg->mode == TUYA_TIMER_MODE_PERIOD) ? ULP_TIMER_MODE_PERIODIC : ULP_TIMER_MODE_ONESHOT,
        .timer_type        = ULP_TIMER_TYP_1US,
        .timer_match_value = 0xFFFFFFFF,
        .timer_direction   = UP_COUNTER,
    };

    rt = sl_si91x_ulp_timer_set_configuration(&sl_timer);
    if (rt != SL_STATUS_OK) {
        return OPRT_COM_ERROR;
    }

    rt = sl_si91x_ulp_timer_register_timeout_callback(sl_timer.timer_num, timer_isr_handle[timer_id]);
    if (rt != SL_STATUS_OK) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief timer start
 *
 * @param[in] timer_id timer id
 * @param[in] us when to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_start(TUYA_TIMER_NUM_E timer_id, uint32_t us)
{
    sl_status_t rt;

    if ((int)timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    rt = sl_si91x_ulp_timer_set_count((ulp_timer_instance_t)timer_id, us);
    if (rt != SL_STATUS_OK) {
        return OPRT_COM_ERROR;
    }

    rt = sl_si91x_ulp_timer_restart((ulp_timer_instance_t)timer_id);
    if (rt != SL_STATUS_OK) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief timer stop
 *
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_stop(TUYA_TIMER_NUM_E timer_id)
{
    if ((int)timer_id >= TIMER_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }

    sl_si91x_ulp_timer_stop((ulp_timer_instance_t)timer_id);

    return OPRT_OK;
}

/**
 * @brief timer deinit
 *
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_deinit(TUYA_TIMER_NUM_E timer_id)
{
    if ((int)timer_id >= TIMER_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }

    sl_si91x_ulp_timer_unregister_timeout_callback((ulp_timer_instance_t)timer_id);

    return OPRT_OK;
}

/**
 * @brief timer get
 *
 * @param[in] timer_id timer id
 * @param[out] ms timer interval
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get(TUYA_TIMER_NUM_E timer_id, uint32_t *us)
{
    if ((int)timer_id >= TIMER_DEV_NUM) {
        return OPRT_INVALID_PARM;
    }

    sl_si91x_ulp_timer_get_count((ulp_timer_instance_t)timer_id, us);

    return OPRT_OK;
}

/**
 * @brief current timer get
 *
 * @param[in] timer_id timer id
 * @param[out] us timer
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get_current_value(TUYA_TIMER_NUM_E timer_id, uint32_t *us)
{
    return OPRT_NOT_SUPPORTED;
}
