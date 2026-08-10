/*
 * FreeRTOS Kernel V11.1.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * A sample implementation of pvPortMallocPsram() and vPortFreePsram() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "psram_heap_4.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
#define configHEAP_CLEAR_MEMORY_ON_FREE 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* Max value that fits in a size_t type. */
#define heapSIZE_MAX (~((size_t)0))

/* Check if multiplying a and b will result in overflow. */
#define heapMULTIPLY_WILL_OVERFLOW(a, b) (((a) > 0) && ((b) > (heapSIZE_MAX / (a))))

/* Check if adding a and b will result in overflow. */
#define heapADD_WILL_OVERFLOW(a, b) ((a) > (heapSIZE_MAX - (b)))

/* Check if the subtraction operation ( a - b ) will result in underflow. */
#define heapSUBTRACT_WILL_UNDERFLOW(a, b) ((a) < (b))

/* MSB of the xBlockSize member of an BlockLink_t structure is used to track
 * the allocation status of a block.  When MSB of the xBlockSize member of
 * an BlockLink_t structure is set then the block belongs to the application.
 * When the bit is free the block is still part of the free heap space. */
#define heapBLOCK_ALLOCATED_BITMASK         (((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1))
#define heapBLOCK_SIZE_IS_VALID(xBlockSize) (((xBlockSize)&heapBLOCK_ALLOCATED_BITMASK) == 0)
#define heapBLOCK_IS_ALLOCATED(pxBlock)     (((pxBlock->xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) != 0)
#define heapALLOCATE_BLOCK(pxBlock)         ((pxBlock->xBlockSize) |= heapBLOCK_ALLOCATED_BITMASK)
#define heapFREE_BLOCK(pxBlock)             ((pxBlock->xBlockSize) &= ~heapBLOCK_ALLOCATED_BITMASK)

/*-----------------------------------------------------------*/

static uint8_t ucHeapPsram[configTOTAL_HEAP_SIZE_PSRAM] __attribute__((section("psram_freertos_heap")))
__attribute__((aligned(4)));

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK {
    struct A_BLOCK_LINK *pxNextFreeBlock; /**< The next free block in the list. */
    size_t               xBlockSize;      /**< The size of the free block. */
} BlockLink_t;

/* Setting configENABLE_HEAP_PROTECTOR to 1 enables heap block pointers
 * protection using an application supplied canary value to catch heap
 * corruption should a heap buffer overflow occur.
 */
#if (configENABLE_HEAP_PROTECTOR == 1)

/**
 * @brief Application provided function to get a random value to be used as canary.
 *
 * @param pxHeapCanary [out] Output parameter to return the canary value.
 */
extern void vApplicationGetRandomHeapCanary(portPOINTER_SIZE_TYPE *pxHeapCanary);

/* Canary value for protecting internal heap pointers. */
PRIVILEGED_DATA static portPOINTER_SIZE_TYPE xHeapCanary;

/* Macro to load/store BlockLink_t pointers to memory. By XORing the
 * pointers with a random canary value, heap overflows will result
 * in randomly unpredictable pointer values which will be caught by
 * heapVALIDATE_BLOCK_POINTER assert. */
#define heapPROTECT_BLOCK_POINTER(pxBlock) ((BlockLink_t *)(((portPOINTER_SIZE_TYPE)(pxBlock)) ^ xHeapCanary))
#else

#define heapPROTECT_BLOCK_POINTER(pxBlock) (pxBlock)

#endif /* configENABLE_HEAP_PROTECTOR */

/* Assert that a heap block pointer is within the heap bounds. */
#define heapVALIDATE_BLOCK_POINTER(pxBlock)                                                                            \
    configASSERT(((uint8_t *)(pxBlock) >= &(ucHeapPsram[0])) &&                                                        \
                 ((uint8_t *)(pxBlock) <= &(ucHeapPsram[configTOTAL_HEAP_SIZE_PSRAM - 1])))

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeListPsram(BlockLink_t *pxBlockToInsert) PRIVILEGED_FUNCTION;

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMallocPsram() is called.
 */
