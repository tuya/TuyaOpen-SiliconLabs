set(SDK_PATH "/home/xwx/xinkeshipei/TuyaOpen/platform/SiWx917/sdks/simplicity_sdk")
set(COPIED_SDK_PATH "simplicity_sdk_2025.6.1")
set(PKG_PATH "/home/xwx/.silabs/slt/installs")

add_library(slc_tuyaopen_switch_demo OBJECT
    "${SDK_PATH}/../wiseconnect/components/board/silabs/src/rsi_board.c"
    "${SDK_PATH}/../wiseconnect/components/common/src/malloc_thread_safety.c"
    "${SDK_PATH}/../wiseconnect/components/common/src/sl_utility.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/ipmu_apis.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/src/iPMU_prog/iPMU_dotc/rsi_system_config_917.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/src/rsi_deepsleep_soc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/src/rsi_ps_ram_func.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/src/startup_si91x.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/src/system_si91x.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/common/src/sl_si91x_stack_object_declare.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/config/src/rsi_nvic_priorities_config.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/cmsis_driver/UDMA.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/cmsis_driver/USART.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/aux_reference_volt_config.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/clock_update.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_adc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_crc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_dac.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_egpio.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_opamp.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_qspi.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_sysrtc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_timers.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_udma.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_udma_wrapper.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/src/rsi_usart.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/service/clock_manager/src/sl_si91x_clock_manager.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/service/sleeptimer/src/sl_sleeptimer_hal_si91x_sysrtc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_bod.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ipmu.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_pll.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_power_save.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_rtc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_temp_sensor.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_time_period.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_ulpss_clk.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/src/rsi_wwdt.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/rsi_d_cache.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_adc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_bjt_temperature_sensor.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_dma.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_driver_gpio.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_i2c.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_psram.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_psram_handle.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_ulp_timer.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/src/sl_si91x_usart.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/src/sl_si91x_peripheral_gpio.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/src/sl_si91x_peripheral_i2c.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/hal/src/sl_si91x_hal_soc_soft_reset.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_ram.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/rsi_hal_mcu_m4_rom.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/sl_platform.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/sl_platform_wireless.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/sl_si91x_bus.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/sl_si91x_timer.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/src/sli_siwx917_soc.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/rsi_ble_gap_apis.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/rsi_ble_gatt_apis.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/rsi_bt_ble.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/rsi_bt_common_apis.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/rsi_common_apis.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/rsi_utils.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/src/sl_si91x_ble.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/crypto_utility/src/sl_si91x_crypto_utility.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/gcm/src/sl_si91x_gcm.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/wrap/src/sl_si91x_wrap.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/firmware_upgrade/firmware_upgradation.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/host_mcu/si91x/siwx917_soc_ncp_host.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/memory/malloc_buffers.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/sl_net/src/sl_net_rsi_utility.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/sl_net/src/sl_net_si91x_callback_framework.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/sl_net/src/sl_net_si91x_integration_handler.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/sl_net/src/sli_net_si91x_utility.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/src/sl_rsi_utility.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/src/sl_si91x_driver.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/src/sli_wifi_memory_manager.c"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/src/sli_wifi_power_profile.c"
    "${SDK_PATH}/../wiseconnect/components/protocol/wifi/si91x/sl_wifi.c"
    "${SDK_PATH}/../wiseconnect/components/protocol/wifi/src/sl_wifi_basic_credentials.c"
    "${SDK_PATH}/../wiseconnect/components/protocol/wifi/src/sl_wifi_callback_framework.c"
    "${SDK_PATH}/../wiseconnect/components/protocol/wifi/src/sli_wifi_callback_framework.c"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/src/sl_net.c"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/src/sl_net_basic_certificate_store.c"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/src/sl_net_basic_profiles.c"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/src/sl_net_credentials.c"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/src/sl_net_for_lwip.c"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/src/sli_net_common_utility.c"
    "${SDK_PATH}/../wiseconnect/components/sli_si91x_wifi_event_handler/src/sli_si91x_wifi_event_handler.c"
    "${SDK_PATH}/../wiseconnect/components/sli_wifi/src/sli_wifi.c"
    "${SDK_PATH}/../wiseconnect/components/sli_wifi/src/sli_wifi_utility.c"
    "${SDK_PATH}/../wiseconnect/components/sli_wifi_command_engine/src/sli_wifi_command_engine.c"
    "${SDK_PATH}/platform/CMSIS/RTOS2/Source/os_systick.c"
    "${SDK_PATH}/platform/common/src/sl_assert.c"
    "${SDK_PATH}/platform/common/src/sl_cmsis_os2_common.c"
    "${SDK_PATH}/platform/common/src/sl_core_cortexm.c"
    "${SDK_PATH}/platform/common/src/sl_slist.c"
    "${SDK_PATH}/platform/common/src/sl_string.c"
    "${SDK_PATH}/platform/common/src/sl_syscalls.c"
    "${SDK_PATH}/platform/common/src/sli_cmsis_os2_ext_task_register.c"
    "${SDK_PATH}/platform/service/sl_main/src/rtos/main_retarget.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_init.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_init_memory.c"
    "${SDK_PATH}/platform/service/sl_main/src/sl_main_kernel.c"
    "${SDK_PATH}/platform/service/sleeptimer/src/sl_sleeptimer.c"
    "${SDK_PATH}/util/third_party/freertos/cmsis/Source/cmsis_os2.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/croutine.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/event_groups.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/list.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/portable/GCC/ARM_CM4F/port.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/portable/MemMang/heap_4.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/queue.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/stream_buffer.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/tasks.c"
    "${SDK_PATH}/util/third_party/freertos/kernel/timers.c"
    "../autogen/sl_event_handler.c"
    "../main.c"
)

