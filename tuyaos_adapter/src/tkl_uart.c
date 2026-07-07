/*****************************************************************************//**
 * @file tkl_uart.c
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
#include "cmsis_os2.h"

#include "sl_si91x_usart.h"
#include "sl_status.h"

#include "tuya_error_code.h"
#include "tkl_uart.h"
#include "tkl_system.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

#define MAX_UART_NUM             3
#define UART_TX_RX_CHARC_TIMEOUT 10
#define UART_RX_BUFFER_SIZE      384

typedef void (*sl_uart_cb_t)(uint32_t event);

typedef struct {
    usart_peripheral_t periph; /**< */
    sl_usart_handle_t  handle; /**< */
    sl_uart_cb_t       uart_ev_cb;
    TUYA_UART_IRQ_CB   uart_rx_cb;
    osMessageQueueId_t rx_buffs;
    uint8_t            initialize;
    uint8_t            rx_tmp;
    volatile uint8_t   tx_inprogress;
    volatile uint8_t   rx_inprogress;
} tkl_uart_t;

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

static void _tuya_uart_event_cb(TUYA_UART_NUM_E port_id, uint32_t ev);
static void _tuya_uart0_event_cb(uint32_t ev);
static void _tuya_uart1_event_cb(uint32_t ev);
static void _tuya_uart2_event_cb(uint32_t ev);

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

tkl_uart_t tkl_uart_context[MAX_UART_NUM] = {
    // TUYA_UART_NUM_0
    {
        .periph     = ULPUART,
        .uart_ev_cb = _tuya_uart0_event_cb,
    },
    // TUYA_UART_NUM_1
    {
        .periph     = USART_0,
        .uart_ev_cb = _tuya_uart1_event_cb,
    },
    // TUYA_UART_NUM_2
    {
        .periph     = UART_1,
        .uart_ev_cb = _tuya_uart2_event_cb,
    },
};

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

void _tuya_uart0_event_cb(uint32_t ev)
{
    _tuya_uart_event_cb(TUYA_UART_NUM_0, ev);
}

void _tuya_uart1_event_cb(uint32_t ev)
{
    _tuya_uart_event_cb(TUYA_UART_NUM_1, ev);
}

void _tuya_uart2_event_cb(uint32_t ev)
{
    _tuya_uart_event_cb(TUYA_UART_NUM_2, ev);
}

void _tuya_uart_event_cb(TUYA_UART_NUM_E port_id, uint32_t ev)
{
    tkl_uart_t *uart = &tkl_uart_context[port_id];

    if (port_id >= MAX_UART_NUM) {
        return;
    }

    if (ev == SL_USART_EVENT_RECEIVE_COMPLETE) {
        osMessageQueuePut(uart->rx_buffs, &uart->rx_tmp, 0U, 0);
        sl_si91x_usart_receive_data(uart->handle, &uart->rx_tmp, 1);
        if (uart->uart_rx_cb != NULL) {
            uart->uart_rx_cb(port_id);
        }
    } else if (ev == SL_USART_EVENT_SEND_COMPLETE) {
        uart->tx_inprogress = 0;
    }
}

/**
 * @brief uart init
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] cfg: uart config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    sl_status_t                     status     = SL_STATUS_OK;
    sl_si91x_usart_control_config_t uart_cfg   = {0};
    sl_si91x_usart_control_config_t get_config = {0};
    tkl_uart_t                     *uart       = NULL;

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (port_id >= MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }
    uart = &tkl_uart_context[port_id];

    if (TUYA_UART_DATA_LEN_5BIT == cfg->databits) {
        uart_cfg.databits = SL_USART_DATA_BITS_5;
    } else if (TUYA_UART_DATA_LEN_6BIT == cfg->databits) {
        uart_cfg.databits = SL_USART_DATA_BITS_6;
    } else if (TUYA_UART_DATA_LEN_7BIT == cfg->databits) {
        uart_cfg.databits = SL_USART_DATA_BITS_7;
    } else if (TUYA_UART_DATA_LEN_8BIT == cfg->databits) {
        uart_cfg.databits = SL_USART_DATA_BITS_8;
    } else {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_STOP_LEN_1BIT == cfg->stopbits) {
        uart_cfg.stopbits = SL_USART_STOP_BITS_1;
    } else if (TUYA_UART_STOP_LEN_2BIT == cfg->stopbits) {
        uart_cfg.stopbits = SL_USART_STOP_BITS_2;
    } else if (TUYA_UART_STOP_LEN_1_5BIT1 == cfg->stopbits) {
        uart_cfg.stopbits = SL_USART_STOP_BITS_1_5;
    } else {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_PARITY_TYPE_NONE == cfg->parity) {
        uart_cfg.parity = SL_USART_NO_PARITY;
    } else if (TUYA_UART_PARITY_TYPE_EVEN == cfg->parity) {
        uart_cfg.parity = SL_USART_EVEN_PARITY;
    } else if (TUYA_UART_PARITY_TYPE_ODD == cfg->parity) {
        uart_cfg.parity = SL_USART_ODD_PARITY;
    } else {
        return OPRT_INVALID_PARM;
    }

    uart_cfg.baudrate      = cfg->baudrate;
    uart_cfg.hwflowcontrol = SL_USART_FLOW_CONTROL_NONE;
    uart_cfg.mode          = SL_USART_MODE_ASYNCHRONOUS;
    uart_cfg.misc_control  = SL_USART_MISC_CONTROL_NONE;
    uart_cfg.usart_module  = uart->periph;
    uart_cfg.config_enable = ENABLE;
    uart_cfg.synch_mode    = DISABLE;

    if (uart->rx_buffs == NULL) {
        uart->rx_buffs = osMessageQueueNew(UART_RX_BUFFER_SIZE, sizeof(uint8_t), NULL);
    }
    // Initialize the UART
    do {
        status = sl_si91x_usart_init(uart->periph, &uart->handle);
        if (status != SL_STATUS_OK) {
            break;
        }

        // Register user callback function
        sl_si91x_usart_multiple_instance_register_event_callback(uart->periph, uart->uart_ev_cb);

        // Configure the USART configurations
        status = sl_si91x_usart_set_configuration(uart->handle, &uart_cfg);
        if (status != SL_STATUS_OK) {
            break;
        }

        // Start receiver data
        status = sl_si91x_usart_receive_data(uart->handle, &uart->rx_tmp, 1);
        if (status != SL_STATUS_OK) {
            break;
        }

        sl_si91x_usart_get_configurations((uint8_t)uart->periph, &get_config);
    } while (0);

    uart->initialize = 1;

    return status == SL_STATUS_OK ? OPRT_OK : OPRT_COM_ERROR;
}

/**
 * @brief uart deinit
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_id)
{
    sl_status_t status;
    tkl_uart_t *uart = NULL;

    if (port_id >= MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    uart   = &tkl_uart_context[port_id];
    status = sl_si91x_usart_deinit(uart->handle);

    return status == SL_STATUS_OK ? OPRT_OK : OPRT_COM_ERROR;
}

/**
 * @brief uart write data
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] data: write buff
 * @param[in] len:  buff len
 *
 * @return return > 0: number of data written; return <= 0: write errror
 */
