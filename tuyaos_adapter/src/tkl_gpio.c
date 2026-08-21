/*****************************************************************************//**
 * @file tkl_gpio.c
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
#include <stdbool.h>
#include "tkl_gpio.h"
#include "tuya_error_code.h"
/* em_device.h, not sl_gpio_board.h: the port constants this file switches on
 * (HP / ULP / UULP_VBAT) come from RTE_Device_917.h, which em_device.h pulls
 * in, and they are the same on every board -- verified against two. What used
 * to be included here was the *board's* sl_gpio_board.h, which declares only
 * the pads that board brings out, and that is what made the pin table below
 * fail to compile on a board that leaves one out. */
#include "em_device.h"
#include "sl_si91x_driver_gpio.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

#define ALLOCATED         true
#define AVAILABLE         false
#define NO_INTR_AVAILABLE 8
#define AVL_INT           0

// Max IRQ number in case of HP ULP and UULP ports
#define MAX_HP_IRQ_NUMBER   8
#define MAX_ULP_IRQ_NUMBER  8
#define MAX_UULP_IRQ_NUMBER 5

// Macro to create the flags for enable and disable interupt api
#define GPIO_INT_FLAGS(channel, trigmask) ((((uint32_t)(channel)&0xFFu) << 16) | ((uint32_t)(trigmask)&0x0Fu))

#define SL_GPIO_SET(gpio, info)                                                                                        \
    do {                                                                                                               \
        gpio.port = info->port;                                                                                        \
        gpio.pin  = info->pin;                                                                                         \
    } while (0)

/********************************************************************
 * Pin mapping table
 *
 * TUYA GPIO number -> (SiWx917 port, pad index), and nothing about any
 * particular board.
 *
 *   TUYA GPIO  [0:4]                        -> UULP GPIO [0:4]
 *   TUYA GPIO  [6:12], 15, [25:34], [46:57] -> HP   GPIO, same number
 *   TUYA GPIO  [20:24], [35:41]             -> ULP  GPIO [0:4], [5:11]
 *
 * The values are the SoC's. An HP pad is (HP, n), a UULP pad is
 * (UULP_VBAT, n), and a ULP pad is (ULP, n) -- checked against
 * RTE_Device_917.h for two different boards, where each of these is defined
 * identically, because none of it is board-specific.
 *
 * This used to read SL_SI91X_<pad>_PIN/_PORT out of sl_gpio_board.h, which is
 * the *board's* file: a board declares only the pads it brings out. So the
 * table stopped compiling on any board that leaves one out -- BRD2605A omits
 * ULP_GPIO_3 and UULP_GPIO_4, and its own RTE_Device_917.h does not define
 * them either, so that is the board stating a fact rather than an oversight.
 * SIWX917_AI_DEV_KIT built only because it ships no sl_gpio_board.h of its own
 * and so inherited the chip-wide default that declares every pad.
 *
 * Which pads a board brings out belongs to the board layer, and is already
 * there: boards/SiWx917/<BOARD>/Kconfig names the pins that board uses. This
 * is how T5AI does it too -- one flat, unconditional pinmap in the chip layer
 * and no board conditionals in it at all.
 *
 * HP and UULP rows take one argument because for them the TUYA number and the
 * pad index are the same; writing it once keeps the two from drifting apart.
 ********************************************************************/

/* ULP pads are reached on the ULP port by their own index on radio-board base
 * versions, and on the HP port at 64 + index otherwise. That is the SDK's own
 * distinction -- RTE_ULP_GPIO_n_PORT_ID, keyed on
 * SLI_SI91X_MCU_CONFIG_RADIO_BOARD_BASE_VER -- and a property of the silicon
 * revision rather than of a board, so it belongs here: one condition for the
 * whole class, not one per pad. */
#ifdef SLI_SI91X_MCU_CONFIG_RADIO_BOARD_BASE_VER
#define ULP_PAD(n) .pin = (n), .port = ULP
#else
#define ULP_PAD(n) .pin = 64 + (n), .port = HP
#endif

#define X_UULP(n)      {.pin_id = TUYA_GPIO_NUM_##n, .pin = (n), .port = UULP_VBAT},
#define X_HP(n)        {.pin_id = TUYA_GPIO_NUM_##n, .pin = (n), .port = HP},
#define X_ULP(tuya, n) {.pin_id = TUYA_GPIO_NUM_##tuya, ULP_PAD(n)},

