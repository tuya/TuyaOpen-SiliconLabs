/**
 * @file tuyaopen_license.h
 * @brief Platform OTP/efuse license read/write for TuyaOpen authorize
 * @version 1.0
 * @date 2026-08-07
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __TUYAOPEN_LICENSE_H__
#define __TUYAOPEN_LICENSE_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read factory/OTP license JSON blob
 * @param[out] data allocated buffer holding license JSON (caller frees with tal_free)
 * @param[out] data_len length of data in bytes
 * @return OPRT_OK on success, OPRT_NOT_SUPPORTED if OTP license is unavailable
 * @note JSON format example:
 *       {"auzkey":"...","uuid":"...","prod_test":false,"ap_ssid":"SmartLife","mac":"..."}
 */
OPERATE_RET tuyaopen_license_read(CHAR_T **data, UINT32_T *data_len);

/**
 * @brief Write factory/OTP license JSON blob
 * @param[in] data license JSON string
 * @param[in] data_len length of data in bytes
 * @return OPRT_OK on success, OPRT_NOT_SUPPORTED if OTP write is unavailable
 */
OPERATE_RET tuyaopen_license_write(const CHAR_T *data, UINT32_T data_len);

#ifdef __cplusplus
}
#endif

#endif /* __TUYAOPEN_LICENSE_H__ */