target_include_directories(slc_tuyaopen_switch_demo PUBLIC
   "../config"
   "../autogen"
    "${SDK_PATH}/../wiseconnect/components/protocol/wifi/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ble/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/cmsis_driver/CMSIS/Driver/Include"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/cmsis_driver"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/common/inc"
    "${SDK_PATH}/../wiseconnect/components/service/network_manager/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_api/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/rom_driver/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/chip/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/peripheral_drivers/inc"
    "${SDK_PATH}/../wiseconnect/components/board/silabs/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/core/config"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/service/clock_manager/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/unified_peripheral_drivers/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/crypto_utility/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/firmware_upgrade"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/gcm/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/hal/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/inc/mqtt/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/sl_net/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/ahb_interface/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/wireless/crypto/wrap/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/service/sleeptimer/inc"
    "${SDK_PATH}/../wiseconnect/components/sli_wifi/inc"
    "${SDK_PATH}/../wiseconnect/components/sli_wifi_command_engine/inc"
    "${SDK_PATH}/../wiseconnect/components/device/silabs/si91x/mcu/drivers/systemlevel/inc"
    "${SDK_PATH}/../wiseconnect/components/common/inc"
    "${SDK_PATH}/platform/common/inc"
    "${SDK_PATH}/platform/CMSIS/Core/Include"
    "${SDK_PATH}/platform/CMSIS/RTOS2/Include"
    "${SDK_PATH}/platform/emlib/inc"
    "${SDK_PATH}/util/third_party/freertos/kernel/include"
    "${SDK_PATH}/util/third_party/freertos/cmsis/Include"
    "${SDK_PATH}/util/third_party/freertos/kernel/portable/GCC/ARM_CM4F"
    "${SDK_PATH}/platform/service/sl_main/inc"
    "${SDK_PATH}/platform/service/sl_main/src"
    "${SDK_PATH}/platform/service/sleeptimer/inc"
)