#define SI91X_PIN_MAPPING                                                                                              \
    X_UULP(0)                                                                                                          \
    X_UULP(1)                                                                                                          \
    X_UULP(2)                                                                                                          \
    X_UULP(3)                                                                                                          \
    X_UULP(4)                                                                                                          \
    X_HP(6)                                                                                                            \
    X_HP(7)                                                                                                            \
    X_HP(8)                                                                                                            \
    X_HP(9)                                                                                                            \
    X_HP(10)                                                                                                           \
    X_HP(11)                                                                                                           \
    X_HP(12)                                                                                                           \
    X_HP(15)                                                                                                           \
    X_HP(25)                                                                                                           \
    X_HP(26)                                                                                                           \
    X_HP(27)                                                                                                           \
    X_HP(28)                                                                                                           \
    X_HP(29)                                                                                                           \
    X_HP(30)                                                                                                           \
    X_HP(31)                                                                                                           \
    X_HP(32)                                                                                                           \
    X_HP(33)                                                                                                           \
    X_HP(34)                                                                                                           \
    X_HP(46)                                                                                                           \
    X_HP(47)                                                                                                           \
    X_HP(48)                                                                                                           \
    X_HP(49)                                                                                                           \
    X_HP(50)                                                                                                           \
    X_HP(51)                                                                                                           \
    X_HP(52)                                                                                                           \
    X_HP(53)                                                                                                           \
    X_HP(54)                                                                                                           \
    X_HP(55)                                                                                                           \
    X_HP(56)                                                                                                           \
    X_HP(57)                                                                                                           \
    X_ULP(20, 0)                                                                                                       \
    X_ULP(21, 1)                                                                                                       \
    X_ULP(22, 2)                                                                                                       \
    X_ULP(23, 3)                                                                                                       \
    X_ULP(24, 4)                                                                                                       \
    X_ULP(35, 5)                                                                                                       \
    X_ULP(36, 6)                                                                                                       \
    X_ULP(37, 7)                                                                                                       \
    X_ULP(38, 8)                                                                                                       \
    X_ULP(39, 9)                                                                                                       \
    X_ULP(40, 10)                                                                                                      \
    X_ULP(41, 11)

typedef struct {
    TUYA_GPIO_NUM_E pin_id;
    uint8_t         pin;
    uint8_t         port;
} pin_map_t;

static const pin_map_t gpio_list[] = {SI91X_PIN_MAPPING};

#undef X_ULP
#undef X_HP
#undef X_UULP
#undef ULP_PAD

typedef struct {
    TUYA_GPIO_NUM_E        pin_id;
    const TUYA_GPIO_IRQ_T *cfg;
    uint8_t                trigger;
    bool                   status;
} hp_irq_entry_t;

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

static hp_irq_entry_t         hp_irq_table[MAX_HP_IRQ_NUMBER]     = {0};
static const TUYA_GPIO_IRQ_T *ulp_irq_table[MAX_ULP_IRQ_NUMBER]   = {0};
static const TUYA_GPIO_IRQ_T *uulp_irq_table[MAX_UULP_IRQ_NUMBER] = {0};

static bool    s_inited = false;
static uint8_t hp_gpio_irq_active_count;
static uint8_t ulp_gpio_irq_active_count;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

static void hp_callback_wrapper(uint32_t intr_num)
{
    if (hp_irq_table[intr_num].cfg->cb != NULL) {
        hp_irq_table[intr_num].cfg->cb(hp_irq_table[intr_num].cfg->arg);
    }
}
static void ulp_callback_wrapper(uint32_t intr_num)
{
    if (ulp_irq_table[intr_num]->cb != NULL) {
        ulp_irq_table[intr_num]->cb(ulp_irq_table[intr_num]->arg);
    }
}
static void uulp_callback_wrapper(uint32_t intr_num)
{
    if (uulp_irq_table[intr_num]->cb != NULL) {
        uulp_irq_table[intr_num]->cb(uulp_irq_table[intr_num]->arg);
    }
}

static const pin_map_t *_tkl_get_pin_info(uint32_t pin_id)
{
    for (unsigned int i = 0; i < sizeof(gpio_list) / sizeof(pin_map_t); i++) {
        if (pin_id == gpio_list[i].pin_id) {
            return &gpio_list[i];
        }
    }

    return NULL;
}

static bool get_hp_intr_info(TUYA_GPIO_NUM_E pin_id, uint8_t *intr_num, uint8_t *trigger)
{
    for (uint8_t i = 0; i < MAX_HP_IRQ_NUMBER; i++) {
        if (hp_irq_table[i].pin_id == pin_id && hp_irq_table[i].status == ALLOCATED) {
            *intr_num = i;
            *trigger  = hp_irq_table[i].trigger;
            return true;
        }
    }
    return false;
}

