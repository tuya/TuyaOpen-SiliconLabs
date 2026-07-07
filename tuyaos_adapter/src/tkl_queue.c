/*****************************************************************************//**
 * @file tkl_queue.c
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
#include "tkl_queue.h"
#include "tuya_error_code.h"
#include "cmsis_os2.h"

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/**
 * @brief Create message queue
 *
 * @param[in] msgsize message size
 * @param[in] msgcount message number
 * @param[out] queue the queue handle created
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_create_init(TKL_QUEUE_HANDLE *queue, int msgsize, int msgcount)
{
    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    *queue = (TKL_QUEUE_HANDLE)osMessageQueueNew(msgcount, msgsize, NULL);
    if (*queue == NULL) {
        return OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED;
    }

    return OPRT_OK;
}

/**
 * @brief post a message to the message queue
 *
 * @param[in] queue the handle of the queue
 * @param[in] data the data of the message
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_post(const TKL_QUEUE_HANDLE queue, void *data, uint32_t timeout)
{
    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    osStatus_t status = osMessageQueuePut((osMessageQueueId_t)queue, data, 0U, timeout);
    if (status != osOK) {
        return OPRT_OS_ADAPTER_QUEUE_SEND_FAIL;
    }

    return OPRT_OK;
}

/**
 * @brief fetch message from the message queue
 *
 * @param[in] queue the message queue handle
 * @param[inout] msg the message fetch form the message queue
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_fetch(const TKL_QUEUE_HANDLE queue, void *msg, uint32_t timeout)
{
    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    osStatus_t status = osMessageQueueGet((osMessageQueueId_t)queue, msg, NULL, timeout);
    if (status != osOK) {
        return OPRT_OS_ADAPTER_QUEUE_RECV_FAIL;
    }

    return OPRT_OK;
}

/**
 * @brief free the message queue
 *
 * @param[in] queue the message queue handle
 *
 * @return void
 */
void tkl_queue_free(const TKL_QUEUE_HANDLE queue)
{
    if (!queue) {
        return;
    }

    osMessageQueueDelete((osMessageQueueId_t)queue);
}