static void prvHeapInitPsram(void) PRIVILEGED_FUNCTION;

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize =
    (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
PRIVILEGED_DATA static BlockLink_t  xStartPsram;
PRIVILEGED_DATA static BlockLink_t *pxEndPsram = NULL;

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
PRIVILEGED_DATA static size_t xFreeBytesRemaining            = (size_t)0U;
PRIVILEGED_DATA static size_t xMinimumEverFreeBytesRemaining = (size_t)0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulAllocations = (size_t)0U;
PRIVILEGED_DATA static size_t xNumberOfSuccessfulFrees       = (size_t)0U;

/*-----------------------------------------------------------*/

void *pvPortMallocPsram(size_t xWantedSize)
{
    BlockLink_t *pxBlock;
    BlockLink_t *pxPreviousBlock;
    BlockLink_t *pxNewBlockLink;
    void        *pvReturn = NULL;
    size_t       xAdditionalRequiredSize;

    if (xWantedSize > 0) {
        /* The wanted size must be increased so it can contain a BlockLink_t
         * structure in addition to the requested amount of bytes. */
        if (heapADD_WILL_OVERFLOW(xWantedSize, xHeapStructSize) == 0) {
            xWantedSize += xHeapStructSize;

            /* Ensure that blocks are always aligned to the required number
             * of bytes. */
            if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00) {
                /* Byte alignment required. */
                xAdditionalRequiredSize = portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

                if (heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0) {
                    xWantedSize += xAdditionalRequiredSize;
                } else {
                    xWantedSize = 0;
                }
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        } else {
            xWantedSize = 0;
        }
    } else {
        mtCOVERAGE_TEST_MARKER();
    }

    vTaskSuspendAll();
    {
        /* If this is the first call to malloc then the heap will require
         * initialisation to setup the list of free blocks. */
        if (pxEndPsram == NULL) {
            prvHeapInitPsram();
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Check the block size we are trying to allocate is not so large that the
         * top bit is set.  The top bit of the block size member of the BlockLink_t
         * structure is used to determine who owns the block - the application or
         * the kernel, so it must be free. */
        if (heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0) {
            if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining)) {
                /* Traverse the list from the start (lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &xStartPsram;
                pxBlock         = heapPROTECT_BLOCK_POINTER(xStartPsram.pxNextFreeBlock);
                heapVALIDATE_BLOCK_POINTER(pxBlock);

                while ((pxBlock->xBlockSize < xWantedSize) &&
                       (pxBlock->pxNextFreeBlock != heapPROTECT_BLOCK_POINTER(NULL))) {
                    pxPreviousBlock = pxBlock;
                    pxBlock         = heapPROTECT_BLOCK_POINTER(pxBlock->pxNextFreeBlock);
                    heapVALIDATE_BLOCK_POINTER(pxBlock);
                }

                while (xStartPsram.pxNextFreeBlock == 0) {
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if (pxBlock != pxEndPsram) {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = (void *)(((uint8_t *)heapPROTECT_BLOCK_POINTER(pxPreviousBlock->pxNextFreeBlock)) +
                                        xHeapStructSize);
                    heapVALIDATE_BLOCK_POINTER(pvReturn);

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                     * two. */
                    configASSERT(heapSUBTRACT_WILL_UNDERFLOW(pxBlock->xBlockSize, xWantedSize) == 0);

                    if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE) {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);
                        configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) == 0);

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize        = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        pxNewBlockLink->pxNextFreeBlock  = pxPreviousBlock->pxNextFreeBlock;
                        pxPreviousBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxNewBlockLink);
                    } else {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining) {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    } else {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    heapALLOCATE_BLOCK(pxBlock);
                    pxBlock->pxNextFreeBlock = NULL;
                    xNumberOfSuccessfulAllocations++;
                } else {
                    mtCOVERAGE_TEST_MARKER();
                }
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC(pvReturn, xWantedSize);
    }
    (void)xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
    {
        if (pvReturn == NULL) {
            vApplicationMallocFailedHook();
        } else {
            mtCOVERAGE_TEST_MARKER();
        }
    }
#endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);
    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFreePsram(void *pv)
{
    uint8_t     *puc = (uint8_t *)pv;
    BlockLink_t *pxLink;

    if (pv != NULL) {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = (void *)puc;

        heapVALIDATE_BLOCK_POINTER(pxLink);
        configASSERT(heapBLOCK_IS_ALLOCATED(pxLink) != 0);
        configASSERT(pxLink->pxNextFreeBlock == NULL);

        if (heapBLOCK_IS_ALLOCATED(pxLink) != 0) {
            if (pxLink->pxNextFreeBlock == NULL) {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                heapFREE_BLOCK(pxLink);
#if (configHEAP_CLEAR_MEMORY_ON_FREE == 1)
                {
                    /* Check for underflow as this can occur if xBlockSize is
                     * overwritten in a heap block. */
                    if (heapSUBTRACT_WILL_UNDERFLOW(pxLink->xBlockSize, xHeapStructSize) == 0) {
                        (void)memset(puc + xHeapStructSize, 0, pxLink->xBlockSize - xHeapStructSize);
                    }
                }
#endif

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE(pv, pxLink->xBlockSize);
                    prvInsertBlockIntoFreeListPsram(((BlockLink_t *)pxLink));
                    xNumberOfSuccessfulFrees++;
                }
                (void)xTaskResumeAll();
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        } else {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSizePsram(void)
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSizePsram(void)
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

// void vPortInitialiseBlocks(void)
// {
//   /* This just exists to keep the linker quiet. */
// }
/*-----------------------------------------------------------*/

void *pvPortCallocPsram(size_t xNum, size_t xSize)
{
    void *pv = NULL;

    if (heapMULTIPLY_WILL_OVERFLOW(xNum, xSize) == 0) {
        pv = pvPortMallocPsram(xNum * xSize);

        if (pv != NULL) {
            (void)memset(pv, 0, xNum * xSize);
        }
    }

    return pv;
}
/*-----------------------------------------------------------*/

void *pvPortReallocPsram(void *pv, size_t xWantedSize)
{
    uint8_t     *puc = (uint8_t *)pv;
    BlockLink_t *pxLink;
    size_t       xOldUsableSize;
    void        *pvReturn;

    if (pv == NULL) {
        return pvPortMallocPsram(xWantedSize);
    }

    if (xWantedSize == 0) {
        vPortFreePsram(pv);
        return NULL;
    }

    /* An BlockLink_t structure sits immediately before the user pointer and
     * carries the real block size, so the copy length is exact. A caller-level
     * wrapper cannot know it and would have to copy the NEW size, reading past
     * the end of the old block whenever the block grows. */
    puc -= xHeapStructSize;
    pxLink = (void *)puc;

    heapVALIDATE_BLOCK_POINTER(pxLink);
    configASSERT(heapBLOCK_IS_ALLOCATED(pxLink) != 0);

    xOldUsableSize = (pxLink->xBlockSize & ~heapBLOCK_ALLOCATED_BITMASK) - xHeapStructSize;

    /* Already big enough: realloc() is free to shrink in place. */
    if (xWantedSize <= xOldUsableSize) {
        return pv;
    }

    pvReturn = pvPortMallocPsram(xWantedSize);

    if (pvReturn == NULL) {
        /* realloc() must leave the original block valid when it fails. */
        return NULL;
    }

    (void)memcpy(pvReturn, pv, xOldUsableSize);
    vPortFreePsram(pv);

    return pvReturn;
}
/*-----------------------------------------------------------*/

static void prvHeapInitPsram(void) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t          *pxFirstFreeBlock;
    portPOINTER_SIZE_TYPE uxStartAddress, uxEndAddress;
    size_t                xTotalHeapSize = configTOTAL_HEAP_SIZE_PSRAM;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxStartAddress = (portPOINTER_SIZE_TYPE)ucHeapPsram;

    if ((uxStartAddress & portBYTE_ALIGNMENT_MASK) != 0) {
        uxStartAddress += (portBYTE_ALIGNMENT - 1);
        uxStartAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= (size_t)(uxStartAddress - (portPOINTER_SIZE_TYPE)ucHeapPsram);
    }

#if (configENABLE_HEAP_PROTECTOR == 1)
    {
        vApplicationGetRandomHeapCanary(&(xHeapCanary));
    }
#endif

    /* xStartPsram is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    xStartPsram.pxNextFreeBlock = (void *)heapPROTECT_BLOCK_POINTER(uxStartAddress);
    xStartPsram.xBlockSize      = (size_t)0;

    /* pxEndPsram is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxEndAddress = uxStartAddress + (portPOINTER_SIZE_TYPE)xTotalHeapSize;
    uxEndAddress -= (portPOINTER_SIZE_TYPE)xHeapStructSize;
    uxEndAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
    pxEndPsram                  = (BlockLink_t *)uxEndAddress;
    pxEndPsram->xBlockSize      = 0;
    pxEndPsram->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(NULL);

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by pxEndPsram. */
    pxFirstFreeBlock                  = (BlockLink_t *)uxStartAddress;
    pxFirstFreeBlock->xBlockSize      = (size_t)(uxEndAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
    pxFirstFreeBlock->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxEndPsram);

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining            = pxFirstFreeBlock->xBlockSize;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeListPsram(BlockLink_t *pxBlockToInsert) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t *pxIterator;
    uint8_t     *puc;
    while (xStartPsram.pxNextFreeBlock == 0) {
    }
    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for (pxIterator = &xStartPsram; heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock) < pxBlockToInsert;
         pxIterator = heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)) {
        /* Nothing to do here, just iterate to the right position. */
    }

    if (pxIterator != &xStartPsram) {
        heapVALIDATE_BLOCK_POINTER(pxIterator);
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = (uint8_t *)pxIterator;

    if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert) {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    } else {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = (uint8_t *)pxBlockToInsert;

    if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)) {
        if (heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock) != pxEndPsram) {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxIterator->pxNextFreeBlock)->pxNextFreeBlock;
        } else {
            pxBlockToInsert->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxEndPsram);
        }
    } else {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if (pxIterator != pxBlockToInsert) {
        pxIterator->pxNextFreeBlock = heapPROTECT_BLOCK_POINTER(pxBlockToInsert);
    } else {
        mtCOVERAGE_TEST_MARKER();
    }
    while (xStartPsram.pxNextFreeBlock == 0) {
    }
}
/*-----------------------------------------------------------*/