target_compile_definitions(slc_tuyaopen_switch_demo PUBLIC
    "SLI_SI91X_ENABLE_IPV6=1"
    "SLI_SI91X_MCU_ENABLE_PSRAM_FEATURE=1"
    "SLI_SI91X_MCU_MOV_ROM_API_TO_FLASH=1"
    "SUPPORT_CPLUSPLUS=1"
    "TKL_MEMORY=MEMORY_FREERTOS_HEAP_PSRAM"
    "TKL_ULP_TIMER_SYSTICK_ENABLE=1"
    "WIFI_INIT_MODE_STA=1"
    "SLI_SI91X_MCU_COMMON_FLASH_MODE=1"
    "SLI_SI91X_MCU_CONFIG_RADIO_BOARD_BASE_VER=1"
    "SLI_SI91X_MCU_CONFIG_RADIO_BOARD_VER2=1"
    "SLI_SI91X_MCU_EXTERNAL_LDO_FOR_PSRAM=1"
    "SL_BOARD_NAME=\"BRD4002A\""
    "SL_BOARD_REV=\"A07\""
    "LFS_THREADSAFE=1"
    "SLI_SI91X_LWIP_HOSTED_NETWORK_STACK=1"
    "SL_TUYAOPEN_BSS_SEGMENT_IN_PSRAM=1"
    "SIWG917M111MGTBA=1"
    "SLI_SI917=1"
    "SLI_SI917B0=1"
    "SLI_SI91X_MCU_ENABLE_FLASH_BASED_EXECUTION=1"
    "SLI_SI91X_ENABLE_BLE=1"
    "SL_SI91X_ENABLE_LITTLE_ENDIAN=1"
    "BSS_SEGMENT_IN_PSRAM=1"
    "SLI_SI91X_MCU_ENABLE_RAM_BASED_EXECUTION=1"
    "__FREERTOS_OS_WISECONNECT=1"
    "SL_NET_COMPONENT_INCLUDED=1"
    "__STATIC_INLINE=static inline"
    "SLI_SI91X_MCU_PSRAM_APS6404L_SQH=1"
    "SLI_SI91X_MCU_PSRAM_PRESENT=1"
    "SL_SI91X_REQUIRES_INTF_PLL=1"
    "CLOCK_ROMDRIVER_PRESENT=1"
    "ULPSS_CLOCK_ROMDRIVER_PRESENT=1"
    "SL_SI91X_BOARD_INIT=1"
    "SRAM_BASE=0x0cUL"
    "SRAM_SIZE=0x4fc00UL"
    "SLI_CODE_CLASSIFICATION_DISABLE=1"
    "SLI_SI91X_MCU_ENABLE_IPMU_APIS=1"
    "SL_SI91X_SOC_MODE=1"
    "CRC_ROMDRIVER_PRESENT=1"
    "SL_SI91X_D_CACHE_ENABLE=1"
    "QSPI_ROMDRIVER_PRESENT=1"
    "TIMER_ROMDRIVER_PRESENT=1"
    "SL_SI91X_SI917_RAM_MEM_CONFIG=3"
    "UDMA_ROMDRIVER_PRESENT=1"
    "SL_SI91X_I2C_DMA=1"
    "SI917=1"
    "SLI_SI91X_ENABLE_OS=1"
    "SLI_SI91X_MCU_INTERFACE=1"
    "TA_DEEP_SLEEP_COMMON_FLASH=1"
    "SL_ULP_TIMER=1"
    "SL_SI91X_DMA=1"
    "SL_SI91X_ULP_UART=1"
    "SL_SI91X_USART_DMA=1"
    "ULP_UART_MODULE=1"
    "SI91X_PLATFORM=1"
    "SI91X_SYSRTC_PRESENT=1"
    "SL_SLEEP_TIMER=1"
    "__WEAK=__attribute__((weak))"
    "PLL_ROMDRIVER_PRESENT=1"
    "TEXT_SEGMENT_IN_PSRAM=1"
    "SL_WIFI_COMPONENT_INCLUDED=1"
    "configNUM_SDK_THREAD_LOCAL_STORAGE_POINTERS=2"
    "SL_COMPONENT_CATALOG_PRESENT=1"
    "SL_CODE_COMPONENT_FREERTOS_KERNEL=freertos_kernel"
    "SL_CODE_COMPONENT_CORE=core"
    "SL_CODE_COMPONENT_SLEEPTIMER=sleeptimer"
)