/**
 * @brief Initialize a GPIO pin with specified configuration
 *
 * This function initializes a GPIO pin on the SiWx917 platform according to the
 * provided configuration. It handles domain-specific initialization for HP, ULP,
 * and UULP power domains, configuring direction, pull-up/down resistors, and
 * initial output levels.
 *
 * NOTE : TUYA_GPIO_NUM_8 and TUYA_GPIO_NUM_16 are configured for vcom and hence
 *        shall not be used for this api.
 * @param pin_id GPIO pin identifier (0-45)
 * @param cfg    Pointer to GPIO configuration structure
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_BASE_CFG_T *cfg)
{
    sl_status_t                          status;
    sl_si91x_gpio_pin_config_t           pin_config;
    sl_si91x_gpio_direction_t            direction;
    sl_si91x_gpio_driver_disable_state_t gpio_mode;
    const pin_map_t                     *pin_info;
    bool                                 mode_valid = true;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    /* Initialize GPIO driver on first use */
    if (!s_inited) {
        status = sl_gpio_driver_init();
        if (status != SL_STATUS_OK) {
            return OPRT_RESOURCE_NOT_READY;
        }
        s_inited = true;
    }

    pin_info = _tkl_get_pin_info(pin_id);
    if (pin_info == NULL) {
        return OPRT_INVALID_PARM;
    }

    /* Configure GPIO direction */
    switch (cfg->direct) {
    case TUYA_GPIO_INPUT:
        direction = GPIO_INPUT;
        break;
    case TUYA_GPIO_OUTPUT:
        direction = GPIO_OUTPUT;
        break;
    default:
        return OPRT_INVALID_PARM;
    }

    /* Set up GPIO pin configuration */
    SL_GPIO_SET(pin_config.port_pin, pin_info);
    pin_config.direction = direction;

    status = sl_gpio_set_configuration(pin_config);
    if (status != SL_STATUS_OK) {
        return OPRT_RESOURCE_NOT_READY;
    }

    /* Configure pull-up/pull-down mode */
    switch (cfg->mode) {
    case TUYA_GPIO_PULLUP:
        gpio_mode = GPIO_PULLUP;
        break;

    case TUYA_GPIO_PULLDOWN:
        gpio_mode = GPIO_PULLDOWN;
        break;

    case TUYA_GPIO_HIGH_IMPEDANCE:
        gpio_mode = GPIO_HZ;
        break;

    default:
        mode_valid = false;
        break;
    }

    if (mode_valid) {
        /* Apply domain-specific pad configuration */
        switch (pin_info->port) {
        case HP:
            status = sl_si91x_gpio_driver_select_pad_driver_disable_state(pin_info->pin, gpio_mode);
            if (status != SL_STATUS_OK) {
                return OPRT_RESOURCE_NOT_READY;
            }
            break;

        case ULP:
            status = sl_si91x_gpio_driver_select_ulp_pad_driver_disable_state(pin_info->pin, gpio_mode);
            if (status != SL_STATUS_OK) {
                return OPRT_RESOURCE_NOT_READY;
            }
            break;

        case UULP_VBAT:
            /* UULP domain only supports high impedance mode */
            if (gpio_mode != GPIO_HZ) {
                // return OPRT_NOT_SUPPORTED;
            }
            break;

        default:
            break;
        }
    }

    /* Set initial output level for output pins */
    if (cfg->direct == TUYA_GPIO_OUTPUT) {
        tkl_gpio_write(pin_id, cfg->level);
    }

    return OPRT_OK;
}

