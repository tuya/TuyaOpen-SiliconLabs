/****************************************************************************
 * @file tkl_pinmux.c
 * @brief this module is used to tkl_pinmux
 * @version 0.0.1
 * @date 2023-06-07
 *
 * @copyright Copyright(C) 2021-2022 Tuya Inc. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "tkl_pinmux.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief tuya io pinmux func
 *
 * @param[in] pin: pin number
 * @param[in] pin_func: pin function
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func)
{
    return OPRT_OK;
}

OPERATE_RET tkl_multi_io_pinmux_config(TUYA_MUL_PIN_CFG_T *cfg, uint16_t num)
{
    return OPRT_OK;
}

int32_t tkl_io_pin_to_func(uint32_t pin, TUYA_PIN_TYPE_E pin_type)
{
    int32_t port_channel = OPRT_NOT_SUPPORTED;

    return port_channel;
}