target_link_libraries(slc_tuyaopen_switch_demo PUBLIC
    "-Wl,--start-group"
    "gcc"
    "nosys"
    "c"
    "m"
    "stdc++"
    "-Wl,--end-group"
)
target_compile_options(slc_tuyaopen_switch_demo PUBLIC
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m4>
    $<$<COMPILE_LANGUAGE:C>:-mthumb>
    $<$<COMPILE_LANGUAGE:C>:-mfpu=fpv4-sp-d16>
    $<$<COMPILE_LANGUAGE:C>:-mfloat-abi=softfp>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Og>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:C>:-g>
    "$<$<COMPILE_LANGUAGE:C>:SHELL:-Wall -Werror>"
    "$<$<COMPILE_LANGUAGE:C>:SHELL:-Wno-unused-parameter -Wno-error=array-bounds -Wno-error=stringop-truncation -Wno-missing-field-initializers>"
    $<$<COMPILE_LANGUAGE:C>:-mfp16-format=ieee>
    $<$<COMPILE_LANGUAGE:C>:-Wno-error=deprecated-declarations>
    "$<$<COMPILE_LANGUAGE:C>:SHELL:-Wall -Werror -Wno-error=deprecated-declarations>"
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m4>
    $<$<COMPILE_LANGUAGE:C>:-fno-lto>
    $<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:CXX>:-mcpu=cortex-m4>
    $<$<COMPILE_LANGUAGE:CXX>:-mthumb>
    $<$<COMPILE_LANGUAGE:CXX>:-mfpu=fpv4-sp-d16>
    $<$<COMPILE_LANGUAGE:CXX>:-mfloat-abi=softfp>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Og>
    $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:CXX>:-g>
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-Wall -Werror>"
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-Wno-unused-parameter -Wno-error=array-bounds -Wno-error=stringop-truncation -Wno-missing-field-initializers>"
    $<$<COMPILE_LANGUAGE:CXX>:-mfp16-format=ieee>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=deprecated-declarations>
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-Wall -Werror -Wno-error=deprecated-declarations>"
    $<$<COMPILE_LANGUAGE:CXX>:-mcpu=cortex-m4>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-lto>
    $<$<COMPILE_LANGUAGE:CXX>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m4>
    $<$<COMPILE_LANGUAGE:ASM>:-mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv4-sp-d16>
    $<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=softfp>
    "$<$<COMPILE_LANGUAGE:ASM>:SHELL:-x assembler-with-cpp>"
)

set(post_build_command )
set_property(TARGET slc_tuyaopen_switch_demo PROPERTY C_STANDARD 17)
set_property(TARGET slc_tuyaopen_switch_demo PROPERTY CXX_STANDARD 17)
set_property(TARGET slc_tuyaopen_switch_demo PROPERTY CXX_EXTENSIONS OFF)

target_link_options(slc_tuyaopen_switch_demo INTERFACE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=softfp
    -T${CMAKE_CURRENT_LIST_DIR}/../autogen/linkerfile_psram_SoC.ld
    --specs=nano.specs
    "SHELL:-Xlinker -Map=$<TARGET_FILE_DIR:tuyaopen_switch_demo>/tuyaopen_switch_demo.map"
    "SHELL:-u _printf_float"
    -Wl,--wrap=main
    -fno-lto
    -Wl,--gc-sections
)

# BEGIN_SIMPLICITY_STUDIO_METADATA=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END_SIMPLICITY_STUDIO_METADATA