void vPortGetHeapStatsPsram(HeapStats_t *pxHeapStats)
{
    BlockLink_t *pxBlock;
    size_t       xBlocks = 0, xMaxSize = 0,
           xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = heapPROTECT_BLOCK_POINTER(xStartPsram.pxNextFreeBlock);

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if (pxBlock != NULL) {
            while (pxBlock != pxEndPsram) {
                /* Increment the number of blocks and record the largest block seen
                 * so far. */
                xBlocks++;

                if (pxBlock->xBlockSize > xMaxSize) {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if (pxBlock->xBlockSize < xMinSize) {
                    xMinSize = pxBlock->xBlockSize;
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = heapPROTECT_BLOCK_POINTER(pxBlock->pxNextFreeBlock);
            }
        }
    }
    (void)xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes  = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks             = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes     = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees       = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

/*
 * Reset the state in this file. This state is normally initialized at start up.
 * This function must be called by the application before restarting the
 * scheduler.
 */
void vPortHeapResetStatePsram(void)
{
    pxEndPsram = NULL;

    xFreeBytesRemaining            = (size_t)0U;
    xMinimumEverFreeBytesRemaining = (size_t)0U;
    xNumberOfSuccessfulAllocations = (size_t)0U;
    xNumberOfSuccessfulFrees       = (size_t)0U;
}
/*-----------------------------------------------------------*/
