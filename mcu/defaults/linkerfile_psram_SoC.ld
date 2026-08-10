/***************************************************************************//**
 * GCC Linker script for Silicon Labs devices
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

/* 
 * PSRAM split into 3 partition:
 *     1) psram: place text
 *     2) psram_bss: place tuyaopen lib bss
 *     3) psram_heap: place heap freertos
 */

MEMORY
{
   rom        (rx) : ORIGIN = 0x8202000, LENGTH = 0x1fe000
   ram        (rwx): ORIGIN = 0x400,     LENGTH = 0x4f800
   psram      (rwx): ORIGIN = 0xa002000, LENGTH = 0x1fe000
   psram_bss  (rwx): ORIGIN = 0xa200000, LENGTH = 0x200000
   psram_heap (rwx): ORIGIN = 0xa400000, LENGTH = 0x400000
}

MEMORY
{
   udma0   (rwx)  : ORIGIN = 0x4fc00, LENGTH = 0x400
   udma1   (rwx)  : ORIGIN = 0x24061c00, LENGTH = 0x400
}

_last_psram_bss_location = ORIGIN(psram_bss);
_last_psram_heap_location = ORIGIN(psram_heap);
_last_psram_location = ORIGIN(psram);
_last_ram_location = ORIGIN(ram);

ENTRY(Reset_Handler)
 
SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        . = ALIGN(32);
        
        KEEP(*(.reset_handler))
        *(EXCLUDE_FILE(*sl_si91x_bus.c.obj *sl_si91x_driver.c.obj *sli_wifi_command_engine.c.obj *sli_si91x_wifi_event_handler.c.obj *rsi_deepsleep_soc.c.obj *rsi_hal_mcu_m4_ram.c.obj *rsi_hal_mcu_m4_rom.c.obj *sl_sleeptimer.c.obj *sl_sleeptimer_hal_si91x_sysrtc.c.obj *rsi_sysrtc.c.obj *sl_si91x_low_power_tickless_mode.c.obj *croutine.c.obj *event_groups.c.obj *list.c.obj *queue.c.obj *stream_buffer.c.obj *tasks.c.obj *timers.c.obj *cmsis_os2.c.obj *freertos_umm_malloc_host.c.obj *malloc_buffers.c.obj *port.c.obj *heap_*.c.obj *sl_si91x_psram.c.obj *rsi_qspi.c.obj *rsi_pll.c.obj *rsi_egpio.c.obj *UDMA.c.obj *sl_rsi_utility.c.obj *ipmu_apis.c.obj *rsi_d_cache.c.obj *sli_si91x_power_manager.c.obj *sl_si91x_power_manager.c.obj *sl_si91x_clock_manager.c.obj *sli_si91x_clock_manager.c.obj *sl_platform.c.obj *ai_audio_player.c.obj *gcm.c.obj *cipher.c.obj) .text*)        
             KEEP(*(.init))
             KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)
        
        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)
        
        *(.rodata*)
        
        KEEP(*(.eh_fram e*))
        _last_psram_location = .;
    } > psram

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        _last_psram_location = .;
    } > psram

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        _last_psram_location = .;
    } > psram
    __exidx_end = .;
    __etext = .;

    /* _sidata is used in code startup code */
    _sidata = __etext;

    . = _last_ram_location;
    .data . :
    {
        __data_start__ = .;
        
        /* _sdata is used in startup code */
        _sdata = __data_start__;
        KEEP(*(.ramVector))
        *rsi_deepsleep_soc.c.obj(.text*)
        *sl_si91x_psram.c.obj(.text*)
        *sl_si91x_psram_handle.c.obj(.text*)
        *rsi_qspi.c.obj(.text*)
        *rsi_pll.c.obj(.text*)
        *rsi_egpio.c.obj(.text*)
        *rsi_deepsleep_soc.c.obj(.data*)
        *sl_si91x_psram.c.obj(.data*)
        *sl_si91x_psram_handle.c.obj(.data*)
        *rsi_qspi.c.obj(.data*)
        *rsi_pll.c.obj(.data*)
        *rsi_egpio.c.obj(.data*)
        *rsi_hal_mcu_m4_ram.c.obj(.text*)
        *rsi_hal_mcu_m4_rom.c.obj(.text*)
        *sl_si91x_driver.c.obj(.text*)
        *sl_si91x_bus.c.obj(.text*)
        *UDMA.c.obj(.text*)
        *sl_sleeptimer.c.obj(.text*)
        *sl_sleeptimer_hal_si91x_sysrtc.c.obj(.text*) 
        *rsi_sysrtc.c.obj(.text*)
        *sl_si91x_low_power_tickless_mode.c.obj(.text*)
        *sli_wifi_command_engine.c.obj(text*)
        *sli_si91x_wifi_event_handler.c.obj(text*)
        *croutine.c.obj(.text*)
        *event_groups.c.obj(.text*)
        *list.c.obj(.text*)
        *queue.c.obj(.text*)
        *cmsis_os2.c.obj(.text*)
        *stream_buffer.c.obj(.text*)
        *tasks.c.obj(.text*)
        *timers.c.obj(.text*)
        *freertos_umm_malloc_host.c.obj(.text*)
        *malloc_buffers.c.obj(.text*)
        *sl_rsi_utility.c.obj(.text*)
        *port.c.obj(.text*)
        *heap_*.c.obj(.text*)
        *rsi_d_cache.c.obj(.text*)
        *sl_platform.c.obj(.text*)
        *sl_si91x_power_manager.c.obj(.text*)
        *sli_si91x_power_manager.c.obj(.text*)
        *sl_si91x_clock_manager.c.obj(.text*)
        *sli_si91x_clock_manager.c.obj(.text*)
        *ai_audio_player.c.obj(.text*)
        *gcm.c.obj(.text*)
        *cipher.c.obj(.text*)
        *(.data*)
        *(.ramfunc*)
        /* ipmu calibration data */
        *(.common_ipmu_ram*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */

        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);
        
        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

        /* _edata is used in startup code */
        _edata = __data_end__;
        _last_ram_location = .;
    } > ram AT> psram

    . = _last_psram_bss_location;
    .tuyaopen_bss (NOLOAD) :
    {
        _tuyaopen_bss_section_start_ = .;
        . = ALIGN(4);
        *(tuyaopen_bss_to_psram*)
        *libbutton.a:*(.bss .bss.*)
        *liblibhttp.a:*(.bss .bss.*)
        *libtal_bluetooth.a:*(.bss .bss.*)
        *libtal_kv.a:*(.bss .bss.*)
        *libtal_wifi.a:*(.bss .bss.*)
        *libtuyaos.a:*(.bss .bss.*)
        *libcommon.a:*(.bss .bss.*)
        *libliblwip.a:*(.bss .bss.*)
        *libtal_cellular.a:*(.bss .bss.*)
        *libtal_network.a:*(.bss .bss.*)
        *libtuya_ai_basic.a:*(.bss .bss.*)
        *libled.a:*(.bss .bss.*)
        *liblibmqtt.a:*(.bss .bss.*)
        *libtal_cli.a:*(.bss .bss.*)
        *libtal_security.a:*(.bss .bss.*)
        *libtuyaapp.a:*(.bss .bss.*)
        *liblibcjson.a:*(.bss .bss.*)
        *liblibtls.a:*(.bss .bss.*)
        *libtal_driver.a:*(.bss .bss.*)
        *libtal_system.a:*(.bss .bss.*)
        *libtuya_cloud_service.a:*(.bss .bss.*)
        _tuyaopen_bss_section_end_ = .;
    } > psram_bss

    . = _last_psram_heap_location;
    .freertos_heap (NOLOAD) :
    {
        . = ALIGN(4);
        *(psram_freertos_heap*)
    } > psram_heap

    . = _last_ram_location;
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        /* Section name has no leading dot (see tuya_mem_section.h), so the
         * pattern must not either, or the input section stays orphaned. */
        *(tuyaopen_bss_to_ram*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        _last_ram_location = .;
    } > ram

    . = _last_ram_location;
    .stack (NOLOAD):
    {
        __StackLimit = .;
        KEEP(*(.stack*))
        . = ALIGN(4);
        __StackTop = .;
        PROVIDE(__stack = __StackTop);
        _last_ram_location = .;
    } > ram
    . = _last_ram_location;

    .heap (COPY):
    {
        __HeapBase = .;
        __end__ = .;
        end = __end__;
        _end = __end__;
        KEEP(*(.heap*))
        . = ORIGIN(ram) + LENGTH(ram);
          
        __HeapLimit = .;
    } > ram

    __heap_size = __HeapLimit - __HeapBase;
    .udma_addr0 :
    {
        *(.udma_addr0*)
    } > udma0 AT> psram
    
    .udma_addr1 :
    {
        *(.udma_addr1*)
    } > udma1 AT> psram

    /* littlefs_enable */  /* Flash Present */
    __main_flash_end__ = 0x8202000 + 0x1fe000;
    /* This is where we handle littlefs flash storage blocks. We use dummy sections for finding the configured
    * block sizes and then "place" them at the end of flash when the size is known. */
    .internal_storage2 (DSECT) : {
        KEEP(*(.internal_storage2*))
    } > rom
    .littlefs (DSECT) : {
        KEEP(*(.ltfs*))
    } > rom

    linker_littlefs_end = __main_flash_end__;
    linker_littlefs_begin = linker_littlefs_end - SIZEOF(.littlefs);
    linker_littlefs_size = SIZEOF(.littlefs);
    linker_storage2_end = linker_littlefs_begin;
    __littlefsBase = linker_littlefs_begin;
    linker_storage2_begin = linker_storage2_end - SIZEOF(.internal_storage2);
    linker_storage2_size = SIZEOF(.internal_storage2);
    ASSERT((linker_storage2_begin >= (SIZEOF(.data))), "FLASH memory overflowed !")  /* littlefs_enable */  /* Flash Present */
}
