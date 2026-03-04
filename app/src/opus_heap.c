/*
 * SPDX-FileCopyrightText: 2024-2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtthread.h"
#include "rtdevice.h"
#include "mem_section.h"
#include "os_support.h"

#define OPUS_HEAP_SIZE (1024 * 1024)
L2_RET_BSS_SECT_BEGIN(opus_heap_pool)
ALIGN(4)
static uint8_t opus_heap_pool[OPUS_HEAP_SIZE] L2_RET_BSS_SECT(opus_heap_pool);
L2_RET_BSS_SECT_END

static struct rt_memheap opus_memheap;

// TODO:
int mbedtls_platform_set_calloc_free(void *(*calloc_func)(size_t, size_t),
                                     void (*free_func)(void *));

void *opus_heap_malloc(uint32_t size)
{
    return rt_memheap_alloc(&opus_memheap, size);
}

void opus_heap_free(void *p)
{
    rt_memheap_free(p);
}

void *opus_heap_calloc(size_t count, size_t size)
{
    return rt_memheap_calloc(&opus_memheap, count, size);
}

int opus_heap_init(void)
{
    rt_memheap_init(&opus_memheap, "opus_memheap", (void *)opus_heap_pool,
                    sizeof(opus_heap_pool));

    mbedtls_platform_set_calloc_free(opus_heap_calloc, opus_heap_free);
    return 0;
}
INIT_BOARD_EXPORT(opus_heap_init);
