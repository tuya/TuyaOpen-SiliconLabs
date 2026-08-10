/**
 * @file tuya_mem_section.h
 * @brief SiWx917 linker section helpers for large static buffers
 * @version 1.0
 * @date 2026-08-10
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __TUYA_MEM_SECTION_H__
#define __TUYA_MEM_SECTION_H__

#ifdef __cplusplus
extern "C" {
#endif

#define TUYA_MEM_SECTION_RAM   __attribute__((section("tuyaopen_bss_to_ram")))
#define TUYA_MEM_SECTION_PSRAM __attribute__((section("tuyaopen_bss_to_psram")))

#ifdef __cplusplus
}
#endif

#endif /* __TUYA_MEM_SECTION_H__ */
