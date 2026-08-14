/*
 * A small, dedicated internal-RAM heap for the MP3 decoder's per-frame
 * scratch buffer and long-lived context.
 *
 * This is deliberately NOT the same pool as pvPortMalloc's ucHeap (see
 * psram_heap_4.c's sibling, the plain-internal-RAM FreeRTOS heap): that pool
 * also serves task stacks, queues and everything else, and a prior attempt to
 * route the decoder through it failed on real hardware -- pvPortMalloc(16KB)
 * returned NULL because the shared heap had no contiguous block that size
 * left. This pool never competes with anything else, so it can't be starved.
 *
 * Sized from the actual objects measured in the previous static-buffer
 * build: DECODER_MP3_CTX_T is 0x1a14 (6676) bytes, mp3dec_scratch_t is
 * 0x3f6c (16236) bytes. 23KB leaves a few hundred bytes of headroom for
 * allocator overhead. This is not a general-purpose heap: it exists to serve
 * exactly those two objects.
 *
 * Usage pattern this relies on: one long-lived ctx (allocated once at
 * decoder_mp3_start, freed at decoder_mp3_stop) and one scratch buffer
 * allocated and freed on every single mp3dec_decode_frame() call. Every
 * scratch alloc/free cycle requests the same size and finds the same freed
 * block, so nothing ever fragments in practice; forward-only coalescing
 * (merging a freed block with the next one if it's also free) is enough.
 *
 * Single-threaded use only: the MP3 decoder runs entirely inside the
 * ai_player thread, so there is no locking here. Do not reuse this pool from
 * more than one thread without adding one.
 */
#include <stddef.h>
#include <stdint.h>
#include "tuya_mem_section.h"

#define MP3_POOL_SIZE (23 * 1024)

typedef struct mp3_pool_block {
    size_t                 size; /* usable size, excludes this header */
    struct mp3_pool_block *next; /* next block in the pool, address order */
    uint8_t                free;
} mp3_pool_block_t;

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((size_t)(a) - 1))
#define MP3_POOL_HDR_SIZE ALIGN_UP(sizeof(mp3_pool_block_t), 8)

static uint8_t s_mp3_pool[MP3_POOL_SIZE] TUYA_MEM_SECTION_RAM __attribute__((aligned(8)));
static mp3_pool_block_t *s_mp3_pool_head;

static void mp3_pool_init(void)
{
    mp3_pool_block_t *first = (mp3_pool_block_t *)s_mp3_pool;
    first->size = MP3_POOL_SIZE - MP3_POOL_HDR_SIZE;
    first->next = NULL;
    first->free = 1;
    s_mp3_pool_head = first;
}

void *mp3_internal_malloc(size_t size)
{
    if (!s_mp3_pool_head) {
        mp3_pool_init();
    }

    size = ALIGN_UP(size, 8);

    for (mp3_pool_block_t *blk = s_mp3_pool_head; blk != NULL; blk = blk->next) {
        if (!blk->free || blk->size < size) {
            continue;
        }
        /* Split off the remainder as a new free block if there's enough
           room left for one; otherwise hand over the whole block. */
        if (blk->size >= size + MP3_POOL_HDR_SIZE + 8) {
            mp3_pool_block_t *rem = (mp3_pool_block_t *)((uint8_t *)blk + MP3_POOL_HDR_SIZE + size);
            rem->size = blk->size - size - MP3_POOL_HDR_SIZE;
            rem->next = blk->next;
            rem->free = 1;
            blk->next = rem;
            blk->size = size;
        }
        blk->free = 0;
        return (uint8_t *)blk + MP3_POOL_HDR_SIZE;
    }

    return NULL;
}

void mp3_internal_free(void *ptr)
{
    if (!ptr) {
        return;
    }

    mp3_pool_block_t *blk = (mp3_pool_block_t *)((uint8_t *)ptr - MP3_POOL_HDR_SIZE);
    blk->free = 1;

    /* Merge with the next block if it's free too. No backward coalescing:
       the usage pattern above never needs it (see file header). */
    if (blk->next != NULL && blk->next->free) {
        blk->size += MP3_POOL_HDR_SIZE + blk->next->size;
        blk->next = blk->next->next;
    }
}