int tkl_uart_write(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    SYS_TIME_T  timeout;
    sl_status_t status;
    tkl_uart_t *uart = NULL;

    if (port_id >= MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    uart = &tkl_uart_context[port_id];

    if (!uart->initialize) {
        return OPRT_COM_ERROR;
    }

    if (uart->tx_inprogress) {
        return OPRT_COM_ERROR;
    }

    timeout             = tkl_system_get_millisecond() + UART_TX_RX_CHARC_TIMEOUT * len;
    uart->tx_inprogress = 1;
    status              = sl_si91x_usart_send_data(uart->handle, buff, len);
    if (status == SL_STATUS_OK) {
        while (uart->tx_inprogress && tkl_system_get_millisecond() < timeout) {
            /* Wait for UART send done */
        }

        if (uart->tx_inprogress) {
            /* Timeout */
            uart->tx_inprogress = 0;
            return -1;
        }
    }

    return len;
}

/**
 * @brief enable uart rx interrupt and regist interrupt callback
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    tkl_uart_t *uart = NULL;

    if (port_id >= MAX_UART_NUM) {
        return;
    }
    uart             = &tkl_uart_context[port_id];
    uart->uart_rx_cb = rx_cb;
}

/**
 * @brief regist uart tx interrupt callback
 * If this function is called, it indicates that the data is sent asynchronously through interrupt,
 * and then write is invoked to initiate asynchronous transmission.
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{
    TKL_UNUSED(port_id);
    TKL_UNUSED(tx_cb);
}

/**
 * @brief uart read data
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[out] data: read data
 * @param[in] len:  buff len
 *
 * @return return >= 0: number of data read; return < 0: read errror
 */
int tkl_uart_read(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    tkl_uart_t *uart   = NULL;
    uint8_t    *abuf   = (uint8_t *)buff;
    int         nbytes = 0;
    int         status;

    if (port_id >= MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    if (NULL == buff || 0 == len) {
        return OPRT_INVALID_PARM;
    }

    uart = &tkl_uart_context[port_id];

    while (osMessageQueueGetCount(uart->rx_buffs) != 0 && nbytes < len) {
        status = osMessageQueueGet(uart->rx_buffs, &abuf[nbytes], 0, 0);
        if (status == osOK) {
            nbytes++;
        } else {
            break;
        }
    }

    return nbytes;
}

/**
 * @brief set uart transmit interrupt status
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe tx int, FALSE-disable tx int
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    TKL_UNUSED(port_id);
    TKL_UNUSED(enable);
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief set uart receive flowcontrol
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe rx flowcontrol, FALSE-disable rx flowcontrol
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_rx_flowctrl(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    TKL_UNUSED(port_id);
    TKL_UNUSED(enable);
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief wait for uart data
 *
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform,
 *                         high 16 bits aslo means uart type,
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] timeout_ms: the max wait time, unit is millisecond
 *                        -1 : block indefinitely
 *                        0  : non-block
 *                        >0 : timeout in milliseconds
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_wait_for_data(TUYA_UART_NUM_E port_id, int timeout_ms)
{
    TKL_UNUSED(port_id);
    TKL_UNUSED(timeout_ms);
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief uart control
 *
 * @param[in] uart refer to tuya_uart_t
 * @param[in] cmd control command
 * @param[in] arg command argument
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_ioctl(TUYA_UART_NUM_E port_id, uint32_t cmd, void *arg)
{
    TKL_UNUSED(port_id);
    TKL_UNUSED(cmd);
    TKL_UNUSED(arg);
    return OPRT_NOT_SUPPORTED;
}