/**
 * @brief Deinitialize a GPIO pin
 *
 * This function deinitializes a GPIO pin on the SiWx917 platform by reversing
 * the configuration applied during initialization. It disables interrupts,
 * clears pin configuration, and sets the pin to high impedance state.
 *
 * @param pin_id GPIO pin identifier (0-45)
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_deinit(TUYA_GPIO_NUM_E pin_id)
{
    sl_status_t      status;
    sl_gpio_t        gpio;
    const pin_map_t *pin_info;

    /* If driver never initialized, make deinit idempotent. */
    if (!s_inited) {
        return OPRT_OK;
    }

    pin_info = _tkl_get_pin_info(pin_id);
    if (pin_info == NULL) {
        return OPRT_INVALID_PARM;
    }

    SL_GPIO_SET(gpio, pin_info);
    /* Avoid surprise HIGH on future re-enable */
    sl_gpio_driver_clear_pin(&gpio);

    switch (pin_info->port) {
    case HP: {
        status = sl_si91x_gpio_driver_select_pad_driver_disable_state(gpio.pin, GPIO_HZ);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;

        status = sl_si91x_gpio_driver_disable_pad_receiver(gpio.pin);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;

        /* Mode = DISABLED, dout=0 => floating, no weak pull */
        status = sl_gpio_driver_set_pin_mode(&gpio, SL_GPIO_MODE_DISABLED, 0);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;
        break;
    }

    case ULP: {
        status = sl_si91x_gpio_driver_select_ulp_pad_driver_disable_state(gpio.pin, GPIO_HZ);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;

        status = sl_si91x_gpio_driver_disable_ulp_pad_receiver(gpio.pin);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;

        /* Mode = DISABLED (no input buffer, no output driver) */
        status = sl_gpio_driver_set_pin_mode(&gpio, SL_GPIO_MODE_DISABLED, 0);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;
        break;
    }

    case UULP_VBAT: {
        /* UULP VBAT: mode 0 = OUTPUT. Select a non-output mode, disable output & receiver. */
        uulp_pad_config_t uulp_pad = {
            .gpio_padnum = gpio.pin, .pad_select = SET, .mode = CLR, .direction = CLR, .receiver = CLR};
        status = sl_si91x_gpio_driver_set_uulp_pad_configuration(&uulp_pad);
        if (status != SL_STATUS_OK)
            return OPRT_RESOURCE_NOT_READY;
        break;
    }

    default:
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

/**
 * @brief Write a value to a GPIO output pin
 *
 * Sets the output level of a GPIO pin that has been configured as an output.
 * The pin must have been previously initialized with tkl_gpio_init().
 *
 * @param pin_id GPIO pin identifier (0-45)
 * @param level  Output level to set (HIGH or LOW)
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_write(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E level)
{
    sl_status_t      status;
    sl_gpio_t        gpio;
    const pin_map_t *pin_info;

    pin_info = _tkl_get_pin_info(pin_id);
    if (pin_info == NULL) {
        return OPRT_INVALID_PARM;
    }

    SL_GPIO_SET(gpio, pin_info);

    switch (level) {
    case TUYA_GPIO_LEVEL_LOW:
        status = sl_gpio_driver_clear_pin(&gpio);
        if (status != SL_STATUS_OK) {
            return OPRT_RESOURCE_NOT_READY;
        }
        break;

    case TUYA_GPIO_LEVEL_HIGH:
        status = sl_gpio_driver_set_pin(&gpio);
        if (status != SL_STATUS_OK) {
            return OPRT_RESOURCE_NOT_READY;
        }
        break;

    default:
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

/**
 * @brief Read the current value of a GPIO pin
 *
 * Reads the current logic level of a GPIO pin. Works for input pins.
 *
 * @param pin_id GPIO pin identifier (0-45)
 * @param level  Pointer to store the read level (HIGH or LOW)
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_read(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E *level)
{
    sl_status_t      status;
    sl_gpio_t        gpio;
    const pin_map_t *pin_info;
    uint8_t          pin_value = 0;

    if (level == NULL) {
        return OPRT_INVALID_PARM;
    }

    pin_info = _tkl_get_pin_info(pin_id);
    if (pin_info == NULL) {
        return OPRT_INVALID_PARM;
    }

    SL_GPIO_SET(gpio, pin_info);

    status = sl_gpio_driver_get_pin(&gpio, &pin_value);
    if (status != SL_STATUS_OK) {
        return OPRT_COM_ERROR;
    }

    *level = (TUYA_GPIO_LEVEL_E)pin_value;

    return OPRT_OK;
}

/**
 * @brief Initialize GPIO interrupt configuration
 *
 * Configures a GPIO pin for interrupt operation with the specified trigger mode
 * and callback function. This function allocates interrupt resources from the
 * appropriate power domain and sets up the interrupt routing.
 *
 * NOTE: This function initializes the interrupt and enables it by default.
 *       Call tkl_gpio_irq_disable() to disable the interrupt(Only for HP domanin).
 *       Call tkl_gpio_irq_enable() to enable a disabled interrupt(Only for HP domain).
 *
 * @param pin_id GPIO pin identifier (0-45)
 * @param cfg    Pointer to GPIO interrupt configuration structure
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_irq_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_IRQ_T *cfg)
{
    sl_status_t              status = SL_STATUS_OK;
    sl_gpio_t                gpio;
    const pin_map_t         *pin_info;
    sl_gpio_interrupt_flag_t flag;
    uint8_t                  intr_num;

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    pin_info = _tkl_get_pin_info(pin_id);
    if (pin_info == NULL) {
        return OPRT_INVALID_PARM;
    }

    SL_GPIO_SET(gpio, pin_info);

    switch (cfg->mode) {
    case TUYA_GPIO_IRQ_RISE:
        flag = SL_GPIO_INTERRUPT_RISING_EDGE;
        break;
    case TUYA_GPIO_IRQ_FALL:
        flag = SL_GPIO_INTERRUPT_FALLING_EDGE;
        break;
    case TUYA_GPIO_IRQ_RISE_FALL:
        flag = SL_GPIO_INTERRUPT_RISE_FALL_EDGE;
        break;
    case TUYA_GPIO_IRQ_LOW:
        flag = SL_GPIO_INTERRUPT_LOW;
        break;
    case TUYA_GPIO_IRQ_HIGH:
        flag = SL_GPIO_INTERRUPT_HIGH;
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }

    switch (pin_info->port) {
    case HP:
        if (hp_gpio_irq_active_count >= MAX_HP_IRQ_NUMBER) {
            return OPRT_NOT_SUPPORTED;
        }

        intr_num                       = hp_gpio_irq_active_count++;
        hp_irq_table[intr_num].pin_id  = pin_id;
        hp_irq_table[intr_num].cfg     = cfg;
        hp_irq_table[intr_num].trigger = flag;
        hp_irq_table[intr_num].status  = ALLOCATED;
        status = sl_gpio_driver_configure_interrupt(&gpio, intr_num, flag, hp_callback_wrapper, AVL_INT);
        break;

    case ULP:
        if (hp_gpio_irq_active_count >= MAX_HP_IRQ_NUMBER) {
            return OPRT_NOT_SUPPORTED;
        }
        intr_num                = ulp_gpio_irq_active_count++;
        ulp_irq_table[intr_num] = cfg;
        status = sl_gpio_driver_configure_interrupt(&gpio, intr_num, flag, ulp_callback_wrapper, AVL_INT);
        break;

    case UULP_VBAT:
        intr_num                 = pin_info->pin;
        uulp_irq_table[intr_num] = cfg;
        status = sl_gpio_driver_configure_interrupt(&gpio, intr_num, flag, uulp_callback_wrapper, AVL_INT);
        break;
    }

    if (status != SL_STATUS_OK) {
        switch (pin_info->port) {
        case HP:
            hp_irq_table[--hp_gpio_irq_active_count].status = AVAILABLE;
            break;
        case ULP:
            ulp_gpio_irq_active_count--;
            break;
        }

        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

/**
 * @brief Enable GPIO interrupt
 *
 * Enables interrupts for a previously configured GPIO pin. The pin must have
 * been initialized for interrupt operation using tkl_gpio_irq_init().
 *
 * NOTE: Currently only supports HP (High Performance) domain interrupts.
 *       ULP and UULP domain interrupt enabling will return OPRT_NOT_SUPPORTED.
 *
 * @param pin_id GPIO pin identifier (0-45)
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_irq_enable(TUYA_GPIO_NUM_E pin_id)
{
    sl_status_t status   = SL_STATUS_OK;
    uint8_t     intr_num = 0;
    uint8_t     trigger  = 0;
    uint32_t    flags;

    bool is_configured = get_hp_intr_info(pin_id, &intr_num, &trigger);
    if (!is_configured) {
        return OPRT_INVALID_PARM;
    }

    flags  = GPIO_INT_FLAGS(intr_num, trigger);
    status = sl_gpio_driver_enable_interrupts(flags);
    if (status != SL_STATUS_OK) {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

/**
 * @brief Disable GPIO interrupt
 *
 * Disables interrupts for a GPIO pin that was previously configured and enabled
 * for interrupt operation. The interrupt configuration remains intact and can
 * be re-enabled using tkl_gpio_irq_enable().
 *
 * NOTE: Currently only supports HP (High Performance) domain interrupts.
 *       ULP and UULP domain interrupt disabling will return OPRT_NOT_SUPPORTED.
 *
 * @param pin_id GPIO pin identifier (0-45)
 * @return OPRT_OK on success, error code on failure
 */
OPERATE_RET tkl_gpio_irq_disable(TUYA_GPIO_NUM_E pin_id)
{
    sl_status_t status   = SL_STATUS_OK;
    uint8_t     intr_num = 0;
    uint8_t     trigger  = 0;
    uint32_t    flags;

    bool is_configured = get_hp_intr_info(pin_id, &intr_num, &trigger);
    if (!is_configured) {
        return OPRT_INVALID_PARM;
    }

    flags  = GPIO_INT_FLAGS(intr_num, trigger);
    status = sl_gpio_driver_disable_interrupts(flags);
    if (status != SL_STATUS_OK) {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_NOT_SUPPORTED;
}
